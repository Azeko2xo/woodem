#pragma once

#include<unordered_map>
#include<woo/core/Field.hpp>
#include<woo/core/Scene.hpp>
#include<woo/core/Field-templates.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/pkg/dem/ContactContainer.hpp>


// namespace woo{namespace dem{

struct Particle;
struct Contact;
struct Material;
struct MatState;
struct Bound;
struct Shape;
struct Impose;

struct ScalarRange;

struct Particle: public Object{
	shared_ptr<Contact> findContactWith(const shared_ptr<Particle>& other);
	typedef ParticleContainer::id_t id_t;
	// try unordered_map
	typedef std::map<id_t,shared_ptr<Contact> > MapParticleContact;
	void checkNodes(bool dyn=true, bool checkOne=true) const;
	void selfTest();

	DECLARE_LOGGER;


	Vector3r& getPos() const; void setPos(const Vector3r&);
	Quaternionr& getOri() const; void setOri(const Quaternionr&);
	Vector3r& getVel() const; void setVel(const Vector3r&);
	Vector3r& getAngVel() const; void setAngVel(const Vector3r&);
	shared_ptr<Impose> getImpose() const; void setImpose(const shared_ptr<Impose>&);
	Real getEk_any(bool trans, bool rot, Scene* scene=NULL) const;
	Real getEk() const {return getEk_any(true,true); }
	Real getEk_trans() const { return getEk_any(true,false); }
	Real getEk_rot() const {return getEk_any(false,true); }
	Real getMass() const;
	Vector3r getInertia() const;
	Vector3r getForce() const;
	Vector3r getTorque() const;
	py::dict pyContacts() const;
	py::list pyCon() const;
	py::list pyTacts() const;
	std::string getBlocked() const; void setBlocked(const std::string&);
	#ifdef WOO_OPENGL
		// getRefPos creates refPos if it does not exist, and initializes to current pos
		Vector3r& getRefPos();
	#else
		// returns NaN,NaN,NaN
		Vector3r getRefPos() const;
	#endif
	void setRefPos(const Vector3r&);
	std::vector<shared_ptr<Node> > getNodes();
	virtual string pyStr() const { return "<Particle #"+to_string(id)+" @ "+lexical_cast<string>(this)+">"; }
	void postLoad(Particle&,void*);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Particle,Object,ClassTrait().doc("Particle in DEM").section("Particle","Each particles in DEM is defined by its shape (given by multiple nodes) and other parameters.",{"Shape","Material","Bound"}),
		((id_t,id,-1,AttrTrait<Attr::readonly>(),"Index in DemField::particles"))
		((uint,mask,1,,"Bitmask for collision detection and other (group 1 by default)"))
		((shared_ptr<Shape>,shape,,,"Geometrical configuration of the particle"))
		((shared_ptr<Material>,material,,,"Material of the particle"))
		((shared_ptr<MatState>,matState,,,"Material state of the particle (such as damage data and similar)"))
		// not saved, reconstructed from DemField::postLoad
		((MapParticleContact,contacts,,AttrTrait<Attr::noSave|Attr::hidden>(),"Contacts of this particle, indexed by id of the other particle."))
		// ((int,flags,0,AttrTrait<Attr::hidden>(),"Various flags, only individually accesible from Python"))
		, /*ctor*/
		, /*py*/
			.add_property("contacts",&Particle::pyContacts)
			.add_property("con",&Particle::pyCon)
			.add_property("tacts",&Particle::pyTacts)
		.add_property("pos",py::make_function(&Particle::getPos,py::return_internal_reference<>()),py::make_function(&Particle::setPos))
		.add_property("refPos",py::make_function(&Particle::getRefPos
			#ifdef WOO_OPENGL
				,py::return_internal_reference<>()
			#endif
			),py::make_function(&Particle::setRefPos))
		.add_property("ori",py::make_function(&Particle::getOri,py::return_internal_reference<>()),py::make_function(&Particle::setOri))
		.add_property("vel",py::make_function(&Particle::getVel,py::return_internal_reference<>()),py::make_function(&Particle::setVel))
		.add_property("angVel",py::make_function(&Particle::getAngVel,py::return_internal_reference<>()),py::make_function(&Particle::setAngVel))
		.add_property("impose",&Particle::getImpose,&Particle::setImpose)
		.add_property("mass",&Particle::getMass)
		.add_property("inertia",&Particle::getInertia)
		.add_property("f",&Particle::getForce)
		.add_property("t",&Particle::getTorque)
		.add_property("nodes",&Particle::getNodes)
		.add_property("blocked",&Particle::getBlocked,&Particle::setBlocked)
		.add_property("Ek",&Particle::getEk,"Summary kinetic energy of the particle")
		.add_property("Ekt",&Particle::getEk_trans,"Translational kinetic energy of the particle")
		.add_property("Ekr",&Particle::getEk_rot,"Rotational kinetic energy of the particle")
	);
};
WOO_REGISTER_OBJECT(Particle);

