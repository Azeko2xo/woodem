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
		scene->ensureCl();
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
void OpenCLCollider::updateMiniMaxi(vector<cl_float>(&mini)[3], vector<cl_float>(&maxi)[3]){
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
void OpenCLCollider::updateBounds(const vector<cl_float>(&mini)[3], const vector<cl_float>(&maxi)[3], vector<CpuAxBound>(&bounds)[3]){
	size_t N=bounds[0].size();
	for(size_t n=0; n<N; n++){
		for(int ax:{0,1,2}){
			CpuAxBound& b=bounds[ax][n];
			b.coord=b.isMin?mini[ax][b.id]:maxi[ax][b.id];
		}
	}
}

/* find initial contact set on the CPU */
vector<Vector2i> OpenCLCollider::initSortCPU(){
	vector<Vector2i> ret;
	LOG_TRACE("Initial sort, number of bounds "<<bounds[0].size());
	// sort all arrays without looking for inversions first
	for(int ax:{0,1,2}) std::sort(bounds[ax].begin(),bounds[ax].end(),[](const CpuAxBound& b1, const CpuAxBound& b2) -> bool { return (isnan(b1.coord)||isnan(b2.coord))?true:b1.coord<b2.coord; } );
	// traverse one axis, always from lower bound to upper bound of the same particle
	// all intermediary bounds are candidates for collision, which must be checked in maxi/mini
	// along other two axes
	int ax0=0; // int ax1=(ax0+1)%3, ax2=(ax0+2)%3;
	for(size_t i=0; i<bounds[ax0].size(); i++){
		const CpuAxBound& b(bounds[ax0][i]);
		if(!b.isMin || isnan(b.coord)) continue;
		LOG_TRACE("← "<<b.coord<<", #"<<b.id);
		for(size_t j=i+1; bounds[ax0][j].id!=b.id && /* just in case, e.g. maximum smaller than minimum */ j<bounds[ax0].size(); j++){
			const CpuAxBound& b2(bounds[ax0][j]);
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
vector<Vector2i> OpenCLCollider::inversionsCPU(vector<CpuAxBound>& bb){
	vector<Vector2i> inv;
	long iMax=bb.size();
	LOG_DEBUG("INVERSIONS_CPU");

	for(long i=0; i<iMax; i++){
	//	LOG_DEBUG(bb[i].coord);
		const CpuAxBound bbInit=bb[i]; // copy, so that it is const
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
	cl::Context& context(scene->context);
	cl::CommandQueue queue(scene->queue);
	// throw std::runtime_error("OpenCLCollider::initSortGPU Not yet implemented.");
	int N=dem->particles.size();

	//
	for(int i:{0,1,2}) gpuBounds[i].resize(2*N);

	//dimension for ocl kernel 
	const int local_size=16;
	int global_size=(trunc(trunc(sqrt(N)) / local_size) + 1) * local_size;

	//power of two for B-sort
	int powerOfTwo = (pow(2, trunc(log2(2*N))) == 2*N) ?
		pow(2, trunc(log2(2*N))) : pow(2, trunc(log2(2*N)) + 1);

	cl_uint bits = 0;
	for (int temp = powerOfTwo; temp > 1; temp >>= 1) {
		++bits;
	}

	int less = powerOfTwo - 2*N;
	cerr << "=======================" << endl;
	cerr << "powerOfTwo : " << powerOfTwo << endl;
	cerr << "less : " << less << endl;
	cerr << "=======================" << endl;

	//gData1-3
	for(int ax:{0,1,2}){
		minBuf[ax]=cl::Buffer(context,CL_MEM_READ_ONLY,N*sizeof(cl_float),NULL);
		maxBuf[ax]=cl::Buffer(context,CL_MEM_READ_ONLY,N*sizeof(cl_float),NULL);
		boundBufs[ax]=cl::Buffer(context,CL_MEM_READ_WRITE,N*2*sizeof(AxBound), NULL);
	}

   try {
		cl::Kernel k1(program,"createAxBound");
		for(int ax:{0,1,2}){
			queue.enqueueWriteBuffer(minBuf[ax],CL_TRUE,0,N*sizeof(cl_float),mini[ax].data());
			LOG_DEBUG("E");
			queue.enqueueWriteBuffer(maxBuf[ax],CL_TRUE,0,N*sizeof(cl_float),maxi[ax].data());
			LOG_DEBUG("F");
			k1.setArg(0,minBuf[ax]);
			LOG_DEBUG("A");
			k1.setArg(1,maxBuf[ax]);
			LOG_DEBUG("B");
			k1.setArg(2,boundBufs[ax]);
			LOG_DEBUG("C");
			k1.setArg(3,N);
			LOG_DEBUG("D");
			queue.enqueueNDRangeKernel(k1,cl::NullRange,cl::NDRange(global_size * global_size),
					cl::NDRange(local_size));
			LOG_DEBUG("G");
		}
	}
	catch(cl::Error& e){
		LOG_FATAL(e.what());
		throw;
	}


	cerr<<"axBound ok"<<endl;

	
	for(int ax:{0,1,2}){
    queue.enqueueReadBuffer(boundBufs[ax], CL_TRUE, 0, 2*N * sizeof (AxBound), gpuBounds[ax].data());
	LOG_DEBUG("H");
	//for (int i = 0; i < 2*N; i++){
	//	LOG_DEBUG(gpuBounds[0][i].coord);
	//}

	// bitonic sort needs a power-of-two array; fill with infinities
	boundBufs[ax]=cl::Buffer(context, CL_MEM_READ_WRITE, powerOfTwo * sizeof (AxBound), NULL);
	gpuBounds[ax].resize(powerOfTwo);
	queue.enqueueWriteBuffer(boundBufs[ax],CL_TRUE, 0, powerOfTwo * sizeof (AxBound), gpuBounds[ax].data());
	LOG_DEBUG("I");
    queue.enqueueReadBuffer(boundBufs[ax], CL_TRUE, 0, powerOfTwo * sizeof (AxBound), gpuBounds[ax].data());

	//for (int i = 0; i < powerOfTwo; i++){
	//	LOG_DEBUG(gpuBounds[0][i].coord);
	//}

	global_size=(trunc(trunc(sqrt(powerOfTwo / 2)) / local_size) + 1) * local_size;

	LOG_DEBUG(global_size);
	try {
		cl::Kernel k2(program, "sortBitonic");
		LOG_DEBUG("J");
		k2.setArg(3, powerOfTwo);
		LOG_DEBUG("K");
		k2.setArg(4, 1);
		LOG_DEBUG("L");
		k2.setArg(0, boundBufs[ax]);
		LOG_DEBUG("M");
	
		cl::Event eve;
		for (cl_uint stage = 0; stage < bits; stage++) {
			k2.setArg(1, stage);
			for (cl_uint passOfStage = 0; passOfStage < stage + 1; passOfStage++) {
				k2.setArg(2, passOfStage);
				queue.enqueueNDRangeKernel(k2, cl::NullRange,
					cl::NDRange(global_size, global_size),
					cl::NDRange(local_size, local_size), NULL, &eve);
				//queue.finish();
				eve.wait();
			}
		}

		// shrink the buffer back, read just the part we need
		gpuBounds[ax].resize(2*N);
		queue.enqueueReadBuffer(boundBufs[ax], CL_TRUE, 0, 2*N*sizeof (AxBound), gpuBounds[ax].data());
		boundBufs[ax]=cl::Buffer(context,CL_MEM_READ_WRITE,2*N*sizeof(AxBound), NULL);
		queue.enqueueWriteBuffer(boundBufs[ax],CL_TRUE, 0, 2*N * sizeof (AxBound), gpuBounds[ax].data());
	
		LOG_DEBUG("N");
	} catch (cl::Error& e) {
		LOG_FATAL(e.what()); throw;
	}
}
	//end of sort
	//have sorted array AxBound
	cerr << "** Array AxBound was sorted.\n";

	for(int i = 0; i < 2*N-1; i++){
		if(gpuBounds[0][i].coord > gpuBounds[0][i+1].coord){
			LOG_DEBUG("FAIL\n");
		}
//		LOG_DEBUG(gpuBounds[0][i].coord);
//		cerr << "test: \n";
	}

	cl_uint *counter;
	counter = new cl_uint[1];
	counter[0] = 0;
	//create overlay array
	int overAlocMem = 2*N;


	cl::Buffer gCounter = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (cl_uint), NULL);
	cl::Buffer gMemCheck = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (cl_uint), NULL);


	cl_uint *test;
	test = new cl_uint[1];
	test[0] = 0;

	cl_uint *memCheck;
	memCheck = new cl_uint[1];

	vector<cl_uint2> overlay;
	cl::Buffer gOverlay;


	for(int i:{0,1,2}){
		boundBufs[i]=cl::Buffer(context, CL_MEM_READ_WRITE, 2*N * sizeof (AxBound), NULL);
		LOG_DEBUG("O");
		queue.enqueueWriteBuffer(boundBufs[i], CL_TRUE, 0, 2*N * sizeof (AxBound), gpuBounds[i].data());
		LOG_DEBUG("P.1");
	}
	global_size = (trunc(trunc(sqrt(2*N)) / local_size) + 1) * local_size;
	try {
		cl::Kernel createOverlayK(program, "createOverlay");
		LOG_DEBUG("Q");
		createOverlayK.setArg(0, boundBufs[0]);
		LOG_DEBUG("R");
		createOverlayK.setArg(3, 2*N);
		LOG_DEBUG("S");
		for(int ax:{1,2}){
			queue.enqueueWriteBuffer(minBuf[ax], CL_TRUE, 0, N * sizeof (cl_float), mini[ax].data());
			LOG_DEBUG("T");
			queue.enqueueWriteBuffer(maxBuf[ax], CL_TRUE, 0, N * sizeof (cl_float), maxi[ax].data());
			LOG_DEBUG("U");
		}

		createOverlayK.setArg(6, minBuf[1]);
		LOG_DEBUG("V");
		createOverlayK.setArg(7, maxBuf[1]);
		LOG_DEBUG("W");
		createOverlayK.setArg(8, minBuf[2]);
		LOG_DEBUG("X");
		createOverlayK.setArg(9, maxBuf[2]);
		LOG_DEBUG("Y");

		memCheck[0] = 0;

		gOverlay = cl::Buffer(context, CL_MEM_WRITE_ONLY, overAlocMem * sizeof (cl_uint2), NULL);

		queue.enqueueWriteBuffer(gMemCheck, CL_TRUE, 0, sizeof (cl_uint), memCheck);
		LOG_DEBUG("Z");
		queue.enqueueWriteBuffer(gCounter, CL_TRUE, 0, sizeof (cl_uint), test);
		LOG_DEBUG("1");
		createOverlayK.setArg(1, gOverlay);
		LOG_DEBUG("2");
		createOverlayK.setArg(2, gCounter);
		LOG_DEBUG("3");
		createOverlayK.setArg(4, overAlocMem);
		LOG_DEBUG("4");
		createOverlayK.setArg(5, gMemCheck);
		LOG_DEBUG("5.1");
	
		cl::Event eve;
		queue.enqueueNDRangeKernel(createOverlayK, cl::NullRange,
				cl::NDRange(global_size, global_size),
				cl::NDRange(local_size, local_size), NULL, &eve);

		LOG_DEBUG("5.3");
		eve.wait();
		LOG_DEBUG("5.5");
		queue.finish();
		LOG_DEBUG("5.9");
		queue.enqueueReadBuffer(gMemCheck, CL_TRUE, 0, sizeof (cl_uint), memCheck);
		LOG_DEBUG("6");
		queue.enqueueReadBuffer(gCounter, CL_TRUE, 0, sizeof (cl_uint), counter);
		LOG_DEBUG("7");

		if (memCheck[0] == 1) {
			memCheck[0] = 0;
			cerr << "** Realokace pole na : " << counter[0] + 1 << endl;
			overAlocMem = counter[0] + 1;

			gOverlay = cl::Buffer(context, CL_MEM_WRITE_ONLY, overAlocMem * sizeof (cl_uint2), NULL);

			queue.enqueueWriteBuffer(gMemCheck, CL_TRUE, 0, sizeof (cl_uint), memCheck);
		LOG_DEBUG("7");
		LOG_DEBUG(test[0]);
			queue.enqueueWriteBuffer(gCounter, CL_TRUE, 0, sizeof (cl_uint), test);
		LOG_DEBUG("8");
			createOverlayK.setArg(1, gOverlay);
		LOG_DEBUG("9");
			createOverlayK.setArg(2, gCounter);
		LOG_DEBUG("!");
			createOverlayK.setArg(4, overAlocMem);
		LOG_DEBUG("@");
			createOverlayK.setArg(5, gMemCheck);
		LOG_DEBUG("#");

			queue.enqueueNDRangeKernel(createOverlayK, cl::NullRange,
					cl::NDRange(global_size, global_size),
					cl::NDRange(local_size, local_size));

			queue.enqueueReadBuffer(gMemCheck, CL_TRUE, 0, 1 * sizeof (cl_uint), memCheck);
		LOG_DEBUG("$");
			queue.enqueueReadBuffer(gCounter, CL_TRUE, 0, sizeof (cl_uint), counter);
		LOG_DEBUG("%");
		}

		if (memCheck[0] == 1) {
			LOG_FATAL("Allocated memory insufficient.");
			abort();
		}

		overlay.resize(overAlocMem);
		queue.enqueueReadBuffer(gOverlay, CL_TRUE, 0, overAlocMem * sizeof (cl_uint2), overlay.data());
		LOG_DEBUG("^");


	} catch (cl::Error& e) {
		cerr << "err: " << e.err() << endl << "what: " << e.what() << endl;
	}

	std::vector<cl_uint2> overlay1;
	overlay1.resize(counter[0]);
	LOG_DEBUG("R-start");
	LOG_DEBUG(counter[0]);
	LOG_DEBUG(overAlocMem);
	queue.enqueueReadBuffer(gOverlay, CL_TRUE, 0, counter[0] * sizeof (cl_uint2), overlay1.data());
	LOG_DEBUG("R-end");
	std::sort(overlay1.begin(), overlay1.end(), [](const cl_uint2& a, const cl_uint2 & b)->bool {
		return a.lo < b.lo || (a.lo == b.lo && a.hi < b.hi);
	});

	cerr << "pocet preryvu : " << counter[0] << endl;
	cout << counter[0] << endl;
	vector<Vector2i> ret; ret.reserve(counter[0]);
	for (const cl_uint2& o : overlay1){
		ret.push_back(Vector2i(o.lo,o.hi));
		//cout<<o.lo<<" "<<o.hi<<endl;
	}
	return ret;
}

/* find inversions in the bb array (called for each axis separately)
   and return unsorted list of inversions.
	TODO: the 'bb' is const, since it is not copied back from GPU to the host.
	This means that GPU gets as input what the CPU did in the last step
	(it should be identical), but will break when CPU is not being run.
	Ideally, the bounds array would sit on the GPU and would be re-used between
	runs without copying it back and forth.
*/
vector<Vector2i> OpenCLCollider::inversionsGPU(int ax){
	if(!cpu) throw std::runtime_error("Running on GPU only is not supported, since bound arrays are not copied from GPU back to the host memory.");
	//throw std::runtime_error("OpenCLCollider::inversionsGPU Not yet implemented.");

	vector<Vector2i> inv;

	int N = gpuBounds[ax].size();		
	LOG_DEBUG("N size");
	LOG_DEBUG(N);
	LOG_DEBUG("mini size");
	LOG_DEBUG(mini[ax].size());
	
	int local_size = 16;
	int global_size = (trunc(trunc(sqrt(N)) / local_size) + 1) * local_size;

	cl::Buffer boundsG(scene->context, CL_MEM_READ_WRITE, N * sizeof (AxBound), NULL);
	cl::Buffer miniG(scene->context, CL_MEM_READ_ONLY, N/2 * sizeof (AxBound), NULL);
	cl::Buffer maxiG(scene->context, CL_MEM_READ_ONLY, N/2 * sizeof (AxBound), NULL);
	LOG_DEBUG("A");
	scene->queue.enqueueWriteBuffer(boundsG, CL_TRUE, 0, N * sizeof (AxBound), gpuBounds[ax].data());
	LOG_DEBUG("B");
	scene->queue.enqueueWriteBuffer(miniG, CL_TRUE, 0, N/2 * sizeof (AxBound), mini[ax].data());
	LOG_DEBUG("C");
	scene->queue.enqueueWriteBuffer(maxiG, CL_TRUE, 0, N/2 * sizeof (AxBound), maxi[ax].data());

	try {
 		cl::Kernel updateBoundK(program, "boundsUpdate");
		updateBoundK.setArg(0, boundsG);
		updateBoundK.setArg(1, miniG);
		updateBoundK.setArg(2, maxiG);
		updateBoundK.setArg(3, N);

		LOG_DEBUG("D");

		scene->queue.enqueueNDRangeKernel(updateBoundK, cl::NullRange,
					cl::NDRange(global_size, global_size),
					cl::NDRange(local_size, local_size));
		scene->queue.finish();
		LOG_DEBUG("E");
	} catch(cl::Error& e) {
		cerr << "err: " << e.err() << endl << "what: " << e.what() << endl;
	}
	cerr << "OK" << endl;
	
	scene->queue.enqueueReadBuffer(boundsG, CL_TRUE, 0, N * sizeof(AxBound), gpuBounds[ax].data());
	LOG_DEBUG("F");
	LOG_DEBUG(ax);
	
	global_size = (trunc(trunc(sqrt(N/2)) / local_size) + 1) * local_size;
 	
	LOG_DEBUG("G");
	cl::Buffer gpuNoOfInv(scene->context, CL_MEM_READ_WRITE, sizeof (cl_uint), NULL);
	cl::Buffer gpuDone(scene->context, CL_MEM_READ_WRITE, sizeof (cl_uint), NULL);
	LOG_DEBUG("H");
	cl_uint noOfInv = 0;
	cl_uint done = 1;
	cl_uint off = 1;
	cl_uint zero = 0;
	LOG_DEBUG("I");
	scene->queue.enqueueWriteBuffer(gpuNoOfInv, CL_TRUE, 0, sizeof (cl_uint), &noOfInv);
	LOG_DEBUG("J");
	LOG_DEBUG(global_size);
	LOG_DEBUG(N);
	/*compute No. of inversions for alloc mem*/
	try {
		cl::Kernel computeNoOfInvK(program, "computeNoOfInv");
		computeNoOfInvK.setArg(0, boundsG);
		computeNoOfInvK.setArg(1, gpuNoOfInv);
		computeNoOfInvK.setArg(4, N);
	LOG_DEBUG("K");
		while (done == 1){

			if(off == 0){
				off = 1;
			} else {
				off = 0;
			}

	LOG_DEBUG("L");
			scene->queue.enqueueWriteBuffer(gpuDone, CL_TRUE, 0, sizeof (cl_uint), &zero);
			computeNoOfInvK.setArg(2, gpuDone);
			computeNoOfInvK.setArg(3, off);
	LOG_DEBUG("M");
			scene->queue.enqueueNDRangeKernel(computeNoOfInvK, cl::NullRange,
					cl::NDRange(global_size, global_size),
					cl::NDRange(local_size, local_size));
			scene->queue.finish();
	LOG_DEBUG("N");
			scene->queue.enqueueReadBuffer(gpuDone, CL_TRUE, 0, sizeof (cl_uint), &done);	
		}
	
	} catch(cl::Error& e) {
		cerr << "err: " << e.err() << endl << "what: " << e.what() << endl;	
	}
		LOG_DEBUG("O");
	scene->queue.enqueueReadBuffer(gpuNoOfInv, CL_TRUE, 0, sizeof(cl_uint), &noOfInv);
		LOG_DEBUG("P");
	LOG_DEBUG("No. of inversions");
	LOG_DEBUG(noOfInv);

	//inv.resize(noOfInv);
	cl_uint2 *inversions =  new cl_uint2[noOfInv];
	cl::Buffer gpuInversion(scene->context, CL_MEM_WRITE_ONLY, noOfInv * sizeof(cl_uint2), NULL);

	/*compute inversion*/
	scene->queue.enqueueWriteBuffer(gpuNoOfInv, CL_TRUE, 0, sizeof (cl_uint), &zero);
	scene->queue.enqueueWriteBuffer(boundsG, CL_TRUE, 0, N * sizeof (AxBound), gpuBounds[ax].data());
	off = 1;
	done = 1;
	try {
		cl::Kernel computeInvK(program, "computeInv");
		computeInvK.setArg(0, boundsG);
		LOG_DEBUG("Q");
		computeInvK.setArg(1, gpuNoOfInv);
		LOG_DEBUG("R");
		computeInvK.setArg(5, N);
		LOG_DEBUG("S");
		computeInvK.setArg(3, gpuInversion); 
		LOG_DEBUG("T");
		while (done == 1){

			if(off == 0){
				off = 1;
			} else {
				off = 0;
			}

	LOG_DEBUG("L");
			scene->queue.enqueueWriteBuffer(gpuDone, CL_TRUE, 0, sizeof (cl_uint), &zero);
			computeInvK.setArg(2, gpuDone);
			computeInvK.setArg(4, off);
	LOG_DEBUG("M");
			scene->queue.enqueueNDRangeKernel(computeInvK, cl::NullRange,
					cl::NDRange(global_size, global_size),
					cl::NDRange(local_size, local_size));
			scene->queue.finish();
	LOG_DEBUG("N");
			scene->queue.enqueueReadBuffer(gpuDone, CL_TRUE, 0, sizeof (cl_uint), &done);	
		}
	
	} catch(cl::Error& e) {
		cerr << "err: " << e.err() << endl << "what: " << e.what() << endl;	
	}

	scene->queue.enqueueReadBuffer(gpuInversion, CL_TRUE, 0, noOfInv * sizeof (cl_uint2), inversions);	

	for(uint i = 0; i < noOfInv; i++){
	//	cerr << inversions[i].lo << " x " << inversions[i].hi << endl;
		inv.push_back(Vector2i(inversions[i].lo, inversions[i].hi));
	}
	LOG_DEBUG(inv.size());
	return inv;

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
		for(auto invs:{&cpuInv[ax],&gpuInv[ax]}){
			std::sort(invs->begin(),invs->end(),[](const Vector2i& p1, const Vector2i& p2)->bool{ return (p1[0]<p2[0] || (p1[0]==p2[0] && p1[1]<p2[1])); });
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
		if(cpuInv[ax].size()!=gpuInv[ax].size()){ axErr[ax]=cpuInv[ax].size(); continue; }
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
					//assert(c);
					if(!c) continue;
					if(!c->isReal()) dem->contacts.remove(c,/*threadSafe*/true);
				}
			} else { /* possible new overlap: min going below max */
				if(dem->contacts.exists(inv[0],inv[1])){
					// since the boxes were separate, existing contact must be actual
					//assert(dem->contacts.find(inv[0],inv[1])->isReal());
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
		OCLC_CHECKPOINT("aabb");
		if(cpu) cpuInit=initSortCPU();
		OCLC_CHECKPOINT("initCPU");
		#ifdef YADE_OPENCL
			if(gpu) gpuInit=initSortGPU();
		#endif
		OCLC_CHECKPOINT("initGPU");
		// comparison
		if(cpu&&gpu){
			if(cpuInit.size()!=gpuInit.size()) throw std::runtime_error("OpenCLCollider: initial contacts differ in length");
			for(auto init: {&cpuInit,&gpuInit}) std::sort(init->begin(),init->end(),[](const Vector2i& p1, const Vector2i& p2){ return (p1[0]<p2[0] || (p1[0]==p2[0] && p1[1]<p2[1])); });
			if(memcmp(&(cpuInit[0]),&(gpuInit[0]),cpuInit.size()*sizeof(Vector2i))!=0) throw std::runtime_error("OpenCLCollider: initial contacts differ in values");
		}
		// create contacts
		for(const Vector2i& ids: (gpu?gpuInit:cpuInit)){
			if(dem->contacts.exists(ids[0],ids[1])){ LOG_TRACE("##"<<ids[0]<<"+"<<ids[1]<<"exists already."); continue; } // contact already there, stop
			shared_ptr<Contact> c=make_shared<Contact>();
			c->pA=dem->particles[ids[0]]; c->pB=dem->particles[ids[1]];
			dem->contacts.add(c); // single-threaded, can be thread-unsafe
		}
		OCLC_CHECKPOINT("initCompare");
		return;
	} else {
		// checkpoints must always follow in the same sequence
		OCLC_CHECKPOINT("aabb"); OCLC_CHECKPOINT("initCPU"); OCLC_CHECKPOINT("initGPU"); OCLC_CHECKPOINT("initCompare");
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
	LOG_DEBUG("INVERSION");
		if(gpu){ 
			// bounds will be modified by the CPU, therefore GPU must be called first and the bounds buffer must not be copied back to the host!
			LOG_DEBUG("START INVERSION");
			for(int ax=0; ax<3; ax++){ gpuInv[ax]=inversionsGPU(ax); }
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
