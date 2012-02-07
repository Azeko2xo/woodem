#include<yade/pkg/dem/OpenCLCollider.hpp>

YADE_PLUGIN(dem,(OpenCLCollider));

CREATE_LOGGER(OpenCLCollider);

#define OCLC_TIMING

#ifdef OCLC_TIMING
	#define OCLC_CHECKPOINT(cpt) timingDeltas->checkpoint(cpt)
#else
	#define OCLC_CHECKPOINT(cpt)
#endif

/* setup OpenCL, load kernel */
void OpenCLCollider::postLoad(OpenCLCollider&){
	#ifdef YADE_OPENCL
	if(!clSrc.empty()){
		// compile the program
		string src("#include\""+clSrc+"\"\n");
		cl::Program::Sources source(1,std::make_pair(src.c_str(),src.size()));
		program=cl::Program(scene->context,source);
		try{
			program.build(std::vector<cl::Device>({scene->device}),"-I.",NULL,NULL);
		}catch(cl::Error& e){
			std::cerr<<"Error building source. Build log follows."<<std::endl;
			std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(scene->device)<<std::endl;
			throw std::runtime_error("Error compiling OpenCL code.");
		}
		clReady=true;
	} else {
		clReady=false;
	}
	#else
		if(gpu){ gpu=false; throw std::runtime_error("OpenCLCollider.gpu==True but yade was compiled without the 'opencl' fefeature."); }
	#endif
}

/* check overlap along one axis */
bool OpenCLCollider::bboxOverlapAx(int id1, int id2, int ax){
	return ((mini[ax][id1]<=maxi[ax][id2]) && (maxi[ax][id1]>=mini[ax][id2]));
}
/* check overlap along all axes */
bool OpenCLCollider::bboxOverlap(int id1, int id2){
	return bboxOverlapAx(id1,id2,0) && bboxOverlapAx(id1,id2,1) && bboxOverlapAx(id1,id2,2);
}

/* update mini and maxi arrays from actual particle positions */
void OpenCLCollider::updateMiniMaxi(vector<cl_double>(&mini)[3], vector<cl_double>(&maxi)[3]){
	size_t N=dem->particles.size();
	for(size_t id=0; id<N; id++){
		if(!dem->particles.exists(id)){ for(int ax:{0,1,2}){ mini[ax][id]=NaN; maxi[ax][id]=NaN; } continue; }
		const shared_ptr<Particle>& p=dem->particles[id];
		if(!p->shape || !p->shape->bound){ for(int ax:{0,1,2}){ mini[ax][id]=NaN; maxi[ax][id]=NaN; } continue; }
		const Bound& b=*p->shape->bound;
		for(int ax:{0,1,2}){ mini[ax][id]=b.min[ax]; maxi[ax][id]=b.max[ax]; }
	}
}

/* update coordinates in the bounds array from mini and maxi arrays */
void OpenCLCollider::updateBounds(const vector<cl_double>(&mini)[3], const vector<cl_double>(&maxi)[3], vector<AxBound>(&bounds)[3]){
	size_t N=bounds[0].size();
	for(size_t n=0; n<N; n++){
		for(int ax:{0,1,2}){
			AxBound& b=bounds[ax][n];
			b.coord=b.isMin?mini[ax][b.id]:maxi[ax][b.id];
		}
	}
}

/* find initial contact set on the CPU */
vector<Vector2i> OpenCLCollider::initSortCPU(){
	vector<Vector2i> ret;
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
			/* create new potential contact here */
			LOG_TRACE("\t++ new contact");
			ret.push_back(Vector2i(b.id,b2.id));
		}
	}
	return ret;
}

/* sort the bb array on the CPU (called for each axis separately)
   and return (unsorted) array of inversions */
