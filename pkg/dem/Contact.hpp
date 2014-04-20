/* all declarations for contacts are in the woo/pkg/dem/Particle.hpp header:
	declarations contain boost::python code, which need full declaration,
	not just forwards.
*/
#pragma once

#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/Particle.hpp> // for Particle::id_t

struct Particle;
struct Scene;
struct Node;


struct CGeom: public Object,public Indexable{
	// XXX: is createIndex() called here at all??
	#define woo_dem_CGeom__CLASS_BASE_DOC_ATTRS_PY \
		CGeom,Object,"Geometrical configuration of contact", \
		((shared_ptr<Node>,node,new Node,,"Local coordinates definition.")) \
		,/*py*/WOO_PY_TOPINDEXABLE(CGeom);
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_CGeom__CLASS_BASE_DOC_ATTRS_PY);
	REGISTER_INDEX_COUNTER(CGeom);
};
WOO_REGISTER_OBJECT(CGeom);

struct CPhys: public Object, public Indexable{
	#define woo_dem_CPhys__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		CPhys,Object,"Physical properties of contact.", \
		/*attrs*/ \
		((Vector3r,force,Vector3r::Zero(),AttrTrait<>().forceUnit(),"Force applied on the first particle in the contact")) \
		((Vector3r,torque,Vector3r::Zero(),AttrTrait<>().torqueUnit(),"Torque applied on the first particle in the contact")) \
		,/*ctor*/ createIndex(); ,/*py*/WOO_PY_TOPINDEXABLE(CPhys)
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_CPhys__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	REGISTER_INDEX_COUNTER(CPhys);
};
WOO_REGISTER_OBJECT(CPhys);

struct CData: public Object{
	#define woo_dem_CData__CLASS_BASE_DOC CData,Object,"Optional data stored in the contact by the Law functor."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_CData__CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(CData);

struct Contact: public Object{
	bool isReal() const { return geom&&phys; }
	bool isColliding() const { return stepCreated>=0; }
	void setNotColliding(){ stepCreated=-1; }
	bool isFresh(Scene* s){ return s->step==stepCreated; }
	bool pyIsFresh(shared_ptr<Scene> s){ return s->step==stepCreated; }
	void swapOrder();
	void reset();
	// return -1 or +1 depending on whether the particle passed to us is pA or pB
	int forceSign(const shared_ptr<Particle>& p) const { return p.get()==leakPA()?1:-1; }
	// return 0 or 1 depending on whether the particle passed is pA or pB
	short pIndex(const shared_ptr<Particle>& p) const { return pIndex(p.get()); }
	short pIndex(const Particle* p) const { return p==leakPA()?0:1; }
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
	Particle* leakOther(const Particle* p) const { assert(p==leakPA() || p==leakPB()); return (p!=leakPA()?leakPA():leakPB()); }
	shared_ptr<Particle> pyPA() const { return pA.lock(); }
	shared_ptr<Particle> pyPB() const { return pB.lock(); }
	#ifdef WOO_OPENGL
		#define woo_dem_Contact__OPENGL__color ((Real,color,0,,"(Normalized) color value for this contact"))
	#else
		#define woo_dem_Contact__OPENGL__color
	#endif

	#define woo_dem_Contact__CLASS_BASE_DOC_ATTRS_PY \
		Contact,Object,"Contact in DEM", \
		((shared_ptr<CGeom>,geom,,AttrTrait<Attr::readonly>(),"Contact geometry")) \
		((shared_ptr<CPhys>,phys,,AttrTrait<Attr::readonly>(),"Physical properties of contact")) \
		((shared_ptr<CData>,data,,AttrTrait<Attr::readonly>(),"Optional data stored by the functor for its own use")) \
		/* these two are overridden below because of weak_ptr */  \
		((weak_ptr<Particle>,pA,,AttrTrait<Attr::readonly>(),"First particle of the contact")) \
		((weak_ptr<Particle>,pB,,AttrTrait<Attr::readonly>(),"Second particle of the contact")) \
		((Vector3i,cellDist,Vector3i::Zero(),AttrTrait<Attr::readonly>(),"Distace in the number of periodic cells by which pB must be shifted to get to the right relative position.")) \
		woo_dem_Contact__OPENGL__color \
		((int,stepCreated,-1,AttrTrait<Attr::hidden>(),"Step in which this contact was created by the collider, or step in which it was made real (if geom and phys exist). This number is NOT reset by Contact::reset(). If negative, it means the collider does not want to keep this contact around anymore (this happens if the contact is real but there is no overlap anymore).")) \
		((Real,minDist00Sq,-1,AttrTrait<Attr::hidden>(),"Minimum distance between nodes[0] of both shapes so that the contact can exist. Set in ContactLoop by geometry functor once, and is used to check for possible contact without having to call the functor. If negative, not used. Currently, only Sphere-Sphere contacts use this information.")) \
		((int,stepLastSeen,-1,AttrTrait<Attr::hidden>(),"")) \
		((size_t,linIx,0,AttrTrait<Attr::readonly>().noGui(),"Position in the linear view (ContactContainer)")) \
		, /*py*/ .add_property("id1",&Contact::pyId1).add_property("id2",&Contact::pyId2).add_property("real",&Contact::isReal).add_property("ids",&Contact::pyIds) \
		.def("dPos",&Contact::dPos_py,"Return position difference vector pB-pA, taking `Contact.cellDist` in account properly. Both particles must be uninodal, exception is raised otherwise.") \
		.def("dist",&Contact::dist_py,"Shorthand for dPos.norm().") \
		.add_property("pA",&Contact::pyPA,"First particle of the contact") \
		.add_property("pB",&Contact::pyPB,"Second particle of the contact") \
		.def("resetPhys",&Contact::pyResetPhys,"Set *phys* to *None* (to force its re-evaluation)") \
		.def("isFresh",&Contact::pyIsFresh,(py::arg("scene")),"Say whether this contact has just been created. Equivalent to ``C.stepCreated==scene.step``.")

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Contact__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Contact);


