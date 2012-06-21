#include<woo/pkg/dem/InsertionSortCollider.hpp>

class OpenCLCollider: public InsertionSortCollider{


	struct CpuAxBound{
		float coord;
		unsigned long id;
		bool isMin;
	};
	// for the GPU
	struct AxBound{
		cl_float coord;
		cl_ulong id;
		AxBound(): coord(std::numeric_limits<cl_float>::infinity()){};
	};

	DECLARE_LOGGER;
	bool bboxOverlapAx(int id1, int id2, int ax);
	bool bboxOverlap(int id1, int id2);

	void updateMiniMaxi(vector<cl_float>(&mini)[3],vector<cl_float>(&maxi)[3]);
	void updateBounds(const vector<cl_float>(&mini)[3],const vector<cl_float>(&maxi)[3],vector<CpuAxBound>(&cpuBounds)[3]);

	vector<Vector2i> initSortCPU();
	vector<Vector2i> inversionsCPU(vector<CpuAxBound>& bb);
	#ifdef WOO_OPENCL
		vector<Vector2i> initSortGPU();
		vector<Vector2i> inversionsGPU(int ax);
	#endif
	void sortAndCopyInversions(vector<Vector2i>(&cpuInv)[3], vector<Vector2i>(&gpuInv)[3]);
	// return false on error
	bool compareInversions(vector<Vector2i>(&cpuInv)[3], vector<Vector2i>(&gpuInv)[3]);
	bool checkBoundsSorted();
	void modifyContactsFromInversions(const vector<Vector2i>(&invs)[3]);

	virtual void postLoad(OpenCLCollider&);

	virtual void run();
	vector<CpuAxBound> cpuBounds[3];
	vector<cl_float> mini[3];
	vector<cl_float> maxi[3];
	#ifdef WOO_OPENCL
		cl::Program program;
		cl::Buffer boundBufs[3], minBuf[3], maxBuf[3], invBufs[3];
		cl::Kernel kernels[3];
		vector<AxBound> gpuBounds[3];
	#endif
	WOO_CLASS_BASE_DOC_ATTRS(OpenCLCollider,InsertionSortCollider,"Collider which prepares data structures for an OpenCL implementation of the algorithm using defined interface. The collision (bound inversion) detection can then run on the CPU (host code) or GPU (OpenCL); both can run in the same step and the results compared.",
		((bool,cpu,true,,"Run host (CPU) code for collision detection"))
		((bool,gpu,false,,"Run OpenCL code (on the GPU presumably) for collision detection; if *cpu* is true as well, compare results"))
		((bool,clReady,false,AttrTrait<Attr::readonly|Attr::noSave>(),"The OpenCL kernel is compiled and ready"))
		((string,clSrc,"",AttrTrait<Attr::triggerPostLoad>(),"Filename of the OpenCL code"))
		((string,clKernel,"collide",,"Name of the kernel to call"))
		((Vector3i,axErr,Vector3i::Zero(),AttrTrait<Attr::readonly>(),"Show (non-zero) along which axes the CPU/GPU comparison failed."))
		((long,numInvs,0,,"Number of inversions on the CPU"))
		((vector<Vector2i>,cpuInit,,AttrTrait<Attr::readonly>(),"Initial contacts created on the CPU"))
		((vector<Vector2i>,gpuInit,,AttrTrait<Attr::readonly>(),"Initial contacts created on the GPU"))
		((vector<Vector2i>,cpuInvX,,AttrTrait<Attr::readonly>(),"Inversion found on the CPU in the last step, along X"))
		((vector<Vector2i>,cpuInvY,,AttrTrait<Attr::readonly>(),"Inversion found on the CPU in the last step, along Y"))
		((vector<Vector2i>,cpuInvZ,,AttrTrait<Attr::readonly>(),"Inversion found on the CPU in the last step, along Z"))
		((vector<Vector2i>,gpuInvX,,AttrTrait<Attr::readonly>(),"Inversion found on the GPU in the last step, along X"))
		((vector<Vector2i>,gpuInvY,,AttrTrait<Attr::readonly>(),"Inversion found on the GPU in the last step, along Y"))
		((vector<Vector2i>,gpuInvZ,,AttrTrait<Attr::readonly>(),"Inversion found on the GPU in the last step, along Z"))
	);
};
REGISTER_SERIALIZABLE(OpenCLCollider);