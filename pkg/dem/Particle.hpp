#pragma once
#include<yade/core/Omega.hpp>
#include<yade/core/Field.hpp>
#include<yade/core/Scene.hpp>
#include<yade/core/Field-templates.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/ContactContainer.hpp>


// namespace yade{namespace dem{

class Particle;
class Contact;
class Material;
class Bound;
class Shape;
class ParticleContainer;

class ScalarRange;

struct Particle: public Serializable{
	shared_ptr<Contact> findContactWith(const shared_ptr<Particle>& other);
	typedef ParticleContainer::id_t id_t;
	typedef std::map<id_t,shared_ptr<Contact> > MapParticleContact;
	void checkNodes(bool dyn=true, bool checkOne=true) const;


	Vector3r& getPos() const; void setPos(const Vector3r&);
	Quaternionr& getOri() const; void setOri(const Quaternionr&);
	Vector3r& getVel() const; void setVel(const Vector3r&);
	Vector3r& getAngVel() const; void setAngVel(const Vector3r&);
	Real getEk_any(bool trans, bool rot) const;
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
	// getRefPos creates refPos if it does not exist, and initializes to current pos
	Vector3r& getRefPos(); void setRefPos(const Vector3r&);
	std::vector<shared_ptr<Node> > getNodes();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Particle,Serializable,"Particle in DEM",
		((id_t,id,-1,Attr::readonly,"Index in DemField::particles"))
		((unsigned int,mask,1,,"Bitmask for collision detection and other (group 1 by default)"))
		((shared_ptr<Shape>,shape,,,"Geometrical configuration of the particle"))
		((shared_ptr<Material>,material,,,"Material of the particle"))
		((MapParticleContact,contacts,,Attr::hidden,"Contacts of this particle, indexed by id of the other particle."))
		// ((int,flags,0,Attr::hidden,"Various flags, only individually accesible from Python"))
		, /*ctor*/
		, /*py*/
			.add_property("contacts",&Particle::pyContacts)
			.add_property("con",&Particle::pyCon)
			.add_property("tacts",&Particle::pyTacts)
		.add_property("pos",py::make_function(&Particle::getPos,py::return_internal_reference<>()),py::make_function(&Particle::setPos))
		.add_property("refPos",py::make_function(&Particle::getRefPos,py::return_internal_reference<>()),py::make_function(&Particle::setRefPos))
		.add_property("ori",py::make_function(&Particle::getOri,py::return_internal_reference<>()),py::make_function(&Particle::setOri))
		.add_property("vel",py::make_function(&Particle::getVel,py::return_internal_reference<>()),py::make_function(&Particle::setVel))
		.add_property("angVel",py::make_function(&Particle::getAngVel,py::return_internal_reference<>()),py::make_function(&Particle::setAngVel))
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
REGISTER_SERIALIZABLE(Particle);


class DemData: public NodeData{
	boost::mutex lock; // used by applyForceTorque
public:
	// bits for flags
	enum {
		DOF_NONE=0,DOF_X=1,DOF_Y=2,DOF_Z=4,DOF_RX=8,DOF_RY=16,DOF_RZ=32,
		CLUMP_CLUMPED=64,CLUMP_CLUMP=128,ENERGY_SKIP=256
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
	bool isBlockedAll()  const { return (flags&DOF_ALL)==DOF_ALL; }
	bool isBlockedAxisDOF(int axis, bool rot) const { return (flags & axisDOF(axis,rot)); }

	// predicates and setters for clumps
	bool isClumped() const { return flags&CLUMP_CLUMPED; }
	bool isClump()   const { return flags&CLUMP_CLUMP; }
	bool isNoClump() const { return !isClumped() && !isClump(); }
	void setClumped(){ flags|=CLUMP_CLUMPED; flags&=~CLUMP_CLUMP; }
	void setClump()  { flags|=CLUMP_CLUMP; flags&=~CLUMP_CLUMPED; }
	void setNoClump(){ flags&=~(CLUMP_CLUMP | CLUMP_CLUMPED); }

	// predicates and setters for skipping this node in energy calculations
	bool isEnergySkip() const { return flags&ENERGY_SKIP; }
	void setEnergySkip(bool skip) { if(!skip) flags&=~ENERGY_SKIP; else flags|=ENERGY_SKIP; }

	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	void addForceTorque(const Vector3r& f, const Vector3r& t=Vector3r::Zero()){ boost::mutex::scoped_lock l(lock); force+=f; torque+=t; }