struct Impose: public Object{
	virtual void velocity(const Scene*, const shared_ptr<Node>&){ throw std::runtime_error("Calling abstract Impose::velocity."); }
	virtual void force(const Scene*, const shared_ptr<Node>&){ throw std::runtime_error("Calling abstract Impose::force."); }
	enum{ NONE=0, VELOCITY=1, FORCE=2};
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Impose,Object,"Impose arbitrary changes in Node and DemData, right after integration of the node.",
		((int,what,,AttrTrait<>().readonly().choice({{0,"none"},{VELOCITY,"velocity"},{FORCE,"force"},{VELOCITY|FORCE,"velocity+force"}}),"What values are to be imposed; this is set by the derived engine automatically depending on what is to be prescribed."))
		,/*ctor*/
		,/*py*/
			;
			_classObj.attr("none")=(int)NONE;
			_classObj.attr("velocity")=(int)VELOCITY;
			_classObj.attr("force")=(int)FORCE;
			
	);
};
WOO_REGISTER_OBJECT(Impose);


struct MatState: public Object{
	boost::mutex lock;
	// returns scalar for rendering purposes 
	virtual Real getScalar(int index, const long& step, const Real& smooth=0){ return NaN; }
	virtual string getScalarName(int index){ return ""; }
	WOO_CLASS_BASE_DOC_ATTRS_PY(MatState,Object,"Holds data related to material state of each particle.",
		((long,stepUpdated,-1,,":obj:`woo.core.Scene.step` in which this object was updated for the last time."))
		,/*py*/
		.def("getScalar",&MatState::getScalar,(py::arg("index"),py::arg("step")=-1,py::arg("smooth")=0),"Return scalar value given its numerical index. *step* is used to check whether data are up-to-date, smooth (if positive) is used to smooth out old data (usually using exponential decay function)")
		.def("getScalarName",&MatState::getScalarName,py::arg("index"),"Return name of scalar at given index (human-readable description)")
	);
};
WOO_REGISTER_OBJECT(MatState);


struct DemData: public NodeData{
public:
	DECLARE_LOGGER;
	// bits for flags
	enum {
		DOF_NONE=0,DOF_X=1,DOF_Y=2,DOF_Z=4,DOF_RX=8,DOF_RY=16,DOF_RZ=32,
		CLUMP_CLUMPED=64,CLUMP_CLUMP=128,ENERGY_SKIP=256,GRAVITY_SKIP=512
	};
	//! shorthands
	static const unsigned DOF_ALL=DOF_X|DOF_Y|DOF_Z|DOF_RX|DOF_RY|DOF_RZ;
	static const unsigned DOF_XYZ=DOF_X|DOF_Y|DOF_Z;
	static const unsigned DOF_RXRYRZ=DOF_RX|DOF_RY|DOF_RZ;

	//! Return DOF_* constant for given axis∈{0,1,2} and rotationalDOF∈{false(default),true}; e.g. axisDOF(0,true)==DOF_RX
	static unsigned axisDOF(int axis, bool rotationalDOF=false){return 1<<(axis+(rotationalDOF?3:0));}
	//! Return DOF_* constant for given DOF number ∈{0,1,2,3,4,5}
	static unsigned linDOF(int dof){ return 1<<(dof); }
	//! Getter of blocked for list of strings (e.g. DOF_X | DOR_RX | DOF_RZ → 'xXZ')
	std::string blocked_vec_get() const;
	//! Setter of blocked from string ('xXZ' → DOF_X | DOR_RX | DOF_RZ)
	void blocked_vec_set(const std::string& dofs);

	bool isBlockedNone() const { return (flags&DOF_ALL)==DOF_NONE; }
	void setBlockedNone() { flags&=~DOF_ALL; }
	bool isBlockedAll()  const { return (flags&DOF_ALL)==DOF_ALL; }
	void setBlockedAll() { flags|=DOF_ALL; }
	bool isBlockedAxisDOF(int axis, bool rot) const { return (flags & axisDOF(axis,rot)); }

