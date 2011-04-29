#pragma once
#include<yade/core/Field.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/ContactContainer.hpp>

// namespace yade{namespace dem{

class Particle;
class Contact;
class Material;
class Bound;
class Shape;
class ParticleContainer;

struct Particle: public Serializable{
	shared_ptr<Contact> findContactWith(const shared_ptr<Particle>& other);
	typedef ParticleContainer::id_t id_t;
	typedef std::map<id_t,shared_ptr<Contact> > MapParticleContact;
	void checkOneNode(bool dyn=true);

	Vector3r& getPos(); void setPos(const Vector3r&);
	Quaternionr& getOri(); void setOri(const Quaternionr&);
	Vector3r& getVel(); void setVel(const Vector3r&);
	Vector3r& getAngVel(); void setAngVel(const Vector3r&);
	Vector3r getForce();
	Vector3r getTorque();
	py::dict pyContacts();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Particle,Serializable,"Particle in DEM",
		((id_t,id,-1,Attr::readonly,"Index in DemField::particles"))
		((shared_ptr<Shape>,shape,,,"Geometrical configuration of the particle"))
		((shared_ptr<Material>,material,,,"Material of the particle"))
		((MapParticleContact,contacts,,Attr::hidden,"Contacts of this particle, indexed by id of the other particle."))
		, /*ctor*/
		, /*py*/ .add_property("contacts",&Particle::pyContacts)
		.add_property("pos",py::make_function(&Particle::getPos,py::return_internal_reference<>()),py::make_function(&Particle::setPos))
		.add_property("ori",py::make_function(&Particle::getOri,py::return_internal_reference<>()),py::make_function(&Particle::setOri))
		.add_property("vel",py::make_function(&Particle::getVel,py::return_internal_reference<>()),py::make_function(&Particle::setVel))
		.add_property("angVel",py::make_function(&Particle::getAngVel,py::return_internal_reference<>()),py::make_function(&Particle::setAngVel))
		.add_property("f",&Particle::getForce)
		.add_property("t",&Particle::getTorque)
	);
};
REGISTER_SERIALIZABLE(Particle);

struct DemField: public Field{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(DemField,Field,"Field describing a discrete element assembly. Each body references (possibly many) nodes by their index in :yref:`Field.nodes` and :yref:`Field.nodalData`. ",
		((ParticleContainer,particles,,(Attr::pyByRef|Attr::readonly),"Particles (each particle holds its contacts, and references associated nodes)"))
		((ContactContainer,contacts,,(Attr::pyByRef|Attr::readonly),"Linear view on particle contacts"))
		, /* ctor */ createIndex(); particles.dem=this; contacts.dem=this; contacts.particles=&particles;
		, /*py*/
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
		,/*ctor*/,/*py*/YADE_PY_TOPINDEXABLE(CPhys)
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
	void swapOrder();
	void reset();
	Particle::id_t pyId1();
	Particle::id_t pyId2();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Contact,Serializable,"Contact in DEM",
		((shared_ptr<CGeom>,geom,,Attr::readonly,"Contact geometry"))
		((shared_ptr<CPhys>,phys,,Attr::readonly,"Physical properties of contact"))
		((shared_ptr<CData>,data,,Attr::readonly,"Optional data stored by the functor for its own use"))
		((shared_ptr<Particle>,pA,,Attr::readonly,"First particle of the contact"))
		((shared_ptr<Particle>,pB,,Attr::readonly,"Second particle of the contact"))
		((Vector3i,cellDist,Vector3i::Zero(),Attr::readonly,"Distace in the number of periodic cells by which pB must be shifted to get to the right relative position."))
		((int,stepLastSeen,-1,Attr::hidden,""))
		((int,stepMadeReal,-1,Attr::hidden,""))
		((size_t,linIx,0,Attr::hidden,"Position in the linear view (ContactContainer)"))
		, /*ctor*/
		, /*py*/ .add_property("id1",&Contact::pyId1).add_property("id2",&Contact::pyId2)
	);
};
REGISTER_SERIALIZABLE(Contact);

struct Shape: public Serializable, public Indexable{
	virtual bool numNodesOk(){ return true; } // checks for the right number of nodes; to be used in assertions
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Shape,Serializable,"Particle geometry",
		((shared_ptr<Bound>,bound,,,"Bound of the particle, for use by collision detection only"))
		((vector<shared_ptr<Node> >,nodes,,,"Nodes associated with this particle"))
		((bool,wire,false,,"Rendering hint"))
		((Vector3r,color,Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom()),,"Color for rendering"))
		,/*ctor*/,/*py*/ YADE_PY_TOPINDEXABLE(Shape);
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



