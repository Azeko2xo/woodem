#include<yade/pkg/dem/OpenCLCollider.hpp>

YADE_PLUGIN(dem,(OpenCLCollider));

CREATE_LOGGER(OpenCLCollider);

#define OCLC_TIMING

#ifdef OCLC_TIMING
	#define OCLC_CHECKPOINT(cpt) timingDeltas->checkpoint(cpt)
#else
	#define OCLC_CHECKPOINT(cpt)
#endif

bool OpenCLCollider::bboxOverlapAx(int id1, int id2, int ax){
	return ((mini[ax][id1]<=maxi[ax][id2]) && (maxi[ax][id1]>=mini[ax][id2]));
}

bool OpenCLCollider::bboxOverlap(int id1, int id2){
	// compiler should unroll
	return bboxOverlapAx(id1,id2,0) && bboxOverlapAx(id1,id2,1) && bboxOverlapAx(id1,id2,2);
}


void OpenCLCollider::updateMiniMaxi(){
	size_t N=dem->particles.size();
	for(size_t id=0; id<N; id++){
		if(!dem->particles.exists(id)){ for(int ax:{0,1,2}){ mini[ax][id]=NaN; maxi[ax][id]=NaN; } continue; }
		const shared_ptr<Particle>& p=dem->particles[id];
		if(!p->shape || !p->shape->bound){ for(int ax:{0,1,2}){ mini[ax][id]=NaN; maxi[ax][id]=NaN; } continue; }
		const Bound& b=*p->shape->bound;
		for(int ax:{0,1,2}){ mini[ax][id]=b.min[ax]; maxi[ax][id]=b.max[ax]; }
	}
}

void OpenCLCollider::initialSort(){
	LOG_TRACE("Initial sort, number of bounds "<<bounds[0].size());
	// sort all arrays without looking for inversions first
	for(int ax:{0,1,2}) std::sort(bounds[ax].begin(),bounds[ax].end(),[](const AxBound& b1, const AxBound& b2) -> bool { return (isnan(b1.coord)||isnan(b2.coord))?true:b1.coord<b2.coord; } );
	// traverse one axis, always from lower bound to upper bound of the same particle
	// all intermediary bounds are candidates for collision, which must be checked in maxi/mini
	// along other two axes
	int ax0=0; // int ax1=(ax0+1)%3, ax2=(ax0+2)%3;
	for(size_t i=0; i<bounds[ax0].size(); i++){
		const AxBound& b(bounds[ax0][i]);
		if(!b.isMin || isnan(b.coord)) continue;
		LOG_TRACE("← "<<b.coord<<", #"<<b.id);
		for(size_t j=i+1; bounds[ax0][j].id!=b.id && /* just in case, e.g. maximum smaller than minimum */ j<bounds[ax0].size(); j++){
			const AxBound& b2(bounds[ax0][j]);
			if(!b2.isMin || isnan(b2.coord)) continue; // overlaps of this kind have been already checked when b2.id was processed upwards
			LOG_TRACE("\t→ "<<b2.coord<<", #"<<b2.id);
			if(!bboxOverlap(b.id,b2.id)){ LOG_TRACE("\t-- no overlap"); continue; } // when no overlap along all axes, stop
			if(dem->contacts.exists(b.id,b2.id)){ LOG_TRACE("\t-- in contact already"); continue;} // contact already there, stop
			/* create new potential contact here */
			LOG_TRACE("\t++ new contact");
			shared_ptr<Contact> c=make_shared<Contact>();
			c->pA=dem->particles[b.id]; c->pB=dem->particles[b2.id];
			dem->contacts.add(c); // single-threaded, can be thread-unsafe
		}
	}
}

