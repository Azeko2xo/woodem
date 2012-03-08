#pragma once
#include<yade/core/Omega.hpp>
#include<yade/core/Dispatcher.hpp>
#include<yade/pkg/dem/Particle.hpp>

#ifdef YADE_OPENGL
#include<yade/pkg/gl/Functors.hpp>
#endif

class Aabb: public Bound{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Aabb,Bound,"Axis-aligned bounding box, for use with :yref:`InsertionSortCollider`.",/*attrs*/
		((vector<Vector3r>,nodeLastPos,,Attr::readonly,"Node positions when bbox was last updated."))
		((Real,maxD2,0,Attr::readonly,"Maximum allowed squared distance for nodal displacements (i.e. how much was the bbox enlarged last time)"))
		,
		/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(Aabb,Bound);
};
REGISTER_SERIALIZABLE(Aabb);

#ifdef YADE_OPENGL
struct Gl1_Aabb: public GlBoundFunctor{
	virtual void go(const shared_ptr<Bound>&);
	RENDERS(Aabb);
	YADE_CLASS_BASE_DOC(Gl1_Aabb,GlBoundFunctor,"Render Axis-aligned bounding box (:yref:`Aabb`).");
};
REGISTER_SERIALIZABLE(Gl1_Aabb);
#endif


class BoundFunctor: public Functor1D</*dispatch types*/ Shape,/*return type*/ void, /*argument types*/ TYPELIST_1(const shared_ptr<Shape>&)>{
	YADE_CLASS_BASE_DOC(BoundFunctor,Functor,"Functor for creating/updating :yref:`Body::bound`.");
};
REGISTER_SERIALIZABLE(BoundFunctor);

struct BoundDispatcher: public Dispatcher1D</* functor type*/ BoundFunctor>{
	void run();
	YADE_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(BoundDispatcher,BoundFunctor,/*optional doc*/,
		/*additional attrs*/
		,/*ctor*/,/*py*/
	);
};
REGISTER_SERIALIZABLE(BoundDispatcher);


class Collider: public GlobalEngine{
	public:
		/*! Probe the Bound on a bodies presence. Returns list of body ids with which there is potential overlap. */
		//virtual vector<Body::id_t> probeBoundingVolume(const Bound&){throw;}

		/*!
		Tell whether given bodies may interact, for other than spatial reasons.
		
		Concrete collider implementations should call this function if the bodies are in potential interaction geometrically. */
		bool mayCollide(const shared_ptr<Particle>&, const shared_ptr<Particle>&, const DemField*);
		/*!
		Invalidate all persistent data (if the collider has any), forcing reinitialization at next run.
		The default implementation does nothing, colliders should override it if it is applicable.
		*/
		virtual void invalidatePersistentData(){}
		// ctor with functors for the integrated BoundDispatcher
		virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);

	virtual void getLabeledObjects(std::map<std::string,py::object>& m);
	YADE_CLASS_BASE_DOC_ATTRS(Collider,GlobalEngine,"Abstract class for finding spatial collisions between bodies. \n\n.. admonition:: Special constructor\n\n\tDerived colliders (unless they override ``pyHandleCustomCtorArgs``) can be given list of :yref:`BoundFunctors <BoundFunctor>` which is used to initialize the internal :yref:`boundDispatcher <Collider.boundDispatcher>` instance.",
		((shared_ptr<BoundDispatcher>,boundDispatcher,new BoundDispatcher,Attr::readonly,":yref:`BoundDispatcher` object that is used for creating :yref:`bounds <Body.bound>` on collider's request as necessary."))
	);
};
REGISTER_SERIALIZABLE(Collider);


