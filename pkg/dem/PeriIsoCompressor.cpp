
// 2009 © Václav Šmilauer <eudoxos@arcig.cz>

#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/PeriIsoCompressor.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Funcs.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/lib/pyutil/gil.hpp>

CREATE_LOGGER(PeriIsoCompressor);

YADE_PLUGIN(dem,(PeriIsoCompressor))

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
	if(((step%globalUpdateInt)==0) || avgStiffness<0 || sigma[0]<0 || sigma[1]<0 || sigma[2]<0){
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
		if(avgGrow<okGrow) throw runtime_error("Unable to shring cell due to maximum body size (although required by stress condition). Increase particle rigidity, increase total sample dimensions, or decrease goal stress.");
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
				if(!doneHook.empty()){ LOG_DEBUG("Running doneHook: "<<doneHook); runPy(doneHook); }
			} else { LOG_INFO("Loaded to "<<sigmaGoal<<" done, going to "<<stresses[state]<<" now"); }
		} else {
			if((step%globalUpdateInt)==0) LOG_DEBUG("Stress="<<sigma<<", goal="<<sigmaGoal<<", unbalanced="<<currUnbalanced);
		}
	}
}

