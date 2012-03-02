#pragma once
#ifdef YADE_CLDEM

#include<yade/core/Field.hpp>
#include<yade/core/Engine.hpp>



namespace clDem{ class Simulation; };

// namespace yade{namespace clDem{

class CLDemData: public NodeData{
public:
	YADE_CLASS_BASE_DOC_ATTRS(CLDemData,NodeData,"Dynamic state of node.",
		((long,clIx,,,"Index of object belonging to this node within clDem arrays (particle/contact)"))
	);
};
REGISTER_SERIALIZABLE(CLDemData);

template<> struct NodeData::Index<CLDemData>{enum{value=Node::ST_CLDEM};};

struct CLDemField: public Field{
	shared_ptr<clDem::Simulation> sim;
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CLDemField,Field,"Field referencing clDem simulation; it contains reference pointer clDem::Simulation. GL functions and proxy engine is defined on this field.",
		// ((shared_ptr<clDem::Simulation>,sim,,Attr::noSave,"The OpenCL simulation in question (not saved!)."))
		, /* ctor */ createIndex();
		, .def_readwrite("sim",&CLDemField::sim,"Simulation field to operate on")
	);
	REGISTER_CLASS_INDEX(CLDemField,Field);
	// get coordinates from particles, not from nodes
	bool renderingBbox(Vector3r&, Vector3r&); 
	// clDem engines should inherit protected from this class
	struct Engine: public Field::Engine{
		virtual bool acceptsField(Field* f){ return dynamic_cast<CLDemField*>(f); }
	};

};
REGISTER_SERIALIZABLE(CLDemField);

struct CLDemRun: public CLDemField::Engine, public PeriodicEngine {
	void run();
	//void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	YADE_CLASS_BASE_DOC_ATTRS(CLDemRun,PeriodicEngine,"Engine which runs some number of steps of the clDem simulation synchronously.",
		((long,steps,-1,,"How many steps to run each time the engine runs. If negative, the value of *stepPeriod* is used."))
	);

};
REGISTER_SERIALIZABLE(CLDemRun);

#ifdef YADE_OPENGL
#include<yade/pkg/gl/Functors.hpp>

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
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_CLDemField,GlFieldFunctor,"Render clDemField.",
		((bool,parWire,false,,"Whether particles are rendered with wirte only"))
		((Real,quality,.2,,"Adjust number of slices/stacks for spheres &c"))
		((Vector2r,quality_range,Vector2r(0,1),,"Range for quality"))
		((bool,bboxes,true,,"Render bounding boxes"))
		((bool,par,true,,"Render particles"))
		((bool,pot,true,,"Render potential contacts"))
		((bool,con,true,,"Render real contacts"))
		((shared_ptr<ScalarRange>,parRange,make_shared<ScalarRange>(),,"Range for particle colors (velocity)"))
		// ((unsigned int,mask,0,,"Only shapes/bounds of particles with this mask will be displayed; if 0, all particles are shown"))
	);
};
#endif




#endif
// }}; /* yade::clDem */