	// predicates and setters for clumps
	bool isClumped() const { return flags&CLUMP_CLUMPED; }
	bool isClump()   const { return flags&CLUMP_CLUMP; }
	bool isNoClump() const { return !isClumped() && !isClump(); }
	void setClumped(const shared_ptr<Node>& _master){ master=_master; flags|=CLUMP_CLUMPED; flags&=~CLUMP_CLUMP; }
	void setClump()  { flags|=CLUMP_CLUMP; flags&=~CLUMP_CLUMPED; }
	void setNoClump(){ flags&=~(CLUMP_CLUMP | CLUMP_CLUMPED); }

	// predicates and setters for skipping this node in energy calculations
	bool isEnergySkip() const { return flags&ENERGY_SKIP; }
	void setEnergySkip(bool skip) { if(!skip) flags&=~ENERGY_SKIP; else flags|=ENERGY_SKIP; }
	bool isGravitySkip() const { return flags&GRAVITY_SKIP; }
	void setGravitySkip(bool grav) { if(!grav) flags&=~GRAVITY_SKIP; else flags|=GRAVITY_SKIP; }

	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	void addForceTorque(const Vector3r& f, const Vector3r& t=Vector3r::Zero()){ boost::mutex::scoped_lock l(lock); force+=f; torque+=t; }
	void addForce(const Vector3r& f){ boost::mutex::scoped_lock l(lock); force+=f; }

	// get kinetic energy of given node
	static Real getEk_any(const shared_ptr<Node>& n, bool trans, bool rot, Scene* scene);

	py::list pyParRef_get();
	void addParRef(const shared_ptr<Particle>&);
	void addParRef_raw(Particle*);
	shared_ptr<Node> pyGetMaster(){ return master.lock(); }
	
	// type for back-referencing particle which has this node;
	// cannot be shared_ptr, as it would be circular
	// weak_ptr was used, but it was buggy due to http://stackoverflow.com/questions/8233252/boostpython-and-weak-ptr-stuff-disappearing
	// use raw pointer, which cannot be serialized, and set it from Particle::postLoad
	// cross thumbs, that is quite fragile :|


	bool isAspherical() const{ return !((inertia[0]==inertia[1] && inertia[1]==inertia[2])); }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(DemData,NodeData,"Dynamic state of node.",
		((Vector3r,vel,Vector3r::Zero(),AttrTrait<>().velUnit(),"Linear velocity."))
		((Vector3r,angVel,Vector3r::Zero(),AttrTrait<>().angVelUnit(),"Angular velocity."))
		((Real,mass,0,AttrTrait<>().massUnit(),"Associated mass."))
		((Vector3r,inertia,Vector3r::Zero(),AttrTrait<>().inertiaUnit(),"Inertia along local (principal) axes"))
		((Vector3r,force,Vector3r::Zero(),AttrTrait<>().forceUnit(),"Applied force"))
		((Vector3r,torque,Vector3r::Zero(),AttrTrait<>().torqueUnit(),"Applied torque"))
		((Vector3r,angMom,Vector3r::Zero(),AttrTrait<>().angMomUnit(),"Angular momentum (used with the aspherical integrator)"))
		((unsigned,flags,0,AttrTrait<Attr::readonly>().bits({"block x","block y","block z","block rot x","block rot y","block rot z","clumped","clump","energy skip","gravity skip"}),"Bit flags storing blocked DOFs, clump status, ..."))
		((long,linIx,-1,AttrTrait<>().readonly().noGui(),"Index within DemField.nodes (for efficient removal)"))
		((std::list<Particle*>,parRef,,AttrTrait<Attr::hidden|Attr::noSave>(),"Back-reference for particles using this node; this is important for knowing when a node may be deleted (no particles referenced) and such. Should be kept consistent."))
		((shared_ptr<Impose>,impose,,,"Impose arbitrary velocity, angular velocity, ... on the node; the functor is called from Leapfrog, after new position and velocity have been computed."))
		((weak_ptr<Node>,master,,AttrTrait<>().hidden(),"Master node; currently only used with clumps (since this is never set from python, it is safe to use weak_ptr)."))
		, /*ctor*/
		, /*py*/ .add_property("blocked",&DemData::blocked_vec_get,&DemData::blocked_vec_set,"Degress of freedom where linear/angular velocity will be always constant (equal to zero, or to an user-defined value), regardless of applied force/torque. String that may contain 'xyzXYZ' (translations and rotations).")
		.add_property("clump",&DemData::isClump).add_property("clumped",&DemData::isClumped).add_property("noClump",&DemData::isNoClump).add_property("energySkip",&DemData::isEnergySkip,&DemData::setEnergySkip).add_property("gravitySkip",&DemData::isGravitySkip,&DemData::setGravitySkip)
		.add_property("master",&DemData::pyGetMaster)
		.add_property("parRef",&DemData::pyParRef_get).def("addParRef",&DemData::addParRef)
		.def("_getDataOnNode",&Node::pyGetData<DemData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<DemData>).staticmethod("_setDataOnNode")
		;
	);
};
WOO_REGISTER_OBJECT(DemData);

