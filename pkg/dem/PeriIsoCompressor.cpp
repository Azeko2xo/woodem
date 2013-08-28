
// 2009 © Václav Šmilauer <eudoxos@arcig.cz>

#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/PeriIsoCompressor.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Funcs.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/lib/pyutil/gil.hpp>

CREATE_LOGGER(PeriIsoCompressor);

WOO_PLUGIN(dem,(PeriIsoCompressor))

void PeriIsoCompressor::avgStressIsoStiffness(const Vector3r& cellAreas, Vector3r& stress, Real& isoStiff){
	Vector3r force(Vector3r::Zero()); Real stiff=0; long n=0;
	FOREACH(const shared_ptr<Contact>& C, *dem->contacts){
		const auto fp=dynamic_pointer_cast<FrictPhys>(C->phys); // needed for stiffness
		if(!fp) continue;
		force+=(C->geom->node->ori.conjugate()*C->phys->force).array().abs().matrix();
		stiff+=(1/3.)*fp->kn+(2/3.)*fp->kt; // count kn in one direction and ks in the other two
		n++;
	}
	isoStiff= n>0 ? (1./n)*stiff : -1;
	stress=-Vector3r(force[0]/cellAreas[0],force[1]/cellAreas[1],force[2]/cellAreas[2]);
}


