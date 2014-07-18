#pragma once
#ifdef WOO_CLDEM

#include<woo/core/Field.hpp>
#include<woo/core/Field-templates.hpp>
#include<woo/core/Engine.hpp>

// forwards
namespace clDem{ class Simulation; class Particle; };

// namespace woo{namespace clDem{

class CLDemData: public NodeData{
public:
	const char* getterName() const WOO_CXX11_OVERRIDE { return "clDem"; }
	void setDataOnNode(Node& n) WOO_CXX11_OVERRIDE { n.setData(static_pointer_cast<CLDemData>(shared_from_this())); }
	
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(CLDemData,NodeData,"Dynamic state of node.",
		((long,clIx,,,"Index of object belonging to this node within clDem arrays (particle/contact)"))
		, /* ctor */
		, /* py */ .def("_getDataOnNode",&Node::pyGetData<CLDemData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<CLDemData>).staticmethod("_setDataOnNode");
	);
};
WOO_REGISTER_OBJECT(CLDemData);

template<> struct NodeData::Index<CLDemData>{enum{value=Node::ST_CLDEM};};

// no longer needed
#if 0
#if BOOST_VERSION<104900
	namespace boost {
		namespace align { struct __attribute__((__aligned__(128))) a128 {};}
		template<> class type_with_alignment<128> { public: typedef align::a128 type; };
	};
#endif
#endif

struct CLDemField: public Field{
	static shared_ptr<Scene> clDemToWoo(const shared_ptr<clDem::Simulation>& sim, int stepPeriod=-1, Real relTol=-1);
	static shared_ptr<clDem::Simulation> wooToClDem(const shared_ptr< ::Scene>& scene, int stepPeriod=-1, Real relTol=-1);
	// returns clDem::Simulation within current scene
	static shared_ptr<clDem::Simulation> getSimulation();
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(CLDemField,Field,"Field referencing clDem simulation; it contains reference pointer clDem::Simulation. GL functions and proxy engine is defined on this field.",
			((shared_ptr<clDem::Simulation>,sim,,,"The OpenCL simulation in question."))
		, /* ctor */ createIndex();
		, /* py */ // .def_readwrite("sim",&CLDemField::sim,"Simulation field to operate on")
		.def("clDemToWoo",&CLDemField::clDemToWoo,(py::arg("clDemSim"),py::arg("stepPeriod")=1,py::arg("relTol")=-1),"Create woo simulation which mimics the one in *clDemSim* as close as possible. If stepPeriod>=1, prepare enginess for running and comparing them in parallel. *stepPeriod* determines how many steps of the CL simulation to launch at once. If *relTol* is greater than 0., comparison between clDem and Woo will be done at every step, with the tolerance specified.")
		.staticmethod("clDemToWoo")
		.def("wooToClDem",&CLDemField::wooToClDem,(py::arg("scene"),py::arg("stepPeriod")=-1,py::arg("relTol")=-1),"Convert woo simulation in *scene* to clDem simulation (returned object), optionally adding the clDem simulation to the woo's scene itself (if stepPeriod>=1) to be run in parallel. Positive value of *relTol* will run checks between both computations after each *stepPeriod* steps.")
		.staticmethod("wooToClDem")
		.def("getSimulation",&CLDemField::getSimulation).staticmethod("getSimulation")
		.def("sceneHasField",&Field_sceneHasField<CLDemField>).staticmethod("sceneHasField")
		.def("sceneGetField",&Field_sceneGetField<CLDemField>).staticmethod("sceneGetField")
	);
	REGISTER_CLASS_INDEX(CLDemField,Field);
	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	// get coordinates from particles, not from nodes
	bool renderingBbox(Vector3r&, Vector3r&); 
};
WOO_REGISTER_OBJECT(CLDemField);

struct CLDemRun: public PeriodicEngine {
	bool acceptsField(Field* f){ return dynamic_cast<CLDemField*>(f); }
	shared_ptr<clDem::Simulation> sim;
	void run();
	void doCompare();
	void compareParticleNodeDyn(const string& pId, const clDem::Particle& cp, const shared_ptr<Node>& yn, const Real kgU, const Real mU, const Real sU);
	//void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(CLDemRun,PeriodicEngine,"Engine which runs some number of steps of the clDem simulation synchronously.",
		((long,steps,-1,,"How many steps to run each time the engine runs. If negative, the value of *stepPeriod* is used."))
		((Real,relTol,-1e-5,,"Tolerance for float comparisons; if it is exceeded, error message is shown, but the simulation is not interrupted. If negative, no comparison is done."))
		((Real,raiseLimit,100.,,"When relative error exceeds relTol*raiseLimit, an exception will be raised. (>=1)"))
		, /* ctor */
		, /* py */
	);
};
WOO_REGISTER_OBJECT(CLDemRun);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>

struct Gl1_CLDemField: public GlFieldFunctor{
	virtual void go(const shared_ptr<Field>&, GLViewInfo*);
	GLViewInfo* viewInfo;
	shared_ptr<clDem::Simulation> sim;
	RENDERS(CLDemField);
	int slices, stacks; // compute from quality at every step

	void renderBboxes();
	void renderPar();
	void renderCon();
	void renderPot();
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_CLDemField,GlFieldFunctor,"Render clDemField.",
		((bool,parWire,false,,"Whether particles are rendered with wirte only"))
		((Real,quality,.2,,"Adjust number of slices/stacks for spheres &c"))
		((Vector2r,quality_range,Vector2r(0,1),AttrTrait<>().noGui(),"Range for quality"))
		((bool,bboxes,true,,"Render bounding boxes"))
		((bool,par,true,,"Render particles"))
		((bool,pot,true,,"Render potential contacts"))
		((bool,con,true,,"Render real contacts"))
		((shared_ptr<ScalarRange>,parRange,make_shared<ScalarRange>(),,"Range for particle colors (velocity)"))
		((shared_ptr<ScalarRange>,conRange,make_shared<ScalarRange>(),,"Range for contact colors (normal force)"))
		// ((unsigned int,mask,0,,"Only shapes/bounds of particles with this mask will be displayed; if 0, all particles are shown"))
	);
};
WOO_REGISTER_OBJECT(Gl1_CLDemField);
#endif




#endif
// }}; /* woo::clDem */