template<> struct NodeData::Index<DemData>{enum{value=Node::ST_DEM};};

struct DemField: public Field{
	DECLARE_LOGGER;
	int collectNodes();
	void clearDead(){ deadNodes.clear(); }
	void removeParticle(Particle::id_t id);
	void removeClump(size_t id);
	AlignedBox3r renderingBbox() const; // overrides Field::renderingBbox
	boost::mutex nodesMutex; // sync adding nodes with the renderer, which might otherwise crash

	void selfTest() WOO_CXX11_OVERRIDE;

	void pyNodesAppend(const shared_ptr<Node>& n);

	//template<> bool sceneHasField<DemField>() const;
	//template<> shared_ptr<DemField> sceneGetField<DemField>() const;
	void postLoad(DemField&,void*);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(DemField,Field,"Field describing a discrete element assembly. Each body references (possibly many) nodes by their index in :ref:`Field.nodes` and :ref:`Field.nodalData`. ",
		((shared_ptr<ParticleContainer>,particles,make_shared<ParticleContainer>(),AttrTrait<>().pyByRef().readonly().ini(),"Particles (each particle holds its contacts, and references associated nodes)"))
		((shared_ptr<ContactContainer>,contacts,make_shared<ContactContainer>(),AttrTrait<>().pyByRef().readonly().ini(),"Linear view on particle contacts"))
		((uint,loneMask,0,,"Particle groups which have bits in loneMask in common (i.e. (A.mask & B.mask & loneMask)!=0) will not have contacts between themselves"))
		((Vector3r,gravity,Vector3r::Zero(),,"Constant gravity acceleration"))
		((bool,saveDeadNodes,false,AttrTrait<>().buttons({"Clear dead nodes","self.clearDead()",""}),"Save unused nodes of deleted particles, which would be otherwise removed (useful for displaying traces of deleted particles)."))
		((vector<shared_ptr<Node>>,deadNodes,,AttrTrait<Attr::readonly>().noGui(),"List of nodes belonging to deleted particles; only used if :ref:`saveDeadNodes` is `True`"))
		, /* ctor */ createIndex(); postLoad(*this,NULL); /* to make sure pointers are OK */
		, /*py*/
		.def("collectNodes",&DemField::collectNodes,"Collect nodes from all particles and clumps and insert them to nodes defined for this field. Nodes are not added multiple times, even if they are referenced from different particles.")
		.def("clearDead",&DemField::clearDead)
		.def("nodesAppend",&DemField::pyNodesAppend,"Append given node to :obj:`nodes`, and set :obj:`DemData.linIx` to the correct value automatically.")
		.def("sceneHasField",&Field_sceneHasField<DemField>).staticmethod("sceneHasField")
		.def("sceneGetField",&Field_sceneGetField<DemField>).staticmethod("sceneGetField")
	);
	REGISTER_CLASS_INDEX(DemField,Field);
};
WOO_REGISTER_OBJECT(DemField);