void PeriIsoCompressor::run(){
	dem=static_cast<DemField*>(field.get());

	if(!scene->isPeriodic){ LOG_FATAL("Being used on non-periodic simulation!"); throw; }
	if(state>=stresses.size()) return;
	// initialize values
	if(charLen<=0){
		if(dem->particles->size()>0 && (*dem->particles)[0] && (*dem->particles)[0]->shape && (*dem->particles)[0]->shape->bound){
			const Bound& bv=*((*dem->particles)[0]->shape->bound);
			const Vector3r sz=bv.max-bv.min;
			charLen=(sz[0]+sz[1]+sz[2])/3.;
			LOG_INFO("No charLen defined, taking avg bbox size of body #0 = "<<charLen);
		} else { LOG_FATAL("No charLen defined and body #0 does not exist has no bound"); throw; }
	}
	if(maxSpan<=0){
		FOREACH(const shared_ptr<Particle>& p, *dem->particles){
			if(!p || !p->shape || !p->shape->bound) continue;
			for(int i=0; i<3; i++) maxSpan=max(maxSpan,p->shape->bound->max[i]-p->shape->bound->min[i]);
		}
	}
	if(maxDisplPerStep<0) maxDisplPerStep=1e-2*charLen; // this should be tuned somehow…
	const long& step=scene->step;
	Vector3r cellSize=scene->cell->getSize(); //unused: Real cellVolume=cellSize[0]*cellSize[1]*cellSize[2];
	Vector3r cellArea=Vector3r(cellSize[1]*cellSize[2],cellSize[0]*cellSize[2],cellSize[0]*cellSize[1]);
	Real minSize=min(cellSize[0],min(cellSize[1],cellSize[2])), maxSize=max(cellSize[0],max(cellSize[1],cellSize[2]));
	if(minSize<2.1*maxSpan){ throw runtime_error("Minimum cell size is smaller than 2.1*span_of_the_biggest_body! (periodic collider requirement)"); }
	if(((step%globalUpdateInt)==0) || isnan(avgStiffness) || isnan(sigma[0]) || isnan(sigma[1])|| isnan(sigma[2])){
		avgStressIsoStiffness(cellArea,sigma,avgStiffness);
		LOG_TRACE("Updated sigma="<<sigma<<", avgStiffness="<<avgStiffness);
	}
	Real sigmaGoal=stresses[state]; assert(sigmaGoal<0);
	// expansion of cell in this step (absolute length)
	Vector3r cellGrow(Vector3r::Zero());
	// is the stress condition satisfied in all directions?
	bool allStressesOK=true;
	if(keepProportions){ // the same algo as below, but operating on quantitites averaged over all dimensions
		Real sigAvg=(sigma[0]+sigma[1]+sigma[2])/3., avgArea=(cellArea[0]+cellArea[1]+cellArea[2])/3., avgSize=(cellSize[0]+cellSize[1]+cellSize[2])/3.;
		Real avgGrow=1e-4*(sigmaGoal-sigAvg)*avgArea/(avgStiffness>0?avgStiffness:1);
		Real maxToAvg=maxSize/avgSize;
		if(abs(maxToAvg*avgGrow)>maxDisplPerStep) avgGrow=Mathr::Sign(avgGrow)*maxDisplPerStep/maxToAvg;
		Real okGrow=-(minSize-2.1*maxSpan)/maxToAvg;
		if(avgGrow<okGrow) throw runtime_error("Unable to shrink cell due to maximum body size (although required by stress condition). Increase particle rigidity, increase total sample dimensions, or decrease goal stress.");
		// avgGrow=max(avgGrow,-(minSize-2.1*maxSpan)/maxToAvg);
		if(avgStiffness>0) { sigma+=(avgGrow*avgStiffness)*Vector3r::Ones(); sigAvg+=avgGrow*avgStiffness; }
		if(abs((sigAvg-sigmaGoal)/sigmaGoal)>5e-3) allStressesOK=false;
		cellGrow=(avgGrow/avgSize)*cellSize;
	}
	else{ // handle each dimension separately
		for(int axis=0; axis<3; axis++){
			// Δσ=ΔεE=(Δl/l)×(l×K/A) ↔ Δl=Δσ×A/K
			// FIXME: either NormShearPhys::{kn,ks} is computed wrong or we have dimensionality problem here
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
	assert(scene->dt>0);
	for(int axis=0; axis<3; axis++){ scene->cell->nextGradV(axis,axis)=cellGrow[axis]/(scene->dt*scene->cell->getSize()[axis]); }

	// handle state transitions
	if(allStressesOK){
		if((step%globalUpdateInt)==0) currUnbalanced=DemFuncs::unbalancedForce(scene,dem,/*useMaxForce=*/false);
		if(currUnbalanced<maxUnbalanced){
			state+=1;
			// sigmaGoal reached and packing stable
			if(state==stresses.size()){ // no next stress to go for
				LOG_INFO("Finished");
				if(!doneHook.empty()){ LOG_DEBUG("Running doneHook: "<<doneHook); Engine::runPy(doneHook); }
			} else { LOG_INFO("Loaded to "<<sigmaGoal<<" done, going to "<<stresses[state]<<" now"); }
		} else {
			if((step%globalUpdateInt)==0){ LOG_DEBUG("Stress="<<sigma<<", goal="<<sigmaGoal<<", unbalanced="<<currUnbalanced); }
		}
	}
}



CREATE_LOGGER(WeirdTriaxControl);
WOO_PLUGIN(dem,(WeirdTriaxControl))

void WeirdTriaxControl::run(){
	dem=static_cast<DemField*>(field.get());
	if (!scene->isPeriodic){ throw runtime_error("PeriTriaxController run on aperiodic simulation."); }
	bool doUpdate((scene->step%globUpdate)==0);
	if(doUpdate){
		//"Natural" strain, still correct for large deformations, used for comparison with goals
		for (int i=0;i<3;i++) strain[i]=log(scene->cell->trsf(i,i));
		Matrix6r stiffness;
		std::tie(stress,stiffness)=DemFuncs::stressStiffness(scene,dem,/*skipMultinodal*/false,scene->cell->getVolume()*relVol);
	}
	if(isnan(mass) || mass<=0){ throw std::runtime_error("WeirdTriaxControl.mass must be positive, not "+to_string(mass)); }

	bool allOk=true;

	// apply condition along each axis separately (stress or strain)
	for(int axis=0; axis<3; axis++){
 		Real& strain_rate=scene->cell->nextGradV(axis,axis);//strain rate on axis
		if(stressMask & (1<<axis)){   // control stress
			assert(mass>0);//user set
			Real dampFactor=1-growDamping*Mathr::Sign(strain_rate*(goal[axis]-stress(axis,axis)));
			strain_rate+=dampFactor*scene->dt*(goal[axis]-stress(axis,axis))/mass;
			LOG_TRACE(axis<<": stress="<<stress(axis,axis)<<", goal="<<goal[axis]<<", gradV="<<strain_rate);
		} else {
			// control strain, see "true strain" definition here http://en.wikipedia.org/wiki/Finite_strain_theory
			strain_rate=(exp(goal[axis]-strain[axis])-1)/scene->dt;
			LOG_TRACE ( axis<<": strain="<<strain[axis]<<", goal="<<goal[axis]<<", cellGrow="<<strain_rate*scene->dt);
		}
		// limit maximum strain rate
		if (abs(strain_rate)>maxStrainRate[axis]) strain_rate=Mathr::Sign(strain_rate)*maxStrainRate[axis];

		// crude way of predicting stress, for steps when it is not computed from intrs
		if(doUpdate) LOG_DEBUG(axis<<": cellGrow="<<strain_rate*scene->dt<<", new stress="<<stress(axis,axis)<<", new strain="<<strain[axis]);
		// used only for temporary goal comparisons. The exact value is assigned in strainStressStiffUpdate
		strain[axis]+=strain_rate*scene->dt;
		// signal if condition not satisfied
		if(stressMask&(1<<axis)){
			Real curr=stress(axis,axis);
			if((goal[axis]!=0 && abs((curr-goal[axis])/goal[axis])>relStressTol) || abs(curr-goal[axis])>absStressTol) allOk=false;
		}else{
			Real curr=strain[axis];
			// since strain is prescribed exactly, tolerances need just to accomodate rounding issues
			if((goal[axis]!=0 && abs((curr-goal[axis])/goal[axis])>1e-6) || abs(curr-goal[axis])>1e-6){
				allOk=false;
				if(doUpdate) LOG_DEBUG("Strain not OK; "<<abs(curr-goal[axis])<<">1e-6");}
		}
	}
 	for (int k=0;k<3;k++) strainRate[k]=scene->cell->nextGradV(k,k);
	//Update energy input
	Real dW=(scene->cell->nextGradV*stress).trace()*scene->dt*scene->cell->getVolume();
	externalWork+=dW;
	if(scene->trackEnergy) scene->energy->add(-dW,"gradVWork",gradVWorkIx,/*non-incremental*/false);
	prevGrow=strainRate;

	if(allOk){
		if(doUpdate || currUnbalanced<0){
			currUnbalanced=DemFuncs::unbalancedForce(scene,dem,/*useMaxForce*/false);
			LOG_DEBUG("Stress/strain="<< (stressMask&1?stress(0,0):strain[0]) <<"," <<(stressMask&2?stress(1,1):strain[1])<<"," <<(stressMask&4?stress(2,2):strain[2]) <<", goal="<<goal<<", unbalanced="<<currUnbalanced );
		}
		if(currUnbalanced<maxUnbalanced){
			if (!doneHook.empty()){
				LOG_DEBUG ( "Running doneHook: "<<doneHook );
				Engine::runPy(doneHook);
			}
		}
	}
}
