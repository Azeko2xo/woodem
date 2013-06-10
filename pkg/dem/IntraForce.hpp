#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/core/Functor.hpp>
#include<woo/core/Dispatcher.hpp>

class IntraFunctor: public Functor2D<
	/*dispatch types*/ Shape,Material,
	/*retrun type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&)
>{
	WOO_CLASS_BASE_DOC(IntraFunctor,Functor,"Functor appying internal forces");
};
WOO_REGISTER_OBJECT(IntraFunctor);

struct IntraForce: public Dispatcher2D</* functor type*/ IntraFunctor, /* autosymmetry*/ false>{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void run();
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(IntraForce,IntraFunctor,/* doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(IntraForce);

