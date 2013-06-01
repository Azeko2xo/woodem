// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<woo/core/Engine.hpp>
#include<woo/core/Functor.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>


/* ***************************************** */

struct CGeomFunctor: public Functor2D<
	/*dispatch types*/ Shape, Shape,
	/*retrun type*/    bool,
	/*argument types*/ TYPELIST_5(const shared_ptr<Shape>&, const shared_ptr<Shape>&, const Vector3r&, const bool&, const shared_ptr<Contact>&)
>{
	WOO_CLASS_BASE_DOC(CGeomFunctor,Functor,"Functor for creating/updating :ref:`Contact::geom` objects.");
};
REGISTER_SERIALIZABLE(CGeomFunctor);

class CPhysFunctor: public Functor2D<
	/*dispatch types*/ Material, Material,
	/*retrun type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&)
>{
	WOO_CLASS_BASE_DOC(CPhysFunctor,Functor,"Functor for creating/updating :ref:`Contact.phys` objects.");
};
REGISTER_SERIALIZABLE(CPhysFunctor);


class LawFunctor: public Functor2D<
	/*dispatch types*/ CGeom,CPhys,
	/*return type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&)
>{
	WOO_CLASS_BASE_DOC(LawFunctor,Functor,"Functor for applying constitutive laws on :ref:`contacts<Contact>`.");
};
REGISTER_SERIALIZABLE(LawFunctor);

/* ***************************************** */

struct CGeomDispatcher: public Dispatcher2D</* functor type*/ CGeomFunctor, /* autosymmetry*/ false>{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	shared_ptr<Contact> explicitAction(Scene*, const shared_ptr<Particle>&, const shared_ptr<Particle>&, bool force);
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CGeomDispatcher,CGeomFunctor,/* doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(CGeomDispatcher);

struct CPhysDispatcher: public Dispatcher2D</*functor type*/ CPhysFunctor>{		
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void explicitAction(Scene*, shared_ptr<Material>&, shared_ptr<Material>&, shared_ptr<Contact>&);
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CPhysDispatcher,CPhysFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
};
REGISTER_SERIALIZABLE(CPhysDispatcher);

struct LawDispatcher: public Dispatcher2D</*functor type*/ LawFunctor, /*autosymmetry*/ false>{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(LawDispatcher,LawFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(LawDispatcher);



class ContactLoop: public GlobalEngine {
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	// store interactions that should be deleted after loop in action, not later
	#ifdef WOO_OPENMP
		vector<list<shared_ptr<Contact> > > removeAfterLoopRefs;
		void removeAfterLoop(const shared_ptr<Contact>& c){ removeAfterLoopRefs[omp_get_thread_num()].push_back(c); }
	#else
		list<shared_ptr<Contact> > removeAfterLoopRefs;
		void removeAfterLoop(const shared_ptr<Contact>& c){ removeAfterLoopRefs.push_back(c); }
	#endif
	public:
		virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);
		virtual void getLabeledObjects(std::map<std::string, py::object>& m, const shared_ptr<LabelMapper>&);
		virtual void run();
		WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ContactLoop,GlobalEngine,"Loop over all contacts, possible in a parallel manner.\n\n.. admonition:: Special constructor\n\n\tConstructs from 3 lists of :ref:`Cg2<CGeomFunctor>`, :ref:`Cp2<IPhysFunctor>`, :ref:`Law<LawFunctor>` functors respectively; they will be passed to interal dispatchers.",
			((shared_ptr<CGeomDispatcher>,geoDisp,new CGeomDispatcher,AttrTrait<Attr::readonly>(),":ref:`CGeomDispatcher` object that is used for dispatch."))
			((shared_ptr<CPhysDispatcher>,phyDisp,new CPhysDispatcher,AttrTrait<Attr::readonly>(),":ref:`CPhysDispatcher` object used for dispatch."))
			((shared_ptr<LawDispatcher>,lawDisp,new LawDispatcher,AttrTrait<Attr::readonly>(),":ref:`LawDispatcher` object used for dispatch."))
			((bool,alreadyWarnedNoCollider,false,,"Keep track of whether the user was already warned about missing collider."))
			((bool,evalStress,false,,"Evaluate stress tensor, in periodic simluations; if energy tracking is enabled, increments *gradV* energy."))
			((bool,applyForces,true,,"Apply forces directly; this avoids IntraForce engine, but will silently skip multinodal particles."))
			((bool,updatePhys,false,,"Call :ref:`CPhysFunctor` even for contacts which already have :ref:`Contact.phys` (to reflect changes in particle's material, for example)"))
			((bool,_forceApplyChecked,false,AttrTrait<>().noGui(),"We already warned if forces are not applied here and no IntraForce engine exists in O.scene.engines"))
			((Matrix3r,stress,Matrix3r::Zero(),AttrTrait<Attr::readonly>(),"Stress value, used to compute *gradV*  energy if *trackWork* is True."))
			((Real,prevVol,NaN,AttrTrait<Attr::hidden>(),"Previous value of cell volume"))
			//((Real,prevTrGradVStress,NaN,AttrTrait<Attr::hidden>(),"Previous value of tr(gradV*stress)"))
			((Matrix3r,prevStress,Matrix3r::Zero(),,"Previous value of stress, used to compute mid-step stress"))
			((int,gradVIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Cache energy index for gradV work"))
			,
			/*ctor*/
				#ifdef IDISP_TIMING
					timingDeltas=shared_ptr<TimingDeltas>(new TimingDeltas);
				#endif
				#ifdef WOO_OPENMP
					removeAfterLoopRefs.resize(omp_get_max_threads());
				#endif
			,
			/*py*/
		);
		DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(ContactLoop);

