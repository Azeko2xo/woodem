// 2009 © Václav Šmilauer <eu@doxos.eu>
#pragma once
#include<woo/core/Engine.hpp>
#include<woo/core/Functor.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/Contact.hpp>


/* ***************************************** */

struct CGeomFunctor: public Functor2D<
	/*dispatch types*/ Shape, Shape,
	/*retrun type*/    bool,
	/*argument types*/ TYPELIST_5(const shared_ptr<Shape>&, const shared_ptr<Shape>&, const Vector3r&, const bool&, const shared_ptr<Contact>&)
>{
	// sets minimum nodes[0] -- nodes[0] distance (only used for Sphere+Sphere)
	virtual void setMinDist00Sq(const shared_ptr<Shape>&, const shared_ptr<Shape>&, const shared_ptr<Contact>& C){ C->minDist00Sq=-1; }
	#define woo_dem_CGeomFunctor__CLASS_BASE_DOC_PY CGeomFunctor,Functor,"Functor for creating/updating :obj:`Contact.geom` objects.", /*py*/ ; woo::converters_cxxVector_pyList_2way<shared_ptr<CGeomFunctor>>();
	WOO_DECL__CLASS_BASE_DOC_PY(woo_dem_CGeomFunctor__CLASS_BASE_DOC_PY);
};
WOO_REGISTER_OBJECT(CGeomFunctor);

class CPhysFunctor: public Functor2D<
	/*dispatch types*/ Material, Material,
	/*retrun type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&)
>{
	#define woo_dem_CPhysFunctor__CLASS_BASE_DOC_PY CPhysFunctor,Functor,"Functor for creating/updating :obj:`Contact.phys` objects.", /*py*/ ; woo::converters_cxxVector_pyList_2way<shared_ptr<CPhysFunctor>>();
	WOO_DECL__CLASS_BASE_DOC_PY(woo_dem_CPhysFunctor__CLASS_BASE_DOC_PY);
};
WOO_REGISTER_OBJECT(CPhysFunctor);


class LawFunctor: public Functor2D<
	/*dispatch types*/ CGeom,CPhys,
	/*return type*/    bool,
	/*argument types*/ TYPELIST_3(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&)
>{
	#define woo_dem_LawFunctor__CLASS_BASE_DOC_PY LawFunctor,Functor,"Functor for applying constitutive laws on :obj:`contact <Contact>`.", /*py*/ ; woo::converters_cxxVector_pyList_2way<shared_ptr<LawFunctor>>();
	WOO_DECL__CLASS_BASE_DOC_PY(woo_dem_LawFunctor__CLASS_BASE_DOC_PY);
};
WOO_REGISTER_OBJECT(LawFunctor);

/* ***************************************** */

