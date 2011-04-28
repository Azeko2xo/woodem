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
	py::dict pyContacts();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Particle,Serializable,"Particle in DEM",
		((id_t,id,-1,Attr::readonly,"Index in DemField::particles"))
		((shared_ptr<Shape>,shape,,,"Geometrical configuration of the particle"))
		((shared_ptr<Material>,material,,,"Material of the particle"))
		((MapParticleContact,contacts,,Attr::hidden,"Contacts of this particle, indexed by id of the other particle."))
		, /*ctor*/
		, /*py*/ .add_property("contacts",&Particle::pyContacts)
	);
};
REGISTER_SERIALIZABLE(Particle);

struct DemField: public Field{
	__attribute__((deprecated)) void addContact(shared_ptr<Contact> c){ return contacts.add(c); }
	__attribute__((deprecated)) void removeContact(shared_ptr<Contact> c){ return contacts.remove(c); }
	__attribute__((deprecated)) shared_ptr<Contact> findContact(Particle::id_t idA, Particle::id_t idB){ return contacts.find(idA,idB); }
	__attribute__((deprecated)) void removePending(){ /* stub */ }

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
		/*no attrs*/,/*ctor*/,/*py*/YADE_PY_TOPINDEXABLE(CGeom)
	);
	REGISTER_INDEX_COUNTER(CGeom);
};
REGISTER_SERIALIZABLE(CGeom);

class CPhys: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CPhys,Serializable,"Physical properties of contact.",
		/*attrs*/,/*ctor*/,/*py*/YADE_PY_TOPINDEXABLE(CPhys)
	);
	REGISTER_INDEX_COUNTER(CPhys);
};
REGISTER_SERIALIZABLE(CPhys);


struct Contact: public Serializable{
	bool isReal() const { return geom&&phys; }
	void swapOrder();
	void reset();
	Particle::id_t pyId1();
	Particle::id_t pyId2();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Contact,Serializable,"Contact in DEM",
		((shared_ptr<Node>,node,,Attr::readonly,"Node at the contact point"))
		((shared_ptr<CGeom>,geom,,Attr::readonly,"Contact geometry"))
		((shared_ptr<CPhys>,phys,,Attr::readonly,"Physical properties of contact"))
		//((Vector3r,force,Vector3r::Zero(),,"Force applied on the first particle in the contact"))
		//((Vector3r,torque,Vector3r::Zero(),,"Torque applied on the first particle in the contact"))
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
		,/*ctor*/,/*py*/ YADE_PY_TOPINDEXABLE(Shape);
	);
	REGISTER_INDEX_COUNTER(Shape);
};
REGISTER_SERIALIZABLE(Shape);

class Material: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Material,Serializable,"Particle material",
		((Real,density,NaN,,"Material density"))
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



