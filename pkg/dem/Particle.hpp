#pragma once

#include<unordered_map>
#include<woo/core/Field.hpp>
#include<woo/core/Scene.hpp>
#include<woo/core/Field-templates.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/pkg/dem/ContactContainer.hpp>
#include<woo/lib/pyutil/converters.hpp>
#include<atomic>
#include<boost/utility/binary.hpp>


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

	WOO_DECL_LOGGER;


	Vector3r& getPos() const; void setPos(const Vector3r&);
	Quaternionr& getOri() const; void setOri(const Quaternionr&);
	Vector3r& getVel() const; void setVel(const Vector3r&);
	Vector3r& getAngVel() const; void setAngVel(const Vector3r&);
	shared_ptr<Impose> getImpose() const; void setImpose(const shared_ptr<Impose>&);
	Real getEk_any(bool trans, bool rot, shared_ptr<Scene> scene=shared_ptr<Scene>()) const;
	Real getEk() const {return getEk_any(true,true); }
	Real getEk_trans() const { return getEk_any(true,false); }
	Real getEk_rot() const {return getEk_any(false,true); }
	Real getMass() const;
	Vector3r getInertia() const;
	Vector3r getForce() const;
	Vector3r getTorque() const;
	py::dict pyContacts() const;
	py::dict pyAllContacts() const;
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
	virtual string pyStr() const WOO_CXX11_OVERRIDE { return "<Particle #"+to_string(id)+" @ "+lexical_cast<string>(this)+">"; }
	int countRealContacts() const;
	void postLoad(Particle&,void*);

	// create new particle with given shape and material, set new nodes with DemData as needed, recompute intertia
	static shared_ptr<Particle> make(const shared_ptr<Shape>&, const shared_ptr<Material>&, bool fixed=false);

	// call Shape.updateMassInertia(mat.density): convenience function
	void updateMassInertia();

	// return internal reference for refPos with OpenGL
	// without OpenGL, there is nothing like that
	// we need to branch here since we can't use #ifdef inside macro definition
	#ifdef WOO_OPENGL
		#define woo_dem_Particle__OPENGL__return_internal_reference ,py::return_internal_reference<>()
	#else
		#define woo_dem_Particle__OPENGL__return_internal_reference
	#endif

	#define woo_dem_Particle__CLASS_BASE_DOC_ATTRS_PY \
		Particle,Object,ClassTrait().doc("Particle in DEM").section("Particle","Each particles in DEM is defined by its shape (given by multiple nodes) and other parameters.",{"Shape","Material"}), \
		((id_t,id,-1,AttrTrait<Attr::readonly>(),"Index in DemField::particles")) \
		((uint,mask,1,,"Bitmask for collision detection and other (group 1 by default)")) \
		((shared_ptr<Shape>,shape,,,"Geometrical configuration of the particle")) \
		((shared_ptr<Material>,material,,,"Material of the particle")) \
		((shared_ptr<MatState>,matState,,,"Material state of the particle (such as damage data and similar)")) \
		/* not saved, reconstructed from DemField::postLoad */ \
		((MapParticleContact,contacts,,AttrTrait<Attr::noSave|Attr::hidden>(),"Contacts of this particle, indexed by id of the other particle.")) \
		/* ((int,flags,0,AttrTrait<Attr::hidden>(),"Various flags, only individually accesible from Python")) */ \
		, /*py*/ \
			.def("updateMassInertia",&Particle::updateMassInertia,"Internal use only. Recompute mass and inertia of all particle's nodes; this function is usually called by particle construction routines; interally calls ``Shape::updateMassInertia`` and only works for particles without shared nodes.") \
			.add_property("contacts",&Particle::pyContacts,"Return all :obj:`real <Contact.real>` particle's contacts as dictionary mapping :obj:`other particles' IDs <Particle.id>` and obj:`Contact` objects.") \
			.add_property("con",&Particle::pyCon,"Return list of :obj:`IDs <Particle.id>` of contacting particles (:obj:`~Contact.real` contacts only); shorthand for ``p.contacts.keys()``.") \
			.add_property("tacts",&Particle::pyTacts,"Return list of :obj:`Contact` objects where this particle takes part (:obj:`~Contact.real` contacts only); shorthand for ``p.contact.values()``.") \
			.add_property("allContacts",&Particle::pyAllContacts,"Return dictionary mapping :obj:`other particles' IDs <Particle.id` to :obj:`Contact` objects, including contacts which are *not* :obj:`~Contact.real`.") \
		.add_property("pos",py::make_function(&Particle::getPos,py::return_internal_reference<>()),py::make_function(&Particle::setPos),"Particle position; shorthand for ``p.shape.nodes[0].pos`` -- uninodal particles only (raises exception otherwise).") \
		.add_property("ori",py::make_function(&Particle::getOri,py::return_internal_reference<>()),py::make_function(&Particle::setOri),"Particle orientation; shorthand for ``p.shape.nodes[0].ori`` -- uninodal particles only (raises exception otherwise).") \
		.add_property("refPos",py::make_function(&Particle::getRefPos woo_dem_Particle__OPENGL__return_internal_reference ),py::make_function(&Particle::setRefPos),"Reference particle position; shorthand for :obj:`p.shape.nodes[0].gl.refPos <woo.gl.GlData.refPos>`, and ``Vector3(nan,nan,nan)`` if :obj:`gl <GlData>` is not defined on the node.") \
		.add_property("vel",py::make_function(&Particle::getVel,py::return_internal_reference<>()),py::make_function(&Particle::setVel),"Particle velocity; shorthand for :obj:`p.shape.nodes[0].dem.vel <DemData.vel>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("angVel",py::make_function(&Particle::getAngVel,py::return_internal_reference<>()),py::make_function(&Particle::setAngVel),"Particle angular velocity; shorthand for :obj:`p.shape.nodes[0].dem.angVel <DemData.angVel>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("impose",&Particle::getImpose,&Particle::setImpose,"Particle imposed constraints; shorthand for :obj:`p.shape.nodes[0].dem.impose <DemData.impose>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("mass",&Particle::getMass,"Particle mass; shorthand for :obj:`p.shape.nodes[0].dem.mass <DemData.mass>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("inertia",&Particle::getInertia,"Particle inertia; shorthand for :obj:`p.shape.nodes[0].dem.inertia <DemData.inertia>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("f",&Particle::getForce,"Force on particle; shorthand for :obj:`p.shape.nodes[0].dem.force <DemData.force>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("t",&Particle::getTorque,"Torque on particle; shorthand for :obj:`p.shape.nodes[0].dem.torque <DemData.force>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("blocked",&Particle::getBlocked,&Particle::setBlocked,"Blocked degrees of freedom of the particle; shorthand for :obj:`p.shape.nodes[0].dem.blocked <DemData.blocked>` -- uninodal particles only (raises exception otherwise).") \
		.add_property("nodes",&Particle::getNodes,"List of particle nodes; shorthand for :obj:`p.shape.nodes <Shape.nodes>`.") \
		.add_property("Ek",&Particle::getEk,"Summary kinetic energy of the particle, shorthand for ``p.Ekt+p.Ekr``.") \
		.add_property("Ekt",&Particle::getEk_trans,"Translational kinetic energy of the particle, computed on-demand as :math:`\\frac{1}{2}m|\\vec{v}|^2` -- uninodal particles only (raises exception otherwise). Space deformation is not considered here, see :obj:`getEk`.") \
		.add_property("Ekr",&Particle::getEk_rot,"Rotational kinetic energy of the particle, computed on-demand as :math:`\\frac{1}{2}\\vec{\\omega}^T\\mat{T}^T\\mat{I}\\mat{T}\\omega` (where :math:`\\mat{T}` is :obj:`p.shape.nodes[0].ori <woo.core.Node.ori>` as rotation matrix, :math:`\\mat{I}` is :obj:`p.shape.nodes[0].dem.inertia <DemData.inertia>` as diagonal matrix  -- uninodal particles only (raises exception otherwise).  Space deformation is not considered here, see :obj:`getEk`.") \
		.def("getEk",&Particle::getEk_any,(py::arg("trans")=true,py::arg("rot")=true,py::arg("scene")=shared_ptr<Scene>()),"Compute kinetic energy (translational and/or rotational); when *scene* is given, only fluctuation linear/angular velocity will be considered if periodic boundary conditions are active.") \
		.def("make",&Particle::make,(py::arg("shape"),py::arg("mat"),py::arg("fixed")=false),"Return Particle instance created from given shape and material; nodes with DemData are automatically added to the shape, mass and inertia is recomuted.").staticmethod("make") \
		; \
		woo::converters_cxxVector_pyList_2way<shared_ptr<Particle>>();



	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Particle__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Particle);

struct Impose: public Object{
	virtual void velocity(const Scene*, const shared_ptr<Node>&){ throw std::runtime_error("Calling abstract Impose::velocity."); }
	virtual void force(const Scene*, const shared_ptr<Node>&){ throw std::runtime_error("Calling abstract Impose::force."); }
	virtual void readForce(const Scene*, const shared_ptr<Node>&){ throw std::runtime_error("Calling abstract Impose::readForce."); }
	bool isFirstStepRun(const Scene* scene){
		// this does not need to be locked (hopefully)
		if(stepLast==scene->step) return false;
		boost::mutex::scoped_lock l(lock);
		if(stepLast==scene->step) return false; // test again under the lock, in case it changed meanwhile
		stepLast=scene->step;
		return true;
	}
	boost::mutex lock;
	// INIT_VELOCITY is used in LawTesterStage, but not by Impose classes
	enum{ NONE=0, VELOCITY=1, FORCE=2, INIT_VELOCITY=4, READ_FORCE=8 };
	#define woo_dem_Impose__CLASS_BASE_DOC_ATTRS_PY \
		Impose,Object,"Impose arbitrary changes in Node and DemData, at certain hook points during motion integration. Velocity is imposed after motion integration (and again after computing acceleration from forces, to make sure forces don't alter what is prescribed), force is imposed when forces from the previous step are being reset (thus additional force may be applied on the node as usual); readForce is a special imposition before resetting force, which is meant to look at summary force applied onto node(s)." , \
		((int,what,,AttrTrait<>().bits({"vel","force","iniVel","readForce"}),"What values are to be imposed; this is set by the derived engine automatically depending on what is to be prescribed.")) \
		((long,stepLast,-1,AttrTrait<>().readonly(),"Step in which this imposition was last used; updated atomically by callers from c++ by calling isFirstStepRun.")) \
		,/*py*/ ; \
			_classObj.attr("none")=(int)NONE; \
			_classObj.attr("velocity")=(int)VELOCITY; \
			_classObj.attr("force")=(int)FORCE;
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Impose__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Impose);


struct MatState: public Object{
	boost::mutex lock;
	// returns scalar for rendering purposes 
	virtual Real getScalar(int index, const long& step, const Real& smooth=0){ return NaN; }
	virtual string getScalarName(int index){ return ""; }
	#define woo_dem_MatState__CLASS_BASE_DOC_ATTRS_PY \
		MatState,Object,"Holds data related to material state of each particle.", \
		/*attrs*/ \
		,/*py*/ \
		.def("getScalar",&MatState::getScalar,(py::arg("index"),py::arg("step")=-1,py::arg("smooth")=0),"Return scalar value given its numerical index. *step* is used to check whether data are up-to-date, smooth (if positive) is used to smooth out old data (usually using exponential decay function)") \
		.def("getScalarName",&MatState::getScalarName,py::arg("index"),"Return name of scalar at given index (human-readable description)")
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_MatState__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(MatState);


struct DemData: public NodeData{
public:
	WOO_DECL_LOGGER;
	const char* getterName() const WOO_CXX11_OVERRIDE { return "dem"; }
	void setDataOnNode(Node& n) WOO_CXX11_OVERRIDE { n.setData(static_pointer_cast<DemData>(shared_from_this())); }


	// bits for flags
	enum {
		DOF_NONE=0,DOF_X=1,DOF_Y=2,DOF_Z=4,DOF_RX=8,DOF_RY=16,DOF_RZ=32,
		CLUMP_CLUMPED=64,CLUMP_CLUMP=128,ENERGY_SKIP=256,GRAVITY_SKIP=512,TRACER_SKIP=1024,DAMPING_SKIP=2048,
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
	bool isBlockedAllRot()  const { return (flags&DOF_RXRYRZ)==DOF_RXRYRZ; }
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
	void setGravitySkip(bool skip) { if(!skip) flags&=~GRAVITY_SKIP; else flags|=GRAVITY_SKIP; }
	bool isTracerSkip() const { return flags&TRACER_SKIP; }
	void setTracerSkip(bool skip) { if(!skip) flags&=~TRACER_SKIP; else flags|=TRACER_SKIP; }
	bool isDampingSkip() const { return flags&DAMPING_SKIP; }
	void setDampingSkip(bool skip) { if(!skip) flags&=~DAMPING_SKIP; else flags|=DAMPING_SKIP; }

	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw) WOO_CXX11_OVERRIDE;
	void addForceTorque(const Vector3r& f, const Vector3r& t=Vector3r::Zero()){ boost::mutex::scoped_lock l(lock); force+=f; torque+=t; }
	void addForce(const Vector3r& f){ boost::mutex::scoped_lock l(lock); force+=f; }

	// get kinetic energy of given node
	static Real getEk_any(const shared_ptr<Node>& n, bool trans, bool rot, Scene* scene);

	py::list pyParRef_get();
	void addParRef(const shared_ptr<Particle>&);
	void addParRef_raw(Particle*);
	shared_ptr<Node> pyGetMaster(){ return master.lock(); }

	//! Attempt to compute node's orientation (if allowed), mass and inertia
	// by asking particles in parRef to do that; this assumes the particles
	// are not overlapping
	static void setOriMassInertia(const shared_ptr<Node>& n);
	
	// type for back-referencing particle which has this node;
	// cannot be shared_ptr, as it would be circular
	// weak_ptr was used, but it was buggy due to http://stackoverflow.com/questions/8233252/boostpython-and-weak-ptr-stuff-disappearing
	// use raw pointer, which cannot be serialized, and set it from Particle::postLoad
	// cross thumbs, that is quite fragile :|

	// set angular velocity and reset angular momentum
	void setAngVel(const Vector3r& av);
	void postLoad(DemData&,void*);

	bool isAspherical() const{ return !((inertia[0]==inertia[1] && inertia[1]==inertia[2])); }

	#define woo_dem_DemData__CLASS_BASE_DOC_ATTRS_PY \
		DemData,NodeData,"Dynamic state of node.", \
		((Vector3r,vel,Vector3r::Zero(),AttrTrait<>().velUnit(),"Linear velocity.")) \
		((Vector3r,angVel,Vector3r::Zero(),AttrTrait<Attr::triggerPostLoad>().angVelUnit(),"Angular velocity; when set, :obj:`angMom` is reset (and updated from :obj:`angVel` in :obj:`Leapfrog`)..")) \
		((Real,mass,0,AttrTrait<>().massUnit(),"Associated mass.")) \
		((Vector3r,inertia,Vector3r::Zero(),AttrTrait<>().inertiaUnit(),"Inertia along local (principal) axes")) \
		((Vector3r,force,Vector3r::Zero(),AttrTrait<>().forceUnit(),"Applied force")) \
		((Vector3r,torque,Vector3r::Zero(),AttrTrait<>().torqueUnit(),"Applied torque")) \
		((Vector3r,angMom,Vector3r(NaN,NaN,NaN),AttrTrait<>().angMomUnit(),"Angular momentum; used with the aspherical integrator. If NaN and aspherical integrator (:obj:`Leapfrog`) is used, the value is initialized to :obj:`inertia` × :obj:`angVel`.")) \
		((unsigned,flags,0,AttrTrait<Attr::readonly>().bits({"blockX","blockY","blockZ","blockRotX","blockRotY","blockRotZ","clumped","clump","energySkip","gravitySkip","tracerSkip","dampingSkip"},/*rw*/true),"Bit flags storing blocked DOFs, clump status, ...")) \
		((long,linIx,-1,AttrTrait<>().readonly().noGui(),"Index within DemField.nodes (for efficient removal)")) \
		((std::list<Particle*>,parRef,,AttrTrait<Attr::hidden|Attr::noSave>().noGui(),"Back-reference for particles using this node; this is important for knowing when a node may be deleted (no particles referenced) and such. Should be kept consistent.")) \
		((shared_ptr<Impose>,impose,,,"Impose arbitrary velocity, angular velocity, ... on the node; the functor is called from Leapfrog, after new position and velocity have been computed.")) \
		((weak_ptr<Node>,master,,AttrTrait<Attr::hidden>().noGui(),"Master node; currently only used with clumps (since this is never set from python, it is safe to use weak_ptr).")) \
		, /*py*/ .add_property("blocked",&DemData::blocked_vec_get,&DemData::blocked_vec_set,"Degress of freedom where linear/angular velocity will be always constant (equal to zero, or to an user-defined value), regardless of applied force/torque. String that may contain 'xyzXYZ' (translations and rotations).") \
		.add_property("noClump",&DemData::isNoClump) \
		/*.add_property("clump",&DemData::isClump).add_property("clumped",&DemData::isClumped).add_property("energySkip",&DemData::isEnergySkip,&DemData::setEnergySkip).add_property("gravitySkip",&DemData::isGravitySkip,&DemData::setGravitySkip).add_property("tracerSkip",&DemData::isTracerSkip,&DemData::setTracerSkip).add_property("dampingSkip",&DemData::isDampingSkip,&DemData::setDampingSkip) */ \
		.add_property("master",&DemData::pyGetMaster) \
		.add_property("parRef",&DemData::pyParRef_get).def("addParRef",&DemData::addParRef) \
		.add_property("isAspherical",&DemData::isAspherical,"Return ``True`` when inertia components are not equal.") \
		.def("setOriMassInertia",&DemData::setOriMassInertia,"Lump mass and inertia from all attached particles and attempt to rotate the node so that its axes are principal axes of inertia, if allowed by particles shape (without chaning the geometry).").staticmethod("setOriMassInertia") \
		.def("_getDataOnNode",&Node::pyGetData<DemData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<DemData>).staticmethod("_setDataOnNode")

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_DemData__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(DemData);

template<> struct NodeData::Index<DemData>{enum{value=Node::ST_DEM};};

struct DemField: public Field{
	WOO_DECL_LOGGER;
	int collectNodes();
	void clearDead(){ deadNodes.clear(); deadParticles.clear(); }
	void removeParticle(Particle::id_t id);
	void removeClump(size_t id);
	vector<shared_ptr<Node>> splitNode(const shared_ptr<Node>&, const vector<shared_ptr<Particle>>& pp, const Real massMult=NaN, const Real inertiaMult=NaN);
	AlignedBox3r renderingBbox() const WOO_CXX11_OVERRIDE; // overrides Field::renderingBbox
	boost::mutex nodesMutex; // sync adding nodes with the renderer, which might otherwise crash

	void selfTest() WOO_CXX11_OVERRIDE;

	void pyNodesAppend(const shared_ptr<Node>& n);
	void pyNodesAppendList(const vector<shared_ptr<Node>> nn);

	Real critDt() WOO_CXX11_OVERRIDE;

	// for passing particles to the ctor
	void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d) WOO_CXX11_OVERRIDE;

	enum {
		// first bit: normally novable particles
		// second bit: boundary particle
		// third bit: deletable particle
		defaultMovableMask =BOOST_BINARY(0101),
		defaultBoundaryMask=BOOST_BINARY(0011),
		defaultLoneMask    =BOOST_BINARY(0010),
		defaultInletMask   =BOOST_BINARY(0101),
		defaultOutletMask =BOOST_BINARY(0100)
	};

	//template<> bool sceneHasField<DemField>() const;
	//template<> shared_ptr<DemField> sceneGetField<DemField>() const;
	void postLoad(DemField&,void*);
	#define woo_dem_DemField__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		DemField,Field,ClassTrait().doc("Field describing a discrete element assembly. Each particle references (possibly many) nodes.\n\n.. admonition:: Special constructor\n\n\tWhen passed the ``par`` parameter in constructor, this sequence of particles will be assigned to :obj:`particles`, and :obj:`collectNodes` will be called immediately.").section("DEM field","TODO",{"ContactContainer","ParticleContainer"}), \
		((shared_ptr<ParticleContainer>,particles,make_shared<ParticleContainer>(),AttrTrait<>().pyByRef().readonly().ini().buttons({"Export spheres to CSV","import woo.pack; from PyQt4.QtGui import QFileDialog\nsp=woo.pack.SpherePack(); sp.fromSimulation(self.scene); csv=woo.master.tmpFilename()+'.csv'; csv=str(QFileDialog.getSaveFileName(None,'Export spheres','.'));\nif csv:\n\tsp.save(csv); print 'Saved exported spheres to',csv",""}),"Particles (each particle holds its contacts, and references associated nodes)")) \
		((shared_ptr<ContactContainer>,contacts,make_shared<ContactContainer>(),AttrTrait<>().pyByRef().readonly().ini(),"Linear view on particle contacts")) \
		((uint,loneMask,((void)":obj:`DemField.defaultLoneMask`",DemField::defaultLoneMask),,"Particle groups which have bits in loneMask in common (i.e. (A.mask & B.mask & loneMask)!=0) will not have contacts between themselves")) \
		((Vector3r,gravity,Vector3r::Zero(),,"Constant gravity acceleration")) \
		((bool,saveDead,false,AttrTrait<>().buttons({"Clear dead nodes","self.clearDead()",""}),"Save unused nodes of deleted particles, which would be otherwise removed (useful for displaying traces of deleted particles).")) \
		((vector<shared_ptr<Node>>,deadNodes,,AttrTrait<Attr::readonly>().noGui(),"List of nodes belonging to deleted particles; only used if :obj:`saveDead` is ``True``")) \
		((vector<shared_ptr<Particle>>,deadParticles,,AttrTrait<Attr::readonly>().noGui(),"Deleted particles; only used if :obj:`saveDead` is ``True``")) \
		, /* ctor */ createIndex(); postLoad(*this,NULL); /* to make sure pointers are OK */ \
		, /*py*/ \
		.def("collectNodes",&DemField::collectNodes,"Collect nodes from all particles and clumps and insert them to nodes defined for this field. Nodes are not added multiple times, even if they are referenced from different particles.") \
		.def("clearDead",&DemField::clearDead) \
		.def("nodesAppend",&DemField::pyNodesAppend,"Append given node to :obj:`nodes`, and set :obj:`DemData.linIx` to the correct value automatically.") \
		.def("nodesAppend",&DemField::pyNodesAppendList,"Append given list of nodes to :obj:`nodes`, and set :obj:`DemData.linIx` to the correct value automatically.") \
		.def("splitNode",&DemField::splitNode,(py::arg("node"),py::arg("pars"),py::arg("massMult")=NaN,py::arg("inertiaMult")=NaN),"For particles *pars*, replace their node *node* by a clone (:obj:`~woo.core.Master.deepcopy`) of this node. If *massMult* and *inertiaMult* are given, mass/inertia of both original and cloned node are multiplied by those factors. Returns the original and the new node. Both nodes will be co-incident in space. This function is used to un-share node shared by multiple particles, such as when breaking mesh apart.")  \
		.def("sceneHasField",&Field_sceneHasField<DemField>).staticmethod("sceneHasField") \
		.def("sceneGetField",&Field_sceneGetField<DemField>).staticmethod("sceneGetField"); \
		_classObj.attr("defaultMovableMask")=(int)DemField::defaultMovableMask; \
		_classObj.attr("defaultBoundaryMask")=(int)DemField::defaultBoundaryMask; \
		_classObj.attr("defaultLoneMask")=(int)DemField::defaultLoneMask; \
		_classObj.attr("defaultInletMask")=(int)DemField::defaultInletMask; \
		_classObj.attr("defaultOutletMask")=(int)DemField::defaultOutletMask;
	
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_DemField__CLASS_BASE_DOC_ATTRS_CTOR_PY);

	REGISTER_CLASS_INDEX(DemField,Field);
};
WOO_REGISTER_OBJECT(DemField);


struct Shape: public Object, public Indexable{
	WOO_DECL_LOGGER;
	// return number of nodes for this shape; derived classes must override
	virtual int numNodes() const { return -1; } 
	// checks for the right number of nodes; to be used in assertions
	bool numNodesOk() const { return numNodes()==(int)nodes.size(); }
	// check that we have the right number of nodes and that each nodes has DemData; raise python exception on failures
	void checkNodesHaveDemData() const;
	// this will be called from DemField::selfTest for each particle
	virtual void selfTest(const shared_ptr<Particle>& p){};
	// color manipulation
	Real getSignedBaseColor(){ return color-trunc(color); }
	Real getBaseColor(){ return abs(color)-trunc(abs(color)); }
	void setBaseColor(Real c){ if(isnan(c)) return; color=trunc(color)+(color<0?-1:1)*CompUtils::clamped(c,0,1); }
	bool getWire() const { return color<0; }
	void setWire(bool w){ color=(w?-1:1)*abs(color); }
	bool getHighlighted() const { return abs(color)>=1 && abs(color)<2; }
	void setHighlighted(bool h){ if(getHighlighted()==h) return; color=(color>=0?1:-1)+getSignedBaseColor(); }
	bool getVisible() const { return abs(color)<=2; }
	void setVisible(bool w){ if(getVisible()==w) return; bool hi=abs(color)>1; int sgn=(color<0?-1:1); color=sgn*((w?0:2)+getBaseColor()+(hi?1:0)); }
	// return average position of nodes, useful for rendering
	// caller must make sure that !nodes.empty()
	Vector3r avgNodePos();

	// predicate whether given point (in global coords) is inside this shape; used for samping when computing clump properties
	virtual bool isInside(const Vector3r&) const;
	virtual AlignedBox3r alignedBox() const;
	// scale all dimensions; do not update mass/inertia, the caller is responsible for that
	virtual void applyScale(Real s); 

	// set shape and geometry from raw numbers (center, radius (bounding sphere) plus array of numbers with shape-specific length)
	virtual void setFromRaw(const Vector3r& center, const Real& radius, vector<shared_ptr<Node>>& nn, const vector<Real>& raw);
	// return this shape config as raw numbers
	virtual void asRaw(Vector3r& center, Real& radius, vector<shared_ptr<Node>>&nn, vector<Real>& raw) const;
	void asRaw_helper_coordsFromNode(vector<shared_ptr<Node>>& nn, vector<Real>& raw, size_t pos, size_t nodeNum) const;
	// python version - return as tuple
	py::tuple pyAsRaw() const;
	// check that size of raw in setFromRaw matches what we expect
	void setFromRaw_helper_checkRaw_makeNodes(const vector<Real>& raw, size_t numRaw);
	// return node given coordinates; handles special (nan,nan,i) coordinate which selects from existing nodes
	// returned node added or already existing inside nn
	shared_ptr<Node> setFromRaw_helper_nodeFromCoords(vector<shared_ptr<Node>>& nn, const vector<Real>& raw, size_t pos);

	#if 0
		shared_ptr<Particle> make(const shared_ptr<Shape>& shape, const shared_ptr<Material>& mat, py::dict kwargs);
		shared_ptr<Particle> make(const shared_ptr<Shape>& shape, const shared_ptr<Material>& mat, int mask=1, const vector<shared_ptr<Node>> nodes=vector<shared_ptr<Node>>(), const Vector3r& pos=Vector3r(NaN,NaN,NaN), const Quaternionr& ori=Quaternionr::Identity(), bool fixed=false, bool wire=false, bool color=NaN, bool highlight=false, bool visible=true);
	#endif

	// update mass and inertia of this object
	virtual void updateMassInertia(const Real& density);

	// add mass and inertia for given node to m and J, in node's coordinate system
	// rotateOk indicates whether the node may rotate without changing the geometry (as for spheres or facets)
	virtual void lumpMassInertia(const shared_ptr<Node>&, Real density, Real& mass, Matrix3r& I, bool& rotateOk);
	py::tuple pyLumpMassInertia(const shared_ptr<Node>& n, Real density);


	// return equivalent radius of the particle, or NaN if not defined by the shape
	virtual Real equivRadius() const { return NaN; }
	virtual Real volume() const { return NaN; }
	#define woo_dem_Shape__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		Shape,Object,"Particle geometry", \
		((shared_ptr<Bound>,bound,,,"Bound of the particle, for use by collision detection only")) \
		((vector<shared_ptr<Node> >,nodes,,,"Nodes associated with this particle")) \
		((Real,color,Mathr::UnitRandom(),,"Normalized color for rendering; negative values render with wire (rather than solid), :math:`|\\text{color}|`>2 means invisible. (use *wire*, *hi* and *visible* to manipulate those)")) \
		,/*ctor*/ createIndex(); \
		,/*py*/ \
			.add_property("wire",&Shape::getWire,&Shape::setWire) \
			.add_property("hi",&Shape::getHighlighted,&Shape::setHighlighted) \
			.add_property("visible",&Shape::getVisible,&Shape::setVisible) \
			.add_property("equivRadius",&Shape::equivRadius) \
			.add_property("volume",&Shape::volume) \
			.def("asRaw",&Shape::pyAsRaw) \
			.def("setFromRaw",&Shape::setFromRaw) \
			.def("isInside",&Shape::isInside) \
			.def("lumpMassInertia",&Shape::pyLumpMassInertia) \
			WOO_PY_TOPINDEXABLE(Shape) \
			; \
			woo::converters_cxxVector_pyList_2way<shared_ptr<Shape>>();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_Shape__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	REGISTER_INDEX_COUNTER(Shape);
};
WOO_REGISTER_OBJECT(Shape);

struct Material: public Object, public Indexable{
	// XXX: is createIndex() called here at all??
	#define woo_dem_Material__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		Material,Object,ClassTrait().doc("Particle material").section("Material properties","TODO",{"MatState"}), \
		((Real,density,1000,AttrTrait<>().densityUnit(),"Density")) \
		((int,id,-1,AttrTrait<>().noGui(),"Some number identifying this material; used with MatchMaker objects, useless otherwise")) \
		,/*ctor*/createIndex(); \
		,/*py*/ WOO_PY_TOPINDEXABLE(Material); \
		woo::converters_cxxVector_pyList_2way<shared_ptr<Material>>();
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_Material__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	REGISTER_INDEX_COUNTER(Material);
};
WOO_REGISTER_OBJECT(Material);

struct Bound: public Object, public Indexable{
	Vector3r pyMin(){ return box.min(); }
	Vector3r pyMax(){ return box.max(); }
	#define woo_dem_Bound__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY \
		Bound,Object,"Object bounding the associated body.", \
		((AlignedBox3r,box,AlignedBox3r(),AttrTrait<Attr::noSave>().readonly().lenUnit(),"Axis-aligned bounding box.")) \
		,/*ini*/((min,box.min()))((max,box.max())) \
		,/*ctor*/ createIndex(); \
		,/*py*/ .add_property("min",&Bound::pyMin).add_property("max",&Bound::pyMax) WOO_PY_TOPINDEXABLE(Bound); 
	
	WOO_DECL__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY(woo_dem_Bound__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY);

	// must come after declaration of box, as that is the order of initialization
	Vector3r& min; Vector3r& max;

	REGISTER_INDEX_COUNTER(Bound);
};
WOO_REGISTER_OBJECT(Bound);

// }}; /* woo::dem */

