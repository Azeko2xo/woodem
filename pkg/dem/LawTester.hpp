#pragma once

#include<woo/pkg/dem/Impose.hpp>
#include<woo/pkg/dem/Particle.hpp>

struct LawTesterStage: public Object{
	DECLARE_LOGGER;
	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	bool pyBroken() const { return hadC && !hasC; }
	bool pyRebound() const { return bounces>=2 || pyBroken(); }
	Real pyCTime() const { return time-timeC0; }
	void reset();
	#define woo_dem_LawTesterStage__CLASS_BASE_DOC_ATTRS_PY \
		LawTesterStage,Object,"Stage to be reached by LawTester.", \
		((Vector6r,values,Vector6r::Zero(),AttrTrait<>(),"Prescribed values during this step")) \
		((Vector6i,whats,Vector6i::Zero(),AttrTrait<>(),"Meaning of *values* components")) \
		((string,until,"",,"Stage finishes when *until* (python expression) evaluates to True. Besides receiving global variables, several local variables are passed: `C` (contact object; `None` if contact does not exist), `pA` (first particle), `pB` (second particle), `scene` (current scene object), `tester` (`LawTester` object), `stage` (`LawTesterStage` object).")) \
		((int,untilEvery,1,,"Test the :obj:`until` expression only every *untilEvery* steps (this may make the execution faster)")) \
		((string,done,"",,"Run this python command when the stage finishes")) \
		((int,step,0,,"Step in this stage")) \
		((Real,time,0,,"Time in this stage")) \
		((bool,hadC,false,,"Flag keeping track of whether there was a contact in this stage at all")) \
		((bool,hasC,false,,"Flag keeping track of whether there was a contact in this stage at all")) \
		((Real,timeC0,NaN,,"Time of creating of the last contact (NaN if there has never been one).")) \
		((int,bounces,0,,"Number of sign changes of the normal relative velocity in this stage")) \
		/* ; to teest for a complete rebound, use ``until='stage.bounces>1 or stage.broken'``.") */ \
		,/*py*/.add_property("broken",&LawTesterStage::pyBroken,"Test whether an existing contact broke in this stage; this is useful for saying ``until='stage.broken'`` (equivalent to ``stage.hadC and not stage.hasC``). This is different from ``until='not C'``, since this condition will be satisfied before any contact exists at all.") \
		.add_property("rebound",&LawTesterStage::pyRebound,"Test for rebound; rebound is considered complete when sign of relative normal velocity changed more than once (adhesive contacts may never separate once they are created -- this catches a single period of the oscillation) or if contact :obj:`breaks <broken>`. Equivalent to ``stage.bounces>=2 or stage.broken``.") \
		.def("reset",&LawTesterStage::reset,"Reset this stage to its initial stage such that it can be used again as if new. This is called automatically from :obj:`LawTester.restart`.") \
		.add_property("cTime",&LawTesterStage::pyCTime,"Time since creation of the last contact (NaN if there has never been one). Useful for testing collision time after the condition ``until='stage.rebound'`` has been satisfied. Equivalent to ``stage.time-stage.timeC0``.") \
		; \
		woo::converters_cxxVector_pyList_2way<shared_ptr<LawTesterStage>>();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_LawTesterStage__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(LawTesterStage);

struct LawTester: public Engine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual void run();
	void restart();
	py::dict pyFuv() const {
		py::dict ret;
		#define _RET_ADD(WHAT) ret[#WHAT]=py::object(WHAT);
			_RET_ADD(f);     _RET_ADD(u);     _RET_ADD(v); _RET_ADD(k);
			_RET_ADD(smooF); _RET_ADD(smooU); _RET_ADD(smooV);
			_RET_ADD(fErrRel);  _RET_ADD(uErrRel);  _RET_ADD(vErrRel);
			_RET_ADD(fErrAbs);  _RET_ADD(uErrAbs);  _RET_ADD(vErrAbs);
		#undef _RET_ADD
		return ret;
	}
	#define woo_dem_LawTester__CLASS_BASE_DOC_ATTRS_PY \
		LawTester,Engine,"Engine for testing contact laws by prescribing various loading scenarios, which are a combination of prescribing force or velocity along given contact-local axes.", \
		((Vector2i,ids,,,"Ids of particles in contact")) \
		((string,done,"tester.dead=True",,"Python expression to run once all stages had finished. This is run *after* :obj:`LawTesterStage.done` of the last stage.")) \
		((Real,abWeight,1,,"Float, usually ∈〈0,1〉, determining on how are displacements/rotations distributed between particles (0 for A, 1 for B); intermediate values will apply respective part to each of them.")) \
		((Vector6r,f,Vector6r::Zero(),AttrTrait<>().readonly(),"Force on contact, NaN if contact is broken")) \
		((Vector6r,k,Vector6r::Zero(),AttrTrait<>().readonly(),"Tangent contact stiffness, NaN if there is no contact (or the contact model does not define it). Diagonal of the K matrix in df=Kdu.")) \
		((Vector6r,smooF,Vector6r::Zero(),AttrTrait<>().readonly(),"Smoothed value of generalized contact forces.")) \
		((Vector6r,u,Vector6r::Zero(),AttrTrait<>().readonly(),"Cumulative value of contact displacement, NaN if contact is broken")) \
		((Vector6r,smooU,Vector6r::Zero(),AttrTrait<>().readonly(),"Smoothed value of generalized contact displacements.")) \
		((Vector6r,v,Vector6r::Zero(),AttrTrait<>().readonly(),"Relative velocity on contact; NaN if the contact is broken")) \
		((Vector6r,smooV,Vector6r::Zero(),AttrTrait<>().readonly(),"Smoothed value of generalized contact relative velocity.")) \
		((Vector6r,fErrRel,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Relative error of contact force (with respect to smoothed value)")) \
		((Vector6r,fErrAbs,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Absolute error of contact force (with respect to smoothed value)")) \
		((Vector6r,uErrRel,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Relative error of contact displacement (with respect to smoothed value)")) \
		((Vector6r,uErrAbs,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Absolute error of contact displacement (with respect to smoothed value)")) \
		((Vector6r,vErrRel,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Relative error of contact velocity (with respect to smoothed value)")) \
		((Vector6r,vErrAbs,Vector6r::Constant(Inf),AttrTrait<>().readonly(),"Absolute error of contact velocity (with respect to smoothed value)")) \
		((Real,smoothErr,-1,,"Smoothing factor for computing errors; if negative, set to *smooth* automatically.")) \
		((Real,smooth,1e-4,,"Smoothing factor for computing *smoothF*")) \
		((int,stage,0,AttrTrait<>().readonly(),"Current stage to be finished")) \
		((Real,stageT0,-1,AttrTrait<>().readonly(),"Time at which this stage was entered")) \
		((vector<shared_ptr<LawTesterStage>>,stages,,,"Stages to be reached during the testing")) \
		((int,maxStageSteps,100000,AttrTrait<>().noGui(),"Throw error if stage takes this many steps")) \
		,/*py*/ .def("fuv",&LawTester::pyFuv,"Return python dictionary containing f,u,v,smooF,smooU,smooU; useful for plotting with `woo.plot.addData(**tester.dict())`").def("restart",&LawTester::restart,"Reset the tester to initial state; all stages are reset via :obj:`LawTesterStage.reset`, the :obj:`woo.core.Engine.dead` flag is unset.") \

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_LawTester__CLASS_BASE_DOC_ATTRS_PY);

};
WOO_REGISTER_OBJECT(LawTester);
