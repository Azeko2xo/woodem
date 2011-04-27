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
	/*argument types*/ TYPELIST_3(shared_ptr<CGeom>&, shared_ptr<CPhys>&, const shared_ptr<Contact>&)
>{
	YADE_CLASS_BASE_DOC(LawFunctor,Functor,"Functor for applying constitutive laws on :yref:`interactions<Interaction>`.");
};
REGISTER_SERIALIZABLE(LawFunctor);

/* ***************************************** */

struct CGeomDispatcher: public Dispatcher2D</* functor type*/ CGeomFunctor, /* autosymmetry*/ false>{
	//	shared_ptr<Interaction> explicitAction(const shared_ptr<Body>& b1, const shared_ptr<Body>& b2, bool force);
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CGeomDispatcher,CGeomFunctor,/* doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(CGeomDispatcher);

struct CPhysDispatcher: public Dispatcher2D</*functor type*/ CPhysFunctor>{		
	//	void explicitAction(shared_ptr<Material>& pp1, shared_ptr<Material>& pp2, shared_ptr<Interaction>& i);
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(CPhysDispatcher,CPhysFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
};
REGISTER_SERIALIZABLE(CPhysDispatcher);

struct LawDispatcher: public Dispatcher2D</*functor type*/ LawFunctor, /*autosymmetry*/ false>{
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(LawDispatcher,LawFunctor,/*doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(LawDispatcher);



class ContactLoop: public GlobalEngine {
	// store interactions that should be deleted after loop in action, not later
	shared_ptr<DemField> field;
#if 0
	#ifdef YADE_OPENMP
		vector<list<idPair> > eraseAfterLoopObj;
		void eraseAfterLoop(const shared_ptr<Contact>& c){ eraseAfterLoopIds[omp_get_thread_num()].push_back(idPair(id1,id2)); }
	#else
		list<shared_ptr<Contact> > eraseAfterLoopObj;
		void eraseAfterLoop(const shared_ptr<Contact>& c){ eraseAfterLoopObj.push_back(c); }
	#endif
#endif
	public:
		virtual void pyHandleCustomCtorArgs(python::tuple& t, python::dict& d);
		virtual void action();
		YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(ContactLoop,GlobalEngine,"Loop over all contacts, possible in a parallel manner.\n\n.. admonition:: Special constructor\n\n\tConstructs from 3 lists of :yref:`Cg2<CGeomFunctor>`, :yref:`Cp2<IPhysFunctor>`, :yref:`Law<LawFunctor>` functors respectively; they will be passed to interal dispatchers.",
			((shared_ptr<CGeomDispatcher>,geoDisp,new CGeomDispatcher,Attr::readonly,":yref:`CGeomDispatcher` object that is used for dispatch."))
			((shared_ptr<CPhysDispatcher>,phyDisp,new CPhysDispatcher,Attr::readonly,":yref:`CPhysDispatcher` object used for dispatch."))
			((shared_ptr<LawDispatcher>,lawDisp,new LawDispatcher,Attr::readonly,":yref:`LawDispatcher` object used for dispatch."))
			,
			/*ctor*/
				#ifdef IDISP_TIMING
					timingDeltas=shared_ptr<TimingDeltas>(new TimingDeltas);
				#endif
				#ifdef YADE_OPENMP
					eraseAfterLoopObj.resize(omp_get_max_threads());
				#endif
			,
			/*py*/
		);
		DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(ContactLoop);


