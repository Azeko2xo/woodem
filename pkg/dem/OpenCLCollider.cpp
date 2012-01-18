#include<yade/pkg/dem/OpenCLCollider.hpp>

YADE_PLUGIN(dem,(OpenCLCollider));

#ifdef OCLC_TIMING
	#define OCLC_CHECKPOINT(cpt) timingDeltas->checkpoint(cpt)
#else
	#define OCLC_CHECKPOINT(cpt)
#endif


void OpenCLCollider::run(){
	if(scene->isPeriodic){ throw std::runtime_error("OpenCLCollider does not support periodic boundary conditions.");	}
	#ifdef OCLC_TIMING
		if(!timingDeltas) timingDeltas=make_shared<TimingDeltas>();
		timingDeltas->start();
	#endif
	OCLC_CHECKPOINT("aabb");
	bool run=prologue_doFullRun() || updateBboxes_doFullRun();
	if(!run) return;
	OCLC_CHECKPOINT("bounds");
	nFullRuns++;
	size_t N=dem->particles.size();
	if(bounds.size()!=3 || bounds[0].size()!=2*N){
		// initial run
		bounds.resize(3);
		for(int ax:{0,1,2}){
			vector<AxBound>& bb(bounds[ax]);
			bb.resize(2*N);
			for(size_t id=0; id<N; id++){
				AxBound& bMin(bb[2*id]); AxBound& bMax(bb[2*id+1]);
				bMin.id=bMax.id=id;
				if(!dem->particles.exists(id) || !(dem->particles[id]->shape) || !(dem->particles[id]->shape->bound)){
					bMin.flags=BOUND_IS_MIN&BOUND_NO_BB; bMax.flags=BOUND_NO_BB;
					bMin.coord=bMax.coord=0.;
					continue;
				}
				bMin.flags=BOUND_IS_MIN; bMax.flags=0;
				const shared_ptr<Bound>& partBound(dem->particles[id]->shape->bound);
				bMin.coord=partBound->min[ax]; bMax.coord=partBound->max[ax];
			}
		}
	} else {
		// update bounds
		for(int ax:{0,1,2}){
			vector<AxBound>& bb(bounds[ax]);
			assert(bb.size()==2*N);
			for(AxBound& b: bb){
				if(!dem->particles.exists(b.id)){ b.flags|=BOUND_NO_BB; continue; }
				const shared_ptr<Shape> shape=dem->particles[b.id]->shape;
				if(!shape || !shape->bound){ b.flags|=BOUND_NO_BB; continue; }
				b.coord=(b.flags&BOUND_IS_MIN)?shape->bound->min[ax]:shape->bound->max[ax];
				b.flags&=~BOUND_NO_BB; // bb is there
			}
		}
	}
	// data ready now

	// enqueue to the GPU
	OCLC_CHECKPOINT("gpu");
	if(gpu){
		/* pass (Bound*)(bounds[0..2][0]) as read-only buffers to the collider kernel */
		/* axes 0-2 can be run out-of order */
	}

	// compute on the CPU meanwhile
	OCLC_CHECKPOINT("cpu");
	if(cpu){ /* parallelize with OpenMP */ }

	// wait for the GPU to finish before going on
	OCLC_CHECKPOINT("gpu-wait");
	if(gpu){ /* ... */ };

	// compare the results if both were run
	if(gpu&&cpu){ /* ... */ }
}
