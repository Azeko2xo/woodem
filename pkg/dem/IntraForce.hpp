#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/core/Functor.hpp>
#include<woo/core/Dispatcher.hpp>

struct IntraFunctor: public Functor2D<
	/*dispatch types*/ Shape,Material,
	/*retrun type*/    void,
	/*argument types*/ TYPELIST_4(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&, const bool)
>{
	// called from IntraForce::critDt
	virtual void addIntraStiffnesses(const shared_ptr<Particle>&, const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const;
	
	#define woo_dem_IntraFunctor__CLASS_BASE_DOC_PY IntraFunctor,Functor,"Functor appying internal forces", /*py*/ ; woo::converters_cxxVector_pyList_2way<shared_ptr<IntraFunctor>>();
	WOO_DECL__CLASS_BASE_DOC_PY(woo_dem_IntraFunctor__CLASS_BASE_DOC_PY);
};
WOO_REGISTER_OBJECT(IntraFunctor);

struct IntraForce: public Dispatcher2D</* functor type*/ IntraFunctor, /* autosymmetry*/ false>{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void addIntraStiffness(const shared_ptr<Particle>&, const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot);
	void run();
	WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(IntraForce,IntraFunctor,/*ClassObject instantiated by the macro*/.doc("Apply internal forces on integration nodes, by calling appropriate :obj:`IntraFunctor` objects.").section("Internal forces","TODO",{"IntraFunctor"}),/*attrs*/,/*ctor*/,/*py*/);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(IntraForce);

