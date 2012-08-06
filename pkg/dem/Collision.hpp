#pragma once
#include<woo/core/Dispatcher.hpp>
#include<woo/pkg/dem/Particle.hpp>

#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
#endif

class Aabb: public Bound{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(Aabb,Bound,"Axis-aligned bounding box, for use with `InsertionSortCollider`.",/*attrs*/
		((vector<Vector3r>,nodeLastPos,,AttrTrait<>(Attr::readonly).lenUnit(),"Node positions when bbox was last updated."))
		((Real,maxD2,0,AttrTrait<>(Attr::readonly).unit("m²").noGui(),"Maximum allowed squared distance for nodal displacements (i.e. how much was the bbox enlarged last time)"))
		,
		/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(Aabb,Bound);
};
REGISTER_SERIALIZABLE(Aabb);

#ifdef WOO_OPENGL
struct Gl1_Aabb: public GlBoundFunctor{
	virtual void go(const shared_ptr<Bound>&);
	RENDERS(Aabb);
	WOO_CLASS_BASE_DOC(Gl1_Aabb,GlBoundFunctor,"Render Axis-aligned bounding box (:ref:`AAbb`).");
};
REGISTER_SERIALIZABLE(Gl1_Aabb);
#endif


class BoundFunctor: public Functor1D</*dispatch types*/ Shape,/*return type*/ void, /*argument types*/ TYPELIST_1(const shared_ptr<Shape>&)>{
	WOO_CLASS_BASE_DOC(BoundFunctor,Functor,"Functor for creating/updating :ref:`woo.dem.Bound`.");
};
REGISTER_SERIALIZABLE(BoundFunctor);

struct BoundDispatcher: public Dispatcher1D</* functor type*/ BoundFunctor>{
	void run();
	WOO_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(BoundDispatcher,BoundFunctor,/*optional doc*/,
		/*additional attrs*/
		,/*ctor*/,/*py*/
	);
};
REGISTER_SERIALIZABLE(BoundDispatcher);


class Collider: public GlobalEngine{
	public:
		/*! Probe the Aabb on particle's presence. Returns list of body ids with which there is potential overlap. */
		virtual vector<Particle::id_t> probeAabb(const Vector3r& mn, const Vector3r& mx){throw std::runtime_error("Calling abstract Collider.probeAabb.");}

		/*!
		Tell whether given bodies may interact, for other than spatial reasons.
		
		Concrete collider implementations should call this function if the bodies are in potential interaction geometrically. */
		static bool mayCollide(const shared_ptr<Particle>&, const shared_ptr<Particle>&, const DemField*);
		/*!
		Invalidate all persistent data (if the collider has any), forcing reinitialization at next run.
		The default implementation does nothing, colliders should override it if it is applicable.
		*/
		virtual void invalidatePersistentData(){}
		// ctor with functors for the integrated BoundDispatcher
		virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);

	virtual void getLabeledObjects(std::map<std::string,py::object>& m);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Collider,GlobalEngine,"Abstract class for finding spatial collisions between bodies. \n\n.. admonition:: Special constructor\n\n\tDerived colliders (unless they override ``pyHandleCustomCtorArgs``) can be given list of :ref:`BoundFunctors <BoundFunctor>` which is used to initialize the internal :ref:`boundDispatcher <Collider.boundDispatcher>` instance.",
		((shared_ptr<BoundDispatcher>,boundDispatcher,new BoundDispatcher,AttrTrait<Attr::readonly>(),":ref:`BoundDispatcher` object that is used for creating :ref:`bounds <Body.bound>` on collider's request as necessary."))
		,/*ctor*/
		,/*py*/ .def("probeAabb",&Collider::probeAabb,(py::arg("mn"),py::arg("mx")),"Return list of particles intersected by axis-aligned box with given corners")
	);
};
REGISTER_SERIALIZABLE(Collider);