	bool isAspherical() const{ return !((inertia[0]==inertia[1] && inertia[1]==inertia[2])); }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(DemData,NodeData,"Dynamic state of node.",
		((Vector3r,vel,Vector3r::Zero(),,"Linear velocity."))
		((Vector3r,angVel,Vector3r::Zero(),,"Angular velocity."))
		((Real,mass,0,,"Associated mass."))
		((Vector3r,inertia,Vector3r::Zero(),,"Inertia along local (principal) axes"))
		((Vector3r,force,Vector3r::Zero(),,"Applied force"))
		((Vector3r,torque,Vector3r::Zero(),,"Applied torque"))
		((Vector3r,angMom,Vector3r::Zero(),,"Angular momentum (used with the aspherical integrator)"))
		((unsigned,flags,0,Attr::readonly,"Bit flags storing blocked DOFs, clump status"))
		, /*ctor*/
		, /*py*/ .add_property("blocked",&DemData::blocked_vec_get,&DemData::blocked_vec_set,"Degress of freedom where linear/angular velocity will be always constant (equal to zero, or to an user-defined value), regardless of applied force/torque. String that may contain 'xyzXYZ' (translations and rotations).")
		.add_property("clump",&DemData::isClump).add_property("clumped",&DemData::isClumped).add_property("noClump",&DemData::isNoClump).add_property("energySkip",&DemData::isEnergySkip,&DemData::setEnergySkip)
		.def("_getDataOnNode",&Node::pyGetData<DemData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<DemData>).staticmethod("_setDataOnNode");
	);
};
REGISTER_SERIALIZABLE(DemData);

template<> struct NodeData::Index<DemData>{enum{value=Node::ST_DEM};};

struct DemField: public Field{
	int collectNodes();

	//template<> bool sceneHasField<DemField>() const;
	//template<> shared_ptr<DemField> sceneGetField<DemField>() const;

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(DemField,Field,"Field describing a discrete element assembly. Each body references (possibly many) nodes by their index in :yref:`Field.nodes` and :yref:`Field.nodalData`. ",
		((ParticleContainer,particles,,(Attr::pyByRef|Attr::readonly),"Particles (each particle holds its contacts, and references associated nodes)"))
		((ContactContainer,contacts,,(Attr::pyByRef|Attr::readonly),"Linear view on particle contacts"))
		((vector<shared_ptr<Node>>,clumps,,Attr::readonly,"Nodes which define clumps; only manipulated from clump-related user-routines, not directly."))
		((int,loneMask,0,,"Particle groups which have bits in loneMask in common (i.e. (A.mask & B.mask & loneMask)!=0) will not have contacts between themselves"))
		, /* ctor */ createIndex(); particles.dem=this; contacts.dem=this; contacts.particles=&particles;
		, /*py*/
		.def("collectNodes",&DemField::collectNodes,"Collect nodes from all particles and clumps and insert them to nodes defined for this field. Nodes are not added multiple times, even if they are referenced from different particles / clumps.")
		.def("sceneHasField",&Field_sceneHasField<DemField>).staticmethod("sceneHasField")
		.def("sceneGetField",&Field_sceneGetField<DemField>).staticmethod("sceneGetField")
	);
	REGISTER_CLASS_INDEX(DemField,Field);
};
REGISTER_SERIALIZABLE(DemField);

class CGeom: public Serializable,public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CGeom,Serializable,"Geometrical configuration of contact",
		((shared_ptr<Node>,node,new Node,,"Local coordinates definition."))
		,/*ctor*/,/*py*/YADE_PY_TOPINDEXABLE(CGeom)
	);
	REGISTER_INDEX_COUNTER(CGeom);
};
REGISTER_SERIALIZABLE(CGeom);

class CPhys: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CPhys,Serializable,"Physical properties of contact.",
		/*attrs*/
		((Vector3r,force,Vector3r::Zero(),,"Force applied on the first particle in the contact"))
		((Vector3r,torque,Vector3r::Zero(),,"Torque applied on the first particle in the contact"))
		,/*ctor*/ createIndex(); ,/*py*/YADE_PY_TOPINDEXABLE(CPhys)
	);
	REGISTER_INDEX_COUNTER(CPhys);
};
REGISTER_SERIALIZABLE(CPhys);

class CData: public Serializable{
	YADE_CLASS_BASE_DOC_ATTRS(CData,Serializable,"Optional data stored in the contact by the Law functor.",
		/* attrs */
	);
};
REGISTER_SERIALIZABLE(CData);