vector<Vector2i> OpenCLCollider::inversionsCPU(vector<AxBound>& bb){
	vector<Vector2i> inv;
	long iMax=bb.size();
	for(long i=0; i<iMax; i++){
		const AxBound bbInit=bb[i]; // copy, so that it is const
		if(isnan(bbInit.coord)) continue;
		long j=i-1;
		while(j>=0 && (bb[j].coord>bbInit.coord || isnan(bb[j].coord))){
			// bbInit is bb[j+1] which is travelling downwards and swaps with b[j]
			// do not store min-min, max-max, nan inversions
			if(bbInit.isMin!=bb[j].isMin && !isnan(bb[j].coord)){
				int minId=min(bbInit.id,bb[j].id), maxId=max(bbInit.id,bb[j].id);
				// min going below max
				if(bbInit.isMin) inv.push_back(Vector2i(minId,maxId));
				// max going below min
				else inv.push_back(Vector2i(maxId,minId));
				LOG_TRACE("["<<maxId<<"↔ "<<minId<<"]");
			}
			bb[j+1]=bb[j];
			j--;
		}
		bb[j+1]=bbInit;
	}
	return inv;
}

#ifdef YADE_OPENCL
/* finid initial contact set on the GPU */
vector<Vector2i> OpenCLCollider::initSortGPU(){
	// run bitonic sort, then check overlap along other axes
	// return array of all overlaps
	throw std::runtime_error("OpenCLCollider::initSortGPU Not yet implemented.");
}

/* find inversions in the bb array (called for each axis separately)
   and return unsorted list of inversions.
	TODO: the 'bb' is const, since it is not copied back from GPU to the host.
	This means that GPU gets as input what the CPU did in the last step
	(it should be identical), but will break when CPU is not being run.
	Ideally, the bounds array would sit on the GPU and would be re-used between
	runs without copying it back and forth.
*/
vector<Vector2i> OpenCLCollider::inversionsGPU(const vector<AxBound>& bb){
	if(!cpu) throw std::runtime_error("Running on GPU only is not supported, since bound arrays are not copied from GPU back to the host memory.");
	throw std::runtime_error("OpenCLCollider::inversionsGPU Not yet implemented.");
	#if 0
		int invMax=1<<10; 
		int invFound;
		boundBuf=clBuffer(scene->context,CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,sizeof(axBound)*bounds[ax].size(),NULL);
		minBuf=clBuffer(scene->context,CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,sizeof(axBound)*mini[ax].size(),NULL);
		maxBuf=clBuffer(scene->context,CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,sizeof(axBound)*maxi[ax].size(),NULL);
		while(true){
			// should be allocated to invMax
			invBuf=cl::Buffer(scene->context,CL_MEM_WRITE_ONLY /*...*/);

			kernels[ax]=cl::Kernel(program,clKernel);
			kernels[ax].setArg(0,minBuf);
			kernels[ax].setArg(1,maxBuf);
			kernels[ax].setArg(2,boundBuf);
			kernels[ax].setArg(3,invBuf);
			// non-blocking ops
			scene->queue.enqueueWriteBuffer(minBuf)
			scene->queue.enqueueWriteBuffer(maxBuf)
			scene->queue.enqueueWriteBuffer(boundBuf)
			scene->queue.enqueueNDRangeKernel(kernel[ax],/*...*/)
			// enqueue reading buffers back
			// etc
			// if output did not fit
			if(invFound<=invMax) break;
			invMax=invFound;
		};
	#endif
}
#endif /* YADE_OPENCL */

void OpenCLCollider::sortAndCopyInversions(vector<Vector2i>(&cpuInv)[3], vector<Vector2i>(&gpuInv)[3]){
	#pragma omp parallel for
	for(int ax=0; ax<3; ax++){
		for(bool isCpu:{true,false}){
			auto& invs(isCpu?cpuInv[ax]:gpuInv[ax]);
			std::sort(invs.begin(),invs.end(),[](const Vector2i& p1, const Vector2i& p2)->bool{ return (p1[0]<p2[0] || (p1[0]==p2[0] && p1[1]<p2[1])); });
		}
	}
	/* copy sorted arrays for python inspection */
	cpuInvX=cpuInv[0]; cpuInvY=cpuInv[1]; cpuInvZ=cpuInv[2];
	gpuInvX=gpuInv[0]; gpuInvY=gpuInv[1]; gpuInvZ=gpuInv[2];
}