void OpenCLCollider::insertionSort_inversions(vector<AxBound>& bb, vector<Vector2i>& inv){
	long iMax=bb.size();
	for(long i=0; i<iMax; i++){
		const AxBound bbInit=bb[i]; // copy, so that it is const
		if(isnan(bbInit.coord)) continue;
		long j=i-1;
		while(j<=0 && (bb[j].coord>bbInit.coord || isnan(bb[j].coord))){
			// bbInit is bb[j+1] which is travelling downwards and swaps with b[j]
			// do not store min-min, max-max, nan inversions
			if(bbInit.isMin!=bb[j].isMin && !isnan(bb[j].coord)){
				int minId=min(bbInit.id,bb[j].id), maxId=max(bbInit.id,bb[j].id);
				// min going below max
				if(bbInit.isMin) inv.push_back(Vector2i(minId,maxId));
				// max going below min
				else inv.push_back(Vector2i(maxId,minId));
			}
			bb[j+1]=bb[j];
		}
		bb[j+1]=bbInit;
	}
}



void OpenCLCollider::compareCpuGpuInversions(vector<Vector2i>(&cpuInv)[3], vector<Vector2i>(&gpuInv)[3]){
	axErr=Vector3i::Zero();
	// sort for easy comparison
	#pragma omp parallel for
	for(int ax=0; ax<3; ax++){
		for(bool cpu:{false,true}){
			vector<Vector2i>& invs=(cpu?cpuInv[ax]:gpuInv[ax]);
			std::sort(invs.begin(),invs.end(),[](const Vector2i& p1, const Vector2i& p2){ return (p1[0]<p2[0] || (p1[0]==p2[0] && p1[1]<p2[1])); });
		}
		if(memcmp(&(cpuInv[ax][0]),&(gpuInv[ax][1]),cpuInv[ax].size()*sizeof(Vector2i))!=0) axErr[ax]=1;
	}
	/* copy sorted arrays for python inspection */
	cpuInvX=cpuInv[0]; cpuInvY=cpuInv[1]; cpuInvZ=cpuInv[2];
	gpuInvX=gpuInv[0]; gpuInvY=gpuInv[1]; gpuInvZ=gpuInv[2];
	// report as exception; sorted arrays available for post-mortem analysis
	if(axErr!=Vector3i::Zero()) throw std::runtime_error(string("OpenCLCollider:: inversions along the following axes were not equal: ")+(axErr[0]!=0?"x ":"")+(axErr[1]!=0?"y ":"")+(axErr[2]!=0?"z ":""));
}

void OpenCLCollider::handleInversions(const vector<Vector2i>(&validInv)[3]){
	#pragma omp parallel for
	for(int ax=0; ax<3; ax++){
		for(const Vector2i& inv: validInv[ax]){
			/* possible separation: max going below min */
			if(inv[0]>inv[1]){
				// is there still overlap along other 2 axes?
				// then the contact should exists and is deleted if it is only potential
				if(bboxOverlapAx(inv[0],inv[1],(ax+1)%3) && bboxOverlapAx(inv[0],inv[1],(ax+2)%3)){
					shared_ptr<Contact> c=dem->contacts.find(inv[0],inv[1]);
					assert(c);
					if(!c->isReal()) dem->contacts.remove(c,/*threadSafe*/true);
				}
			} else { /* possible new overlap: min going below max */
				if(dem->contacts.exists(inv[0],inv[1])){
					// since the boxes were separate, existing contact must be actual
					assert(dem->contacts.find(inv[0],inv[1])->isReal());
					continue; 
				}
				// no contact yet, check overlap in other two dimensions
				if(!(bboxOverlapAx(inv[0],inv[1],(ax+1)%3) && bboxOverlapAx(inv[0],inv[1],(ax+2)%3))) continue;
				// there is overlap, and no contact; create new potential contact then
				shared_ptr<Contact> c=make_shared<Contact>();
				c->pA=dem->particles[inv[0]]; c->pB=dem->particles[inv[1]];
				dem->contacts.add(c,/*threadSafe*/true);
			}
		}
	}
}


