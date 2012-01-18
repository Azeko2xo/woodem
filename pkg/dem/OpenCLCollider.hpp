#include<yade/pkg/dem/InsertionSortCollider.hpp>
#if 0
	// must detect path where the SDK is installed :/
	#include<CL/cl.hpp>
#else
	typedef double cl_double;
	typedef unsigned long cl_ulong;
	typedef unsigned short cl_ushort;
#endif

enum {BOUND_IS_MIN=1, BOUND_NO_BB=2 };
struct AxBound{
	cl_double coord;
	cl_ulong id;
	cl_ushort flags;
};

class OpenCLCollider: public InsertionSortCollider{
	virtual void run();
	shared_ptr<TimingDeltas> timingDeltas;
	vector<vector<AxBound>> bounds;
	YADE_CLASS_BASE_DOC_ATTRS(OpenCLCollider,InsertionSortCollider,"Collider which prepares data structures for an OpenCL implementation of the algorithm using defined interface. The collision (bound inversion) detection can then run on the CPU (host code) or GPU (OpenCL); both can run in the same step and the results compared.",
		((bool,cpu,true,,"Run host (CPU) code for collision detection"))
		((bool,gpu,false,,"Run OpenCL code (on the GPU presumably) for collision detection; if *cpu* is true as well, compare results"))
	);
};
REGISTER_SERIALIZABLE(OpenCLCollider);
