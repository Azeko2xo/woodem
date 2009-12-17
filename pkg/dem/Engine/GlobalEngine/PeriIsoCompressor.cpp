
// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#include<yade/pkg-dem/PeriIsoCompressor.hpp>
#include<yade/pkg-dem/Shop.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg-common/NormalShearInteractions.hpp>
#include<yade/pkg-dem/DemXDofGeom.hpp>

using namespace std;

YADE_PLUGIN((PeriIsoCompressor)(PeriTriaxController))


CREATE_LOGGER(PeriIsoCompressor);
void PeriIsoCompressor::action(Scene* rb){
	if(!rb->isPeriodic){ LOG_FATAL("Being used on non-periodic simulation!"); throw; }
	if(state>=stresses.size()) return;
	// initialize values
	if(charLen<=0){
		Bound* bv=Body::byId(0,rb)->bound.get();
		if(!bv){ LOG_FATAL("No charLen defined and body #0 has no bound"); throw; }
		const Vector3r sz=bv->max-bv->min;
		charLen=(sz[0]+sz[1]+sz[2])/3.;
		LOG_INFO("No charLen defined, taking avg bbox size of body #0 = "<<charLen);
	}
	if(maxSpan<=0){
		FOREACH(const shared_ptr<Body>& b, *rb->bodies){
			if(!b || !b->bound) continue;
			for(int i=0; i<3; i++) maxSpan=max(maxSpan,b->bound->max[i]-b->bound->min[i]);
		}
		
	}
	if(maxDisplPerStep<0) maxDisplPerStep=1e-2*charLen; // this should be tuned somehow…
	const long& step=rb->currentIteration;
	Vector3r cellSize=rb->cell.size; //unused: Real cellVolume=cellSize[0]*cellSize[1]*cellSize[2];
	Vector3r cellArea=Vector3r(cellSize[1]*cellSize[2],cellSize[0]*cellSize[2],cellSize[0]*cellSize[1]);
	Real minSize=min(cellSize[0],min(cellSize[1],cellSize[2])), maxSize=max(cellSize[0],max(cellSize[1],cellSize[2]));
	if(minSize<2.1*maxSpan){ throw runtime_error("Minimum cell size is smaller than 2.1*span_of_the_biggest_body! (periodic collider requirement)"); }
	if(((step%globalUpdateInt)==0) || avgStiffness<0 || sigma[0]<0 || sigma[1]<0 || sigma[2]<0){
		Vector3r sumForces=Shop::totalForceInVolume(avgStiffness,rb);
		sigma=-Vector3r(sumForces[0]/cellArea[0],sumForces[1]/cellArea[1],sumForces[2]/cellArea[2]);
		LOG_TRACE("Updated sigma="<<sigma<<", avgStiffness="<<avgStiffness);
	}
	Real sigmaGoal=stresses[state]; assert(sigmaGoal<0);
	// expansion of cell in this step (absolute length)
	Vector3r cellGrow(Vector3r::ZERO);
	// is the stress condition satisfied in all directions?
	bool allStressesOK=true;
	if(keepProportions){ // the same algo as below, but operating on quantitites averaged over all dimensions
		Real sigAvg=(sigma[0]+sigma[1]+sigma[2])/3., avgArea=(cellArea[0]+cellArea[1]+cellArea[2])/3., avgSize=(cellSize[0]+cellSize[1]+cellSize[2])/3.;
		Real avgGrow=1e-4*(sigmaGoal-sigAvg)*avgArea/(avgStiffness>0?avgStiffness:1);
		Real maxToAvg=maxSize/avgSize;
		if(abs(maxToAvg*avgGrow)>maxDisplPerStep) avgGrow=Mathr::Sign(avgGrow)*maxDisplPerStep/maxToAvg;
		avgGrow=max(avgGrow,-(minSize-2.1*maxSpan)/maxToAvg);
		if(avgStiffness>0) { sigma+=(avgGrow*avgStiffness)*Vector3r::ONE; sigAvg+=avgGrow*avgStiffness; }
		if(abs((sigAvg-sigmaGoal)/sigmaGoal)>5e-3) allStressesOK=false;
		cellGrow=(avgGrow/avgSize)*cellSize;
	}
	else{ // handle each dimension separately
		for(int axis=0; axis<3; axis++){
			// Δσ=ΔεE=(Δl/l)×(l×K/A) ↔ Δl=Δσ×A/K
			// FIXME: either NormalShearInteraction::{kn,ks} is computed wrong or we have dimensionality problem here
			// FIXME: that is why the fixup 1e-4 is needed here
			// FIXME: or perhaps maxDisplaPerStep=1e-2*charLen is too big??
			cellGrow[axis]=1e-4*(sigmaGoal-sigma[axis])*cellArea[axis]/(avgStiffness>0?avgStiffness:1);  // FIXME: wrong dimensions? See PeriTriaxController
			if(abs(cellGrow[axis])>maxDisplPerStep) cellGrow[axis]=Mathr::Sign(cellGrow[axis])*maxDisplPerStep;
			cellGrow[axis]=max(cellGrow[axis],-(cellSize[axis]-2.1*maxSpan));
			// crude way of predicting sigma, for steps when it is not computed from intrs
			if(avgStiffness>0) sigma[axis]+=cellGrow[axis]*avgStiffness; // FIXME: dimensions
			if(abs((sigma[axis]-sigmaGoal)/sigmaGoal)>5e-3) allStressesOK=false;
		}
	}
	TRVAR4(cellGrow,sigma,sigmaGoal,avgStiffness);
	rb->cell.size+=cellGrow;
	// handle state transitions
	if(allStressesOK){
		if((step%globalUpdateInt)==0) currUnbalanced=Shop::unbalancedForce(/*useMaxForce=*/false,rb);
		if(currUnbalanced<maxUnbalanced){
			state+=1;
			// sigmaGoal reached and packing stable
			if(state==stresses.size()){ // no next stress to go for
				LOG_INFO("Finished");
				if(!doneHook.empty()){ LOG_DEBUG("Running doneHook: "<<doneHook); PyGILState_STATE gstate; gstate=PyGILState_Ensure(); PyRun_SimpleString(doneHook.c_str()); PyGILState_Release(gstate); }
			} else { LOG_INFO("Loaded to "<<sigmaGoal<<" done, going to "<<stresses[state]<<" now"); }
		} else {
			if((step%globalUpdateInt)==0) LOG_DEBUG("Stress="<<sigma<<", goal="<<sigmaGoal<<", unbalanced="<<currUnbalanced);
		}
	}
}