struct CGeomDispatcher: public Dispatcher2D</* functor type*/ CGeomFunctor, /* autosymmetry*/ false>{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	shared_ptr<Contact> explicitAction(Scene*, const shared_ptr<Particle>&, const shared_ptr<Particle>&, bool force);
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CGeomDispatcher,CGeomFunctor,/* doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(CGeomDispatcher);

struct CPhysDispatcher: public Dispatcher2D</*functor type*/ CPhysFunctor>{		
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void explicitAction(Scene*, shared_ptr<Material>&, shared_ptr<Material>&, shared_ptr<Contact>&);
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CPhysDispatcher,CPhysFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
};
WOO_REGISTER_OBJECT(CPhysDispatcher);

struct LawDispatcher: public Dispatcher2D</*functor type*/ LawFunctor, /*autosymmetry*/ false>{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(LawDispatcher,LawFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(LawDispatcher);


// #define CONTACTLOOP_TIMING

#ifdef CONTACTLOOP_TIMING
	#define CONTACTLOOP_CHECKPOINT(what) timingDeltas->checkpoint(__LINE__,what);
#else
	#define CONTACTLOOP_CHECKPOINT(what)
#endif

class ContactLoop: public Engine {
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	// store interactions that should be deleted after loop in action, not later
	#ifdef WOO_OPENMP
		vector<list<shared_ptr<Contact>>> removeAfterLoopRefs;
		void removeAfterLoop(const shared_ptr<Contact>& c){ removeAfterLoopRefs[omp_get_thread_num()].push_back(c); }
	#else
		list<shared_ptr<Contact>> removeAfterLoopRefs;
		void removeAfterLoop(const shared_ptr<Contact>& c){ removeAfterLoopRefs.push_back(c); }
	#endif
	void reorderContacts();
	public:
		virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);
		virtual void getLabeledObjects(const shared_ptr<LabelMapper>&);
		virtual void run();
	#ifdef CONTACTLOOP_TIMING
		#define woo_dem_ContactLoop__CTOR_timingDeltas timingDeltas=shared_ptr<TimingDeltas>(new TimingDeltas);
	#else
		#define woo_dem_ContactLoop__CTOR_timingDeltas
	#endif
	#ifdef WOO_OPENMP
		#define woo_dem_ContactLoop__CTOR_removeAfterLoopRefs removeAfterLoopRefs.resize(omp_get_max_threads());
	#else
		#define woo_dem_ContactLoop__CTOR_removeAfterLoopRefs
	#endif

	#define woo_dem_ContactLoop__CLASS_BASE_DOC_ATTRS_CTOR \
		ContactLoop,Engine,"Loop over all contacts, possible in a parallel manner.\n\n.. admonition:: Special constructor\n\n\tConstructs from 3 lists of :obj:`Cg2 <CGeomFunctor>`, :obj:`Cp2 <IPhysFunctor>`, :obj:`Law <LawFunctor>` functors respectively; they will be passed to interal dispatchers.", \
			((shared_ptr<CGeomDispatcher>,geoDisp,new CGeomDispatcher,AttrTrait<Attr::readonly>(),":obj:`CGeomDispatcher` object that is used for dispatch.")) \
			((shared_ptr<CPhysDispatcher>,phyDisp,new CPhysDispatcher,AttrTrait<Attr::readonly>(),":obj:`CPhysDispatcher` object used for dispatch.")) \
			((shared_ptr<LawDispatcher>,lawDisp,new LawDispatcher,AttrTrait<Attr::readonly>(),":obj:`LawDispatcher` object used for dispatch.")) \
			((bool,alreadyWarnedNoCollider,false,AttrTrait<>().noGui(),"Keep track of whether the user was already warned about missing collider.")) \
			((bool,evalStress,false,,"Evaluate stress tensor, in periodic simluations; if energy tracking is enabled, increments *gradV* energy.")) \
			((bool,applyForces,true,,"Apply forces directly; this avoids IntraForce engine, but will silently skip multinodal particles.")) \
			((bool,updatePhys,false,,"Call :obj:`CPhysFunctor` even for contacts which already have :obj:`Contact.phys` (to reflect changes in particle's material, for example)")) \
			/*((bool,alreadyWarnedForceNotApplied,false,AttrTrait<>().noGui(),"We already warned if forces are not applied here and no IntraForce engine exists in O.scene.engines")) */ \
			((bool,dist00,true,,"Whether to apply the Contact.minDist00Sq optimization (for mesuring the speedup only)")) \
			((Matrix3r,stress,Matrix3r::Zero(),AttrTrait<Attr::readonly>(),"Stress value, used to compute *gradV*  energy if *trackWork* is True.")) \
			((int,reorderEvery,1000,,"Reorder contacts so that real ones are at the beginning in the linear sequence, making the OpenMP loop traversal (hopefully) less unbalanced.")) \
			((Real,prevVol,NaN,AttrTrait<Attr::hidden>(),"Previous value of cell volume")) \
			/*((Real,prevTrGradVStress,NaN,AttrTrait<Attr::hidden>(),"Previous value of tr(gradV*stress)"))*/ \
			((Matrix3r,prevStress,Matrix3r::Zero(),,"Previous value of stress, used to compute mid-step stress")) \
			((int,gradVIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Cache energy index for gradV work")) \
			, /*ctor*/ \
				woo_dem_ContactLoop__CTOR_timingDeltas \
				woo_dem_ContactLoop__CTOR_removeAfterLoopRefs

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_ContactLoop__CLASS_BASE_DOC_ATTRS_CTOR);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(ContactLoop);

