#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/core/Functor.hpp>
#include<yade/pkg/dem/Sphere.hpp>
#include<yade/pkg/dem/FrictMat.hpp>

class IntraFunctor: public Functor2D<
	/*dispatch types*/ Shape,Material,
	/*retrun type*/    void,
	/*argument types*/ TYPELIST_3(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&)
>{
	YADE_CLASS_BASE_DOC(IntraFunctor,Functor,"Functor appying internal forces");
};
REGISTER_SERIALIZABLE(IntraFunctor);

struct IntraDispatcher: public Dispatcher2D</* functor type*/ IntraFunctor, /* autosymmetry*/ false>{
	void action();
	YADE_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(IntraDispatcher,IntraFunctor,/* doc is optional*/,/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(IntraDispatcher);

struct In2_Sphere_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&);
	FUNCTOR2D(Sphere,ElastMat);
	YADE_CLASS_BASE_DOC_ATTRS(In2_Sphere_ElastMat,IntraFunctor,"Apply contact forces on sphere; having one node only, Sphere generates no internal forces as such.",/*attrs*/);
};
REGISTER_SERIALIZABLE(In2_Sphere_ElastMat);
