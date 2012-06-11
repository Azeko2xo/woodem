#include<yade/pkg/dem/Impose.hpp>
#include<yade/pkg/dem/Particle.hpp>

struct LawTesterStage: public Serializable{
	DECLARE_LOGGER;
	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	YADE_CLASS_BASE_DOC_ATTRS(LawTesterStage,Serializable,"Stage to be reached by LawTester.",
		((Vector6r,values,Vector6r::Zero(),AttrTrait<>(),"Prescribed values during this step"))
		((Vector6i,whats,Vector6i::Zero(),AttrTrait<>(),"Meaning of *values* components"))
		((string,until,"",,"Stage finishes when *until* (python expression) evaluates to True. Besides receiving global variables, several local variables are passed: `C` (contact object; `None` if contact does not exist), `pA` (first particle), `pB` (second particle), `scene` (current scene object), `tester` (`LawTester` object), `stage` (`LawTesterStage` object)."))
		((string,done,"",,"Run this python command when the stage finishes"))
		((int,step,0,,"Step in this stage"))
		((Real,time,0,,"Time in this stage"))
	);
};
REGISTER_SERIALIZABLE(LawTesterStage);

struct LawTester: public Engine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual void run();
	py::dict pyFuv() const {
		py::dict ret;
		#define _RET_ADD(WHAT) ret[#WHAT]=py::object(WHAT);
			_RET_ADD(f);     _RET_ADD(u);     _RET_ADD(v);
			_RET_ADD(smooF); _RET_ADD(smooU); _RET_ADD(smooV);
			_RET_ADD(fErrRel);  _RET_ADD(uErrRel);  _RET_ADD(vErrRel);
			_RET_ADD(fErrAbs);  _RET_ADD(uErrAbs);  _RET_ADD(vErrAbs);
		#undef _RET_ADD
		return ret;
	}
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(LawTester,Engine,"Engine for testing contact laws by prescribing various loading scenarios, which are a combination of prescribing force or velocity along given contact-local axes.",
		((Vector2i,ids,,,"Ids of particles in contact"))
		((string,done,"tester.dead=True",,"Python expression to run once all stages had finished."))
		((Real,abWeight,1,,"Float, usually ∈〈0,1〉, determining on how are displacements/rotations distributed between particles (0 for A, 1 for B); intermediate values will apply respective part to each of them."))
		((Vector6r,f,Vector6r::Zero(),AttrTrait<>().readonly(),"Force on contact, NaN if contact is broken"))
		((Vector6r,smooF,Vector6r::Zero(),AttrTrait<>().readonly(),"Smoothed value of generalized contact forces."))
		((Vector6r,u,Vector6r::Zero(),AttrTrait<>().readonly(),"Cumulative value of contact displacement, NaN if contact is broken"))
		((Vector6r,smooU,Vector6r::Zero(),AttrTrait<>().readonly(),"Smoothed value of generalized contact displacements."))
		((Vector6r,v,Vector6r::Zero(),AttrTrait<>().readonly(),"Relative velocity on contact; NaN if the contact is broken"))
		((Vector6r,smooV,Vector6r::Zero(),AttrTrait<>().readonly(),"Smoothed value of generalized contact relative velocity."))
		((Vector6r,fErrRel,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Relative error of contact force (with respect to smoothed value)"))
		((Vector6r,fErrAbs,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Absolute error of contact force (with respect to smoothed value)"))
		((Vector6r,uErrRel,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Relative error of contact displacement (with respect to smoothed value)"))
		((Vector6r,uErrAbs,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Absolute error of contact displacement (with respect to smoothed value)"))
		((Vector6r,vErrRel,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Relative error of contact velocity (with respect to smoothed value)"))
		((Vector6r,vErrAbs,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Absolute error of contact velocity (with respect to smoothed value)"))
		((Real,smoothErr,-1,,"Smoothing factor for computing errors; if negative, set to *smooth* automatically."))
		((Real,smooth,1e-4,,"Smoothing factor for computing *smoothF*"))
		((int,stage,0,AttrTrait<>().readonly(),"Current stage to be finished"))
		((Real,stageT0,-1,AttrTrait<>().readonly(),"Time at which this stage was entered"))
		((vector<shared_ptr<LawTesterStage>>,stages,,,"Stages to be reached during the testing"))
		((int,maxStageSteps,100000,AttrTrait<>().noGui(),"Throw error if stage takes this much steps"))
		,/*ctor*/
		,/*py*/ .def("fuv",&LawTester::pyFuv,"Return python dictionary containing f,u,v,smooF,smooU,smooU; useful for plotting with `yade.plot.addData(**tester.dict())`")
	);
};
REGISTER_SERIALIZABLE(LawTester);