void PeriTriaxController::strainStressStiffUpdate(){
	// update strain first
	const Vector3r& cellSize(scene->cell.size);
	for(int i=0; i<3; i++) strain[i]=(cellSize[i]-refSize[i])/refSize[i];
	// stress and stiffness
	Vector3r sumForce(Vector3r::ZERO), sumStiff(Vector3r::ZERO), sumLength(Vector3r::ZERO);
	int n=0;
	FOREACH(const shared_ptr<Interaction>&I, *scene->interactions){
		if(!I->isReal()) continue;
		n++;
		NormalShearInteraction* nsi=YADE_CAST<NormalShearInteraction*>(I->interactionPhysics.get());
		GenericSpheresContact* gsc=YADE_CAST<GenericSpheresContact*>(I->interactionGeometry.get());
		for(int i=0; i<3; i++){
			const Real absNormI=abs(gsc->normal[i]);
			Real f=(reversedForces?-1.:1.)*(nsi->normalForce[i]+nsi->shearForce[i]);
			// force is as applied to id1, and normal is from id1 towards id2;
			// therefore if they have the same sense, it is tensile force (positive)
			sumForce[i]+=Mathr::Sign(f*gsc->normal[i])*abs(f);
			sumLength[i]+=absNormI*(gsc->refR1+gsc->refR2);
			sumStiff[i]+=absNormI*nsi->kn+(1-absNormI)*nsi->ks;
		}
	}
	if(n>0){
		// sumForce/area is sum of stresses on fictious slices; the number of slices is (average length)/length in the respective sense
		for(int i=0; i<3; i++) stress[i]=((1./n)*sumLength[i]/cellSize[i])*sumForce[i]/(cellSize[(i+1)%3]*cellSize[(i+2)%3]);
		stiff=(1./n)*sumStiff;
	} else { stiff=Vector3r::ZERO; stress=Vector3r::ZERO; }
}

CREATE_LOGGER(PeriTriaxController);