struct Shape: public Object, public Indexable{
	virtual bool numNodesOk() const { return true; } // checks for the right number of nodes; to be used in assertions
	// this will be called from DemField::selfTest for each particle
	virtual void selfTest(const shared_ptr<Particle>& p){};
	// return average position of nodes, useful for rendering
	// caller must make sure that !nodes.empty()
	Real getSignedBaseColor(){ return color-trunc(color); }
	Real getBaseColor(){ return abs(color)-trunc(abs(color)); }
	void setBaseColor(Real c){ if(isnan(c)) return; color=trunc(color)+(color<0?-1:1)*CompUtils::clamped(c,0,1); }
	bool getWire() const { return color<0; }
	void setWire(bool w){ color=(w?-1:1)*abs(color); }
	bool getHighlighted() const { return abs(color)>=1 && abs(color)<2; }
	void setHighlighted(bool h){ if(getHighlighted()==h) return; color=(color>=0?1:-1)+getSignedBaseColor(); }
	bool getVisible() const { return abs(color)<=2; }
	void setVisible(bool w){ if(getVisible()==w) return; bool hi=abs(color)>1; int sgn=(color<0?-1:1); color=sgn*((w?0:2)+getBaseColor()+(hi?1:0)); }
	Vector3r avgNodePos();
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Shape,Object,"Particle geometry",
		((shared_ptr<Bound>,bound,,,"Bound of the particle, for use by collision detection only"))
		((vector<shared_ptr<Node> >,nodes,,,"Nodes associated with this particle"))
		((Real,color,Mathr::UnitRandom(),,"Normalized color for rendering; negative values render with wire (rather than solid), |color|>2 means invisible. (use *wire*, *hi* and *visible* to manipulate those)"))
		,/*ctor*/,/*py*/
			.add_property("wire",&Shape::getWire,&Shape::setWire)
			.add_property("hi",&Shape::getHighlighted,&Shape::setHighlighted)
			.add_property("visible",&Shape::getVisible,&Shape::setVisible)
			WOO_PY_TOPINDEXABLE(Shape)
	);
	REGISTER_INDEX_COUNTER(Shape);
};
WOO_REGISTER_OBJECT(Shape);

struct Material: public Object, public Indexable{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Material,Object,"Particle material",
		((Real,density,NaN,AttrTrait<>().densityUnit(),"Density"))
		((int,id,-1,AttrTrait<>().noGui(),"Some number identifying this material; used with MatchMaker objects, useless otherwise"))
		,/*ctor*/,/*py*/ WOO_PY_TOPINDEXABLE(Material);
	);
	REGISTER_INDEX_COUNTER(Material);
};
WOO_REGISTER_OBJECT(Material);

struct Bound: public Object, public Indexable{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Bound,Object,"Object bounding the associated body.",
		// ((Vector3r,color,Vector3r(1,1,1),,"Color for rendering this object"))
		((Vector3r,min,Vector3r(NaN,NaN,NaN),AttrTrait<Attr::noSave>().readonly().lenUnit(),"Lower corner of box containing this bound"))
		((Vector3r,max,Vector3r(NaN,NaN,NaN),AttrTrait<Attr::noSave>().readonly().lenUnit(),"Lower corner of box containing this bound"))
		,/* ctor*/,	/*py*/ WOO_PY_TOPINDEXABLE(Bound)
	);
	REGISTER_INDEX_COUNTER(Bound);
};
WOO_REGISTER_OBJECT(Bound);


/***************************************************

                CONTACT CLASSES

***************************************************/


struct CGeom: public Object,public Indexable{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(CGeom,Object,"Geometrical configuration of contact",
		((shared_ptr<Node>,node,new Node,,"Local coordinates definition."))
		,/*ctor*/,/*py*/WOO_PY_TOPINDEXABLE(CGeom)
	);
	REGISTER_INDEX_COUNTER(CGeom);
};
WOO_REGISTER_OBJECT(CGeom);

struct CPhys: public Object, public Indexable{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(CPhys,Object,"Physical properties of contact.",
		/*attrs*/
		((Vector3r,force,Vector3r::Zero(),AttrTrait<>().forceUnit(),"Force applied on the first particle in the contact"))
		((Vector3r,torque,Vector3r::Zero(),AttrTrait<>().torqueUnit(),"Torque applied on the first particle in the contact"))
		,/*ctor*/ createIndex(); ,/*py*/WOO_PY_TOPINDEXABLE(CPhys)
	);
	REGISTER_INDEX_COUNTER(CPhys);
};
WOO_REGISTER_OBJECT(CPhys);

struct CData: public Object{
	WOO_CLASS_BASE_DOC_ATTRS(CData,Object,"Optional data stored in the contact by the Law functor.",
		/* attrs */
	);
};
WOO_REGISTER_OBJECT(CData);