struct Contact: public Serializable{
	bool isReal() const { return geom&&phys; }
	bool isFresh(Scene* s){ return s->step==stepMadeReal; }
	void swapOrder();
	void reset();
	// return -1 or +1 depending on whether the particle passed to us is pA or pB
	int forceSign(const shared_ptr<Particle>& p) const { return p==pA?1:-1; }
	/* return force and torque in global coordinates which act at contact point C located at branch from node nodeI of particle.
	Contact force is reversed automatically if particle==pB.
	Force and torque at node itself are  F and branch.cross(F)+T respectively.
	Branch takes periodicity (cellDist) in account.
	See In2_Sphere_Elastmat::go for its use.  */
	std::tuple<Vector3r,Vector3r,Vector3r> getForceTorqueBranch(const shared_ptr<Particle>&, int nodeI, Scene* scene);
	// return position vector between pA and pB, taking in account PBC's; both must be uninodal
	Vector3r dPos(const Scene* s) const;
	Vector3r dPos_py() const{ return dPos(Omega::instance().getScene().get()); }
	Real dist_py() const { return dPos_py().norm(); }
	Particle::id_t pyId1() const;
	Particle::id_t pyId2() const;
	Vector2i pyIds() const;
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Contact,Serializable,"Contact in DEM",
		((shared_ptr<CGeom>,geom,,Attr::readonly,"Contact geometry"))
		((shared_ptr<CPhys>,phys,,Attr::readonly,"Physical properties of contact"))
		((shared_ptr<CData>,data,,Attr::readonly,"Optional data stored by the functor for its own use"))
		((shared_ptr<Particle>,pA,,Attr::readonly,"First particle of the contact"))
		((shared_ptr<Particle>,pB,,Attr::readonly,"Second particle of the contact"))
		((Vector3i,cellDist,Vector3i::Zero(),Attr::readonly,"Distace in the number of periodic cells by which pB must be shifted to get to the right relative position."))
		#ifdef YADE_OPENGL
			((Real,color,0,,"(Normalized) color value for this contact"))
		#endif
		((int,stepLastSeen,-1,Attr::hidden,""))
		((int,stepMadeReal,-1,Attr::hidden,""))
		((size_t,linIx,0,(Attr::readonly|Attr::noGui),"Position in the linear view (ContactContainer)"))
		, /*ctor*/
		, /*py*/ .add_property("id1",&Contact::pyId1).add_property("id2",&Contact::pyId2).add_property("real",&Contact::isReal).add_property("ids",&Contact::pyIds)
		.def("dPos",&Contact::dPos_py,"Return position difference vector pB-pA, taking `Contact.cellDist` in account properly. Both particles must be uninodal, exception is raised otherwise.")
		.def("dist",&Contact::dist_py,"Shorthand for dPos.norm().")
	);
};
REGISTER_SERIALIZABLE(Contact);

struct Shape: public Serializable, public Indexable{
	virtual bool numNodesOk() const { return true; } // checks for the right number of nodes; to be used in assertions
	// return average position of nodes, useful for rendering
	// caller must make sure that !nodes.empty()
	Real getSignedBaseColor(){ return color-trunc(color); }
	Real getBaseColor(){ return abs(color)-trunc(abs(color)); }
	bool getWire() const { return color<0; }
	void setWire(bool w){ color=(w?-1:1)*abs(color); }
	bool getHighlighted() const { return abs(color)>=1 && abs(color)<2; }
	void setHighlighted(bool h){ if(getHighlighted()==h) return; color=(color>=0?1:-1)+getSignedBaseColor(); }
	bool getVisible() const { return abs(color)<=2; }
	void setVisible(bool w){ if(getVisible()==w) return; bool hi=abs(color)>1; int sgn=(color<0?-1:1); color=sgn*((w?0:2)+getBaseColor()+(hi?1:0)); }
	Vector3r avgNodePos();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Shape,Serializable,"Particle geometry",
		((shared_ptr<Bound>,bound,,,"Bound of the particle, for use by collision detection only"))
		((vector<shared_ptr<Node> >,nodes,,,"Nodes associated with this particle"))
		((Real,color,Mathr::UnitRandom(),,"Normalized color for rendering; negative values render with wire (rather than solid), |color|>2 means invisible. (use *wire*, *hi* and *visible* to manipulate those)"))
		,/*ctor*/,/*py*/
			.add_property("wire",&Shape::getWire,&Shape::setWire)
			.add_property("hi",&Shape::getHighlighted,&Shape::setHighlighted)
			.add_property("visible",&Shape::getVisible,&Shape::setVisible)
			YADE_PY_TOPINDEXABLE(Shape)
	);
	REGISTER_INDEX_COUNTER(Shape);
};
REGISTER_SERIALIZABLE(Shape);

class Material: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Material,Serializable,"Particle material",
		((Real,density,NaN,,"Material density"))
		((int,id,-1,,"Some number identifying this material; used with MatchMaker objects, otherwise useless"))
		,/*ctor*/,/*py*/ YADE_PY_TOPINDEXABLE(Material);
	);
	REGISTER_INDEX_COUNTER(Material);
};
REGISTER_SERIALIZABLE(Material);

class Bound: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Bound,Serializable,"Object bounding the associated body.",
		// ((Vector3r,color,Vector3r(1,1,1),,"Color for rendering this object"))
		((Vector3r,min,Vector3r(NaN,NaN,NaN),(Attr::noSave|Attr::readonly),"Lower corner of box containing this bound"))
		((Vector3r,max,Vector3r(NaN,NaN,NaN),(Attr::noSave|Attr::readonly),"Lower corner of box containing this bound"))
		,/* ctor*/,	/*py*/ YADE_PY_TOPINDEXABLE(Bound)
	);
	REGISTER_INDEX_COUNTER(Bound);
};
REGISTER_SERIALIZABLE(Bound);


// }}; /* yade::dem */


