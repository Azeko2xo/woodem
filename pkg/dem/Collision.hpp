#pragma once
#include<yade/pkg/dem/Particle.hpp>

class Aabb: public Bound{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Aabb,Bound,"Axis-aligned bounding box, for use with :yref:`InsertionSortCollider`. (This class is quasi-redundant since min,max are already contained in :yref:`Bound` itself. That might change at some point, though.)",/*attrs*/,/*ctor*/createIndex(););
	REGISTER_CLASS_INDEX(Aabb,Bound);
};
REGISTER_SERIALIZABLE(Aabb);


class BoundFunctor: public Functor1D</*dispatch types*/ Shape,/*return type*/ void, /*argument types*/ TYPELIST_1(const shared_ptr<Shape>&)>{
	YADE_CLASS_BASE_DOC(BoundFunctor,Functor,"Functor for creating/updating :yref:`Body::bound`.");
};
REGISTER_SERIALIZABLE(BoundFunctor);

struct BoundDispatcher: public Dispatcher1D</* functor type*/ BoundFunctor>{
	void action();
	YADE_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(BoundDispatcher,BoundFunctor,/*optional doc*/,
		/*additional attrs*/
		((Real,sweepDist,0,,"Distance by which enlarge all bounding boxes, to prevent collider from being run at every step (only should be changed by the collider)."))
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
		bool mayCollide(const shared_ptr<Particle>&, const shared_ptr<Particle>&);
		/*!
		Invalidate all persistent data (if the collider has any), forcing reinitialization at next run.
		The default implementation does nothing, colliders should override it if it is applicable.
		*/
		virtual void invalidatePersistentData(){}
		// ctor with functors for the integrated BoundDispatcher
		virtual void pyHandleCustomCtorArgs(python::tuple& t, python::dict& d);
	YADE_CLASS_BASE_DOC_ATTRS(Collider,GlobalEngine,"Abstract class for finding spatial collisions between bodies. \n\n.. admonition:: Special constructor\n\n\tDerived colliders (unless they override ``pyHandleCustomCtorArgs``) can be given list of :yref:`BoundFunctors <BoundFunctor>` which is used to initialize the internal :yref:`boundDispatcher <Collider.boundDispatcher>` instance.",
		((shared_ptr<BoundDispatcher>,boundDispatcher,new BoundDispatcher,Attr::readonly,":yref:`BoundDispatcher` object that is used for creating :yref:`bounds <Body.bound>` on collider's request as necessary."))
	);
};
REGISTER_SERIALIZABLE(Collider);