void OpenCLCollider::compareInversions(vector<Vector2i>(&cpuInv)[3], vector<Vector2i>(&gpuInv)[3]){
	axErr=Vector3i::Zero();
	// sort for easy comparison
	for(int ax=0; ax<3; ax++){
		if(cpuInv[ax].size()!=gpuInv[ax].size()){ axErr[ax]=1; continue; }
		if(memcmp(&(cpuInv[ax][0]),&(gpuInv[ax][0]),cpuInv[ax].size()*sizeof(Vector2i))!=0) axErr[ax]=2;
	}
	// report as exception; sorted arrays available for post-mortem analysis
	if(axErr!=Vector3i::Zero()) throw std::runtime_error(string("OpenCLCollider: inversions along some axes not equal (1=lengths differ, 2=only values differ): "+lexical_cast<string>(axErr)));
}

void OpenCLCollider::modifyContactsFromInversions(const vector<Vector2i>(&invs)[3]){
	#pragma omp parallel for
	for(int ax=0; ax<3; ax++){
		for(const Vector2i& inv: invs[ax]){
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
	#ifdef YADE_OPENCL
		scene->ensureCl();
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
		updateMiniMaxi(mini,maxi);
		// create initial unsorted bound arrays, ordered by particle ids
		for(size_t id=0; id<N; id++){
			for(int ax:{0,1,2}){
				bounds[ax][2*id]  ={mini[ax][id],id,true };
				bounds[ax][2*id+1]={maxi[ax][id],id,false};
			}
		}
		// collision detection
		if(cpu) cpuInit=initSortCPU();
		#ifdef YADE_OPENCL
			if(gpu) gpuInit=initSortGPU();
		#endif
		// comparison
		if(cpu&&gpu){
			if(cpuInit.size()!=gpuInit.size()) throw std::runtime_error("OpenCLCollider: initial contacts differ in length");
			for(bool isCpu: {true,false}){ auto& init(isCpu?cpuInit:gpuInit); std::sort(init.begin(),init.end(),[](const Vector2i& p1, const Vector2i& p2){ return (p1[0]<p2[0] || (p1[0]==p2[0] && p1[1]<p2[1])); }); }
			if(memcmp(&(cpuInit[0]),&(gpuInit[0]),cpuInit.size()*sizeof(Vector2i))!=0) throw std::runtime_error("OpenCLCollider: initial contacts differ in values");
		}
		// create contacts
		for(const Vector2i& ids: (gpu?gpuInit:cpuInit)){
			if(dem->contacts.exists(ids[0],ids[1])){ LOG_TRACE("##"<<ids[0]<<"+"<<ids[1]<<"exists already."); continue; } // contact already there, stop
			shared_ptr<Contact> c=make_shared<Contact>();
			c->pA=dem->particles[ids[0]]; c->pB=dem->particles[ids[1]];
			dem->contacts.add(c); // single-threaded, can be thread-unsafe
		}
		return;
	}


	// update mini & maxi arrays from particles
	OCLC_CHECKPOINT("minmaxUp");
	updateMiniMaxi(mini,maxi);
	OCLC_CHECKPOINT("boundsUp");
	// update coordinates in pre-sorted arrays
	updateBounds(mini,maxi,bounds);

	vector<Vector2i> cpuInv[3], gpuInv[3];

	OCLC_CHECKPOINT("gpuInv");
	#ifdef YADE_OPENCL
		if(gpu){ 
			// bounds will be modified by the CPU, therefore GPU must be called first and the bounds buffer must not be copied back to the host!
			for(int ax=0; ax<3; ax++){ gpuInv[ax]=inversionsGPU(bounds[ax]); }
		}
	#endif
	OCLC_CHECKPOINT("cpuInv");
	if(cpu){
		#pragma omp parallel for
		for(int ax=0; ax<3; ax++){ cpuInv[ax]=inversionsCPU(bounds[ax]/*gets modified as well*/); }
	}
	// copy sorted inversions into python-exposed arrays
	OCLC_CHECKPOINT("sortAndCopy");
	sortAndCopyInversions(cpuInv,gpuInv);
	// if cpu&&gpu, set axErr and throws exception if there is a difference
	OCLC_CHECKPOINT("compareInv");
	if(cpu&&gpu) compareInversions(cpuInv,gpuInv);

	OCLC_CHECKPOINT("handleInversions");
	modifyContactsFromInversions(cpu?cpuInv:gpuInv);
}
