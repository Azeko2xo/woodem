// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<yade/core/Engine.hpp>
#include<yade/core/Functor.hpp>
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/Collision.hpp>


/* ***************************************** */

struct CGeomFunctor: public Functor2D<
	/*dispatch types*/ Shape, Shape,
	/*retrun type*/    bool,
	/*argument types*/ TYPELIST_5(const shared_ptr<Shape>&, const shared_ptr<Shape>&, const Vector3r&, const bool&, const shared_ptr<Contact>&)
>{
	YADE_CLASS_BASE_DOC(CGeomFunctor,Functor,"Functor for creating/updating :yref:`Contact::geom` objects.");
};
REGISTER_SERIALIZABLE(CGeomFunctor);

class CPhysFunctor: public Functor2D<
	/*dispatch types*/ Material, Material,
	/*retrun type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&)
>{
	YADE_CLASS_BASE_DOC(CPhysFunctor,Functor,"Functor for creating/updating :yref:`Contact::phys` objects.");
};
REGISTER_SERIALIZABLE(CPhysFunctor);


class LawFunctor: public Functor2D<
	/*dispatch types*/ CGeom,CPhys,
	/*return type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&)
>{
	YADE_CLASS_BASE_DOC(LawFunctor,Functor,"Functor for applying constitutive laws on :yref:`contacts<Contact>`.");
};
REGISTER_SERIALIZABLE(LawFunctor);

/* ***************************************** */

struct CGeomDispatcher: public Dispatcher2D</* functor type*/ CGeomFunctor, /* autosymmetry*/ false>{
	shared_ptr<Contact> explicitAction(Scene*, const shared_ptr<Particle>&, const shared_ptr<Particle>&, bool force);
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CGeomDispatcher,CGeomFunctor,/* doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(CGeomDispatcher);

struct CPhysDispatcher: public Dispatcher2D</*functor type*/ CPhysFunctor>{		
	void explicitAction(Scene*, shared_ptr<Material>&, shared_ptr<Material>&, shared_ptr<Contact>&);
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CPhysDispatcher,CPhysFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
};
REGISTER_SERIALIZABLE(CPhysDispatcher);

struct LawDispatcher: public Dispatcher2D</*functor type*/ LawFunctor, /*autosymmetry*/ false>{
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(LawDispatcher,LawFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(LawDispatcher);



class ContactLoop: public GlobalEngine, private DemField::Engine {
	// store interactions that should be deleted after loop in action, not later
	DemField* dem;
	#ifdef YADE_OPENMP
		vector<list<shared_ptr<Contact> > > removeAfterLoopRefs;
		void removeAfterLoop(const shared_ptr<Contact>& c){ removeAfterLoopRefs[omp_get_thread_num()].push_back(c); }
	#else
		list<shared_ptr<Contact> > removeAfterLoopRefs;
		void removeAfterLoop(const shared_ptr<Contact>& c){ removeAfterLoopRefs.push_back(c); }
	#endif
	public:
		virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);
		virtual void getLabeledObjects(std::map<std::string, py::object>& m);
		virtual void run();
		YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(ContactLoop,GlobalEngine,"Loop over all contacts, possible in a parallel manner.\n\n.. admonition:: Special constructor\n\n\tConstructs from 3 lists of :yref:`Cg2<CGeomFunctor>`, :yref:`Cp2<IPhysFunctor>`, :yref:`Law<LawFunctor>` functors respectively; they will be passed to interal dispatchers.",
			((shared_ptr<CGeomDispatcher>,geoDisp,new CGeomDispatcher,Attr::readonly,":yref:`CGeomDispatcher` object that is used for dispatch."))
			((shared_ptr<CPhysDispatcher>,phyDisp,new CPhysDispatcher,Attr::readonly,":yref:`CPhysDispatcher` object used for dispatch."))
			((shared_ptr<LawDispatcher>,lawDisp,new LawDispatcher,Attr::readonly,":yref:`LawDispatcher` object used for dispatch."))
			((bool,alreadyWarnedNoCollider,false,,"Keep track of whether the user was already warned about missing collider."))
			((bool,trackWork,false,,"In periodic simulations, increment *gradV* energy according to current stress and cell configuration."))
			((Matrix3r,stress,Matrix3r::Zero(),Attr::readonly,"Stress value, used to compute *gradV*  energy if *trackWork* is True."))
			((Real,prevVol,NaN,Attr::hidden,"Previous value of cell volume"))
			//((Real,prevTrGradVStress,NaN,Attr::hidden,"Previous value of tr(gradV*stress)"))
			((Matrix3r,prevStress,Matrix3r::Zero(),,"Previous value of stress, used to compute mid-step stress"))
			((int,gradVIx,-1,(Attr::hidden|Attr::noSave),"Cache energy index for gradV work"))
			,
			/*ctor*/
				#ifdef IDISP_TIMING
					timingDeltas=shared_ptr<TimingDeltas>(new TimingDeltas);
				#endif
				#ifdef YADE_OPENMP
					removeAfterLoopRefs.resize(omp_get_max_threads());
				#endif
			,
			/*py*/
		);
		DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(ContactLoop);


