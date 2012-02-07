#include<yade/pkg/dem/InsertionSortCollider.hpp>

struct AxBound{
	cl_double coord;
	cl_ulong id;
	cl_bool isMin;
};

class OpenCLCollider: public InsertionSortCollider{
	DECLARE_LOGGER;
	bool bboxOverlapAx(int id1, int id2, int ax);
	bool bboxOverlap(int id1, int id2);

	void updateMiniMaxi(vector<cl_double>(&mini)[3],vector<cl_double>(&maxi)[3]);
	void updateBounds(const vector<cl_double>(&mini)[3],const vector<cl_double>(&maxi)[3],vector<AxBound>(&bounds)[3]);

	vector<Vector2i> initSortCPU();
	vector<Vector2i> inversionsCPU(vector<AxBound>& bb);
	#ifdef YADE_OPENCL
		vector<Vector2i> initSortGPU();
		vector<Vector2i> inversionsGPU(const vector<AxBound>& bb);
	#endif
	void sortAndCopyInversions(vector<Vector2i>(&cpuInv)[3], vector<Vector2i>(&gpuInv)[3]);
	void compareInversions(vector<Vector2i>(&cpuInv)[3], vector<Vector2i>(&gpuInv)[3]);
	void modifyContactsFromInversions(const vector<Vector2i>(&invs)[3]);

	virtual void postLoad(OpenCLCollider&);

	virtual void run();
	vector<AxBound> bounds[3];
	vector<cl_double> mini[3];
	vector<cl_double> maxi[3];
	#ifdef YADE_OPENCL
		cl::Program program;
		cl::Buffer boundBufs[3], minBufs[3], maxBufs[3], invBufs[3];
		cl::Kernel kernels[3];
	#endif
	YADE_CLASS_BASE_DOC_ATTRS(OpenCLCollider,InsertionSortCollider,"Collider which prepares data structures for an OpenCL implementation of the algorithm using defined interface. The collision (bound inversion) detection can then run on the CPU (host code) or GPU (OpenCL); both can run in the same step and the results compared.",
		((bool,cpu,true,,"Run host (CPU) code for collision detection"))
		((bool,gpu,false,,"Run OpenCL code (on the GPU presumably) for collision detection; if *cpu* is true as well, compare results"))
		((bool,clReady,false,(Attr::readonly|Attr::noSave),"The OpenCL kernel is compiled and ready"))
		((string,clSrc,"",Attr::triggerPostLoad,"Filename of the OpenCL code"))
		((string,clKernel,"collide",,"Name of the kernel to call"))
		((Vector3i,axErr,Vector3i::Zero(),Attr::readonly,"Show (non-zero) along which axes the CPU/GPU comparison failed."))
		((vector<Vector2i>,cpuInit,,Attr::readonly,"Initial contacts created on the CPU"))
		((vector<Vector2i>,gpuInit,,Attr::readonly,"Initial contacts created on the GPU"))
		((vector<Vector2i>,cpuInvX,,Attr::readonly,"Inversion found on the CPU in the last step, along X"))
		((vector<Vector2i>,cpuInvY,,Attr::readonly,"Inversion found on the CPU in the last step, along Y"))
		((vector<Vector2i>,cpuInvZ,,Attr::readonly,"Inversion found on the CPU in the last step, along Z"))
		((vector<Vector2i>,gpuInvX,,Attr::readonly,"Inversion found on the GPU in the last step, along X"))
		((vector<Vector2i>,gpuInvY,,Attr::readonly,"Inversion found on the GPU in the last step, along Y"))
		((vector<Vector2i>,gpuInvZ,,Attr::readonly,"Inversion found on the GPU in the last step, along Z"))
	);
};
REGISTER_SERIALIZABLE(OpenCLCollider);