void OpenCLCollider::run(){
	if(scene->isPeriodic){ throw std::runtime_error("OpenCLCollider does not support periodic boundary conditions.");	}
	#ifdef OCLC_TIMING
		if(!timingDeltas) timingDeltas=make_shared<TimingDeltas>();
		timingDeltas->start();
	#endif
	if(!cpu && !gpu) throw std::runtime_error("OpenCLCollider: Neither CPU nor GPU collision detection enabled.");
	OCLC_CHECKPOINT("aabbCheck");
	// optimizations to not run the collider at every step
	bool run=prologue_doFullRun();
	if(run) run=/*not to be short-cirtuited!*/updateBboxes_doFullRun();
	if(!run){ LOG_TRACE("Not running in this step."); return; }
	nFullRuns++;
	size_t N=dem->particles.size(); size_t oldN=mini[0].size();
	bool initialize=(N!=oldN);
	#ifdef YADE_DEBUG
		for(int ax:{0,1,2}){ assert(bounds[ax].size()==2*oldN); assert(mini[ax].size()==oldN); assert(maxi[ax].size()==oldN); }
	#endif


	/*
	if running for the first time, or after number of particles changed, initialize everything from scratch.
	This first step is algorithmically extremely expensive, see https://www.yade-dem.org/wiki/Colliders_performace	(left axis, in log scale).
	We will want to avoid this global initial sort when particles are added/deleted with a special array for ids of deleted/added particles.
	In that case, this expensive procedure will be a separate kernel, run only once.
	*/
	OCLC_CHECKPOINT("initialize");
	if(initialize){
		for(int ax:{0,1,2}){ bounds[ax].resize(2*N); mini[ax].resize(N); maxi[ax].resize(N); }
		updateMiniMaxi();
		// create bound arrays, "sorted" by particle ids
		for(size_t id=0; id<N; id++){
			for(int ax:{0,1,2}){
				bounds[ax][2*id]  ={mini[ax][id],id,true };
				bounds[ax][2*id+1]={maxi[ax][id],id,false};
			}
		}
		if(gpu) LOG_WARN("Full (intial) sorting on GPU not yet implemented, running on CPU only");
		initialSort();
		return;
	}


	// update mini & maxi arrays from particles
	OCLC_CHECKPOINT("minmaxUp");
	updateMiniMaxi();
	OCLC_CHECKPOINT("boundsUp");
	// update coordinates in pre-sorted arrays
	for(size_t n=0; n<2*N; n++){
		for(int ax:{0,1,2}){
			AxBound& b=bounds[ax][n];
			b.coord=b.isMin?mini[ax][b.id]:maxi[ax][b.id];
		}
	}

	vector<Vector2i> cpuInv[3], gpuInv[3];

	// enqueue to the GPU
	OCLC_CHECKPOINT("gpu");
	if(gpu){
		/* pass (Bound*)(bounds[0..2][0]) as read-only buffers to the collider kernel */
		/* axes 0-2 can be run out-of-order */
		throw std::runtime_error("OpenCLCollider: Inversion detection on the GPU not yet implemented.");
	}
	
	// compute on the CPU meanwhile
	OCLC_CHECKPOINT("cpu");
	if(cpu){
		/* parallelize with OpenMP */
		#pragma omp parallel for
		for(int ax=0; ax<3; ax++){
			insertionSort_inversions(bounds[ax],cpuInv[ax]); // both args modified
		}
	}

	// wait for the GPU to finish before going on
	OCLC_CHECKPOINT("gpuWait");
	if(gpu){ /* ... */ };

	OCLC_CHECKPOINT("compare");
	// compare the results if both were run; sets axErr and throws exception if there is a difference
	if(gpu&&cpu) compareCpuGpuInversions(cpuInv,gpuInv);

	OCLC_CHECKPOINT("handleInversions");
	handleInversions(cpu?cpuInv:gpuInv);
}
