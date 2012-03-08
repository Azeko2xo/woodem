#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/core/Functor.hpp>
#include<yade/core/Dispatcher.hpp>

class IntraFunctor: public Functor2D<
	/*dispatch types*/ Shape,Material,
	/*retrun type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&)
>{
	YADE_CLASS_BASE_DOC(IntraFunctor,Functor,"Functor appying internal forces");
};
REGISTER_SERIALIZABLE(IntraFunctor);

struct IntraForce: public Dispatcher2D</* functor type*/ IntraFunctor, /* autosymmetry*/ false>{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void run();
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(IntraForce,IntraFunctor,/* doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(IntraForce);