struct Contact: public Object{
	bool isReal() const { return geom&&phys; }
	bool isColliding() const { return stepCreated>=0; }
	void setNotColliding(){ stepCreated=-1; }
	bool isFresh(Scene* s){ return s->step==stepCreated; }
	void swapOrder();
	void reset();
	// return -1 or +1 depending on whether the particle passed to us is pA or pB
	int forceSign(const shared_ptr<Particle>& p) const { return p.get()==leakPA()?1:-1; }
	/* return force and torque in global coordinates which act at contact point C located at branch from node nodeI of particle.
	Contact force is reversed automatically if particle==pB.
	Force and torque at node itself are  F and branch.cross(F)+T respectively.
	Branch takes periodicity (cellDist) in account.
	See In2_Sphere_Elastmat::go for its use.  */
	std::tuple<Vector3r,Vector3r,Vector3r> getForceTorqueBranch(const shared_ptr<Particle>& p, int nodeI, Scene* scene){ return getForceTorqueBranch(p.get(),nodeI,scene); }
	std::tuple<Vector3r,Vector3r,Vector3r> getForceTorqueBranch(const Particle*, int nodeI, Scene* scene);
	// return position vector between pA and pB, taking in account PBC's; both must be uninodal
	Vector3r dPos(const Scene* s) const;
	Vector3r dPos_py() const{ if(pA.expired()||pB.expired()) return Vector3r(NaN,NaN,NaN); return dPos(Master::instance().getScene().get()); }
	Real dist_py() const { return dPos_py().norm(); }
	Particle::id_t pyId1() const;
	Particle::id_t pyId2() const;
	Vector2i pyIds() const;
	void pyResetPhys(){ phys.reset(); }
	virtual string pyStr() const { return "<Contact ##"+to_string(pyId1())+"+"+to_string(pyId2())+" @ "+lexical_cast<string>(this)+">"; }
	// potentially unsafe pointer extraction (assert safety in debug builds only)
	// only use when the particles are sure to exist and never return this pointer anywhere
	// we call it "leak" to make this very explicit
	Particle* leakPA() const { assert(!pA.expired()); return pA.lock().get(); }
	Particle* leakPB() const { assert(!pB.expired()); return pB.lock().get(); }
	shared_ptr<Particle> pyPA() const { return pA.lock(); }
	shared_ptr<Particle> pyPB() const { return pB.lock(); }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Contact,Object,"Contact in DEM",
		((shared_ptr<CGeom>,geom,,AttrTrait<Attr::readonly>(),"Contact geometry"))
		((shared_ptr<CPhys>,phys,,AttrTrait<Attr::readonly>(),"Physical properties of contact"))
		((shared_ptr<CData>,data,,AttrTrait<Attr::readonly>(),"Optional data stored by the functor for its own use"))
		// these two are overridden below because of weak_ptr
		((weak_ptr<Particle>,pA,,AttrTrait<Attr::readonly>(),"First particle of the contact"))
		((weak_ptr<Particle>,pB,,AttrTrait<Attr::readonly>(),"Second particle of the contact"))
		((Vector3i,cellDist,Vector3i::Zero(),AttrTrait<Attr::readonly>(),"Distace in the number of periodic cells by which pB must be shifted to get to the right relative position."))
		#ifdef WOO_OPENGL
			((Real,color,0,,"(Normalized) color value for this contact"))
		#endif
		((int,stepCreated,-1,AttrTrait<Attr::hidden>(),"Step in which this contact was created by the collider, or step in which it was made real (if geom and phys exist). This number is NOT reset by Contact::reset(). If negative, it means the collider does not want to keep this contact around anymore (this happens if the contact is real but there is no overlap anymore)."))
		((Real,minDist00Sq,-1,AttrTrait<Attr::hidden>(),"Minimum distance between nodes[0] of both shapes so that the contact can exist. Set in ContactLoop by geometry functor once, and is used to check for possible contact without having to call the functor. If negative, not used. Currently, only Sphere-Sphere contacts use this information."))
		((int,stepLastSeen,-1,AttrTrait<Attr::hidden>(),""))
		((size_t,linIx,0,AttrTrait<Attr::readonly>().noGui(),"Position in the linear view (ContactContainer)"))
		, /*ctor*/
		, /*py*/ .add_property("id1",&Contact::pyId1).add_property("id2",&Contact::pyId2).add_property("real",&Contact::isReal).add_property("ids",&Contact::pyIds)
		.def("dPos",&Contact::dPos_py,"Return position difference vector pB-pA, taking `Contact.cellDist` in account properly. Both particles must be uninodal, exception is raised otherwise.")
		.def("dist",&Contact::dist_py,"Shorthand for dPos.norm().")
		.add_property("pA",&Contact::pyPA,"First particle of the contact")
		.add_property("pB",&Contact::pyPB,"Second particle of the contact")
		.def("resetPhys",&Contact::pyResetPhys,"Set *phys* to *None* (to force its re-evaluation)")
	);
};
WOO_REGISTER_OBJECT(Contact);




// }}; /* woo::dem */