void PeriTriaxController::action(Scene* scene){
	if(!scene->isPeriodic){ throw runtime_error("PeriTriaxController run on aperiodic simulation."); }
	const Vector3r& cellSize=scene->cell.size;
	Vector3r cellArea=Vector3r(cellSize[1]*cellSize[2],cellSize[0]*cellSize[2],cellSize[0]*cellSize[1]);
	// initial updates
	if(refSize[0]<0) refSize=cellSize;
	if(maxBodySpan[0]<=0){
		FOREACH(const shared_ptr<Body>& b,*scene->bodies){
			if(!b || !b->bound) continue;
			for(int i=0; i<3; i++) maxBodySpan[i]=max(maxBodySpan[i],b->bound->max[i]-b->bound->min[i]);
		}
	}
	// check current size
	if(2.1*maxBodySpan[0]>cellSize[0] || 2.1*maxBodySpan[1]>cellSize[1] || 2.1*maxBodySpan[2]>cellSize[2]){
		LOG_DEBUG("cellSize="<<cellSize<<", maxBodySpan="<<maxBodySpan);
		throw runtime_error("Minimum cell size is smaller than 2.1*maxBodySpan (periodic collider requirement)");
	}
	bool doUpdate((scene->currentIteration%globUpdate)==0);
	if(doUpdate || min(stiff[0],min(stiff[1],stiff[2]))<=0){ strainStressStiffUpdate(); }

	bool allOk=true; Vector3r cellGrow(Vector3r::ZERO);
	// apply condition along each axis separately (stress or strain)
	for(int axis=0; axis<3; axis++){
		Real maxGrow=refSize[axis]*maxStrainRate[axis]*scene->dt;
		if(stressMask & (1<<axis)){ // control stress
			// stiffness K=EA; σ₁=goal stress; Δσ wanted stress difference to apply
			// ΔεE=(Δl/l₀)(K/A) = Δσ=σ₁-σ ↔ Δl=(σ₁-σ)l₀(A/K)
			cellGrow[axis]=(goal[axis]-stress[axis])*refSize[axis]*cellArea[axis]/(stiff[axis]>0?stiff[axis]:1.);
			LOG_TRACE(axis<<": stress="<<stress[axis]<<", goal="<<goal[axis]<<", cellGrow="<<cellGrow[axis]);
		} else { // control strain
			cellGrow[axis]=(goal[axis]-strain[axis])*refSize[axis];
			LOG_TRACE(axis<<": strain="<<strain[axis]<<", goal="<<goal[axis]<<", cellGrow="<<cellGrow[axis]);
		}
		// limit maximum strain rate
		if(abs(cellGrow[axis])>maxGrow) cellGrow[axis]=Mathr::Sign(cellGrow[axis])*maxGrow;
		// do not shrink below minimum cell size (periodic collider condition), although it is suboptimal WRT resulting stress
		cellGrow[axis]=max(cellGrow[axis],-(cellSize[axis]-2.1*maxBodySpan[axis]));
		// steady evolution with fluctuations; see TriaxialStressController
		cellGrow[axis]=(1-growDamping)*cellGrow[axis]+.8*prevGrow[axis];
		// crude way of predicting stress, for steps when it is not computed from intrs
		if(doUpdate) LOG_DEBUG(axis<<": cellGrow="<<cellGrow[axis]<<", new stress="<<stress[axis]<<", new strain="<<strain[axis]);
		// signal if condition not satisfied
		if(stressMask & (1<<axis)){
			Real curr=stress[axis];
			if((goal[axis]!=0 && abs((curr-goal[axis])/goal[axis])>relStressTol) || abs(curr-goal[axis])>absStressTol) allOk=false;
		} else {
			Real curr=strain[axis];
			// since strain is prescribed exactly, tolerances need just to accomodate rounding issues
			if((goal[axis]!=0 && abs((curr-goal[axis])/goal[axis])>1e-6) || abs(curr-goal[axis])>1e-6) {
				allOk=false;
				if(doUpdate) LOG_DEBUG("Strain not OK; "<<abs(curr-goal[axis])<<">1e-6");
			}
		}
	}
	// update stress and strain
	for(int axis=0; axis<3; axis++){
		strain[axis]+=cellGrow[axis]/refSize[axis];
		// take in account something like poisson's effect here…
		//Real bogusPoisson=0.25; int ax1=(axis+1)%3,ax2=(axis+2)%3;
		if(stiff[axis]>0) stress[axis]+=(cellGrow[axis]/refSize[axis])*(stiff[axis]/cellArea[axis]); //-bogusPoisson*(cellGrow[ax1]/refSize[ax1])*(stiff[ax1]/cellArea[ax1])-bogusPoisson*(cellGrow[ax2]/refSize[ax2])*(stiff[ax2]/cellArea[ax2]);
	}
	// change cell size now
	scene->cell.size+=cellGrow;
	strainRate=cellGrow/scene->dt;

	if(allOk){
		if(doUpdate || currUnbalanced<0) {
			currUnbalanced=Shop::unbalancedForce(/*useMaxForce=*/false,scene);
			LOG_DEBUG("Stress/strain="<<(stressMask&1?stress[0]:strain[0])<<","<<(stressMask&2?stress[1]:strain[1])<<","<<(stressMask&4?stress[2]:strain[2])<<", goal="<<goal<<", unbalanced="<<currUnbalanced);
		}
		if(currUnbalanced<maxUnbalanced){
			// LOG_INFO("Goal reached, packing stable.");
			if(!doneHook.empty()){
				LOG_DEBUG("Running doneHook: "<<doneHook);
				PyGILState_STATE gstate; gstate=PyGILState_Ensure(); PyRun_SimpleString(doneHook.c_str()); PyGILState_Release(gstate);
			} else { Omega::instance().stopSimulationLoop(); }
		}
	}
}
