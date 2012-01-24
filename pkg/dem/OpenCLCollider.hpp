#include<yade/pkg/dem/InsertionSortCollider.hpp>
#if 0
	// must detect path where the SDK is installed :/
	#include<CL/cl.hpp>
#else
	typedef double cl_double;
	typedef unsigned long cl_ulong;
	typedef unsigned short cl_ushort;
	typedef bool cl_bool;
#endif

struct AxBound{
	cl_double coord;
	cl_ulong id;
	cl_bool isMin;
};

class OpenCLCollider: public InsertionSortCollider{
	bool bboxOverlapAx(int id1, int id2, int ax);
	bool bboxOverlap(int id1, int id2);
	virtual void run();
	shared_ptr<TimingDeltas> timingDeltas;
	vector<AxBound> bounds[3];
	vector<Real> mini[3];
	vector<Real> maxi[3];
	YADE_CLASS_BASE_DOC_ATTRS(OpenCLCollider,InsertionSortCollider,"Collider which prepares data structures for an OpenCL implementation of the algorithm using defined interface. The collision (bound inversion) detection can then run on the CPU (host code) or GPU (OpenCL); both can run in the same step and the results compared.",
		((bool,cpu,true,,"Run host (CPU) code for collision detection"))
		((bool,gpu,false,,"Run OpenCL code (on the GPU presumably) for collision detection; if *cpu* is true as well, compare results"))
		((Vector3i,axErr,Vector3i::Zero(),Attr::readonly,"Show (non-zero) along which axes the CPU/GPU comparison failed."))
		((vector<Vector2i>,cpuInvX,,Attr::readonly,"Inversion found on the CPU in the last step, along X"))
		((vector<Vector2i>,cpuInvY,,Attr::readonly,"Inversion found on the CPU in the last step, along Y"))
		((vector<Vector2i>,cpuInvZ,,Attr::readonly,"Inversion found on the CPU in the last step, along Z"))
		((vector<Vector2i>,gpuInvX,,Attr::readonly,"Inversion found on the GPU in the last step, along X"))
		((vector<Vector2i>,gpuInvY,,Attr::readonly,"Inversion found on the GPU in the last step, along Y"))
		((vector<Vector2i>,gpuInvZ,,Attr::readonly,"Inversion found on the GPU in the last step, along Z"))
	);
};
REGISTER_SERIALIZABLE(OpenCLCollider);
