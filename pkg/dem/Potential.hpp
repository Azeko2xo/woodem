#pragma once

#if 0
#include<woo/pkg/dem/Particle.hpp>
#include<woo/core/Dispatcher.hpp>


struct PotentialFunctor: public Functor1D</*dispatch types*/ Shape,/*return type*/ Real, /*argument types*/ TYPELIST_2(const shared_ptr<Shape>&, const Vector3r&)>{
	// function returning potential at the coordinate given
	typedef std::function<Real(const Vector3r&)> PotFuncType;
	typedef std::function<Vector3r(const Vector3r&)> GradFuncType;
	virtual PotFuncType funcPotentialValue(const shared_ptr<Shape>&);
	virtual GradFuncType funcPotentialGradient(const shared_ptr<Shape>&);
	Real go(const shared_ptr<Shape>&, const Vector3r& pos) WOO_CXX11_OVERRIDE;
	// return bounding sphere radius, for uninodal particles only
	virtual Real boundingSphereRadius(const shared_ptr<Shape>&){ return NaN; }
	#define woo_dem_PotentialFunctor__CLASS_BASE_DOC_PY PotentialFunctor,Functor,"Functor for creating/updating :obj:`woo.dem.Potential`.", /*py*/ ; woo::converters_cxxVector_pyList_2way<shared_ptr<PotentialFunctor>>();
	WOO_DECL__CLASS_BASE_DOC_PY(woo_dem_PotentialFunctor__CLASS_BASE_DOC_PY);
};
WOO_REGISTER_OBJECT(PotentialFunctor);

struct PotentialDispatcher: public Dispatcher1D</* functor type*/PotentialFunctor>{
	void run(){}
	WOO_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(PotentialDispatcher,PotentialFunctor,/*optional doc*/,/*additional attrs*/,/*ctor*/,/*py*/);
};
WOO_REGISTER_OBJECT(PotentialDispatcher);

#include<woo/pkg/dem/Sphere.hpp>
struct Pot1_Sphere: public PotentialFunctor{
	Real boundingSphereRadius(const shared_ptr<Shape>&) WOO_CXX11_OVERRIDE;
	Real pot(const shared_ptr<Shape>& s, const Vector3r& x);
	PotFuncType funcPotentialValue(const shared_ptr<Shape>&) WOO_CXX11_OVERRIDE;
	FUNCTOR1D(Sphere);
	#define woo_dem_Pot1_Sphere__CLASS_BASE_DOC Pot1_Sphere,PotentialFunctor,"Fuctor computing potential for spheres."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Pot1_Sphere__CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(Pot1_Sphere);

#include<woo/pkg/dem/Wall.hpp>
struct Pot1_Wall: public PotentialFunctor{
	Real pot(const shared_ptr<Shape>& s, const Vector3r& x);
	PotFuncType funcPotentialValue(const shared_ptr<Shape>&) WOO_CXX11_OVERRIDE;
	FUNCTOR1D(Wall);
	#define woo_dem_Pot1_Wall__CLASS_BASE_DOC Pot1_Wall,PotentialFunctor,"Fuctor computing potential for spheres."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Pot1_Wall__CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(Pot1_Wall);

struct Cg2_Shape_Shape_L6Geom__Potential: public Cg2_Any_Any_L6Geom__Base{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	void setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);
	#define woo_dem_Cg2_Shape_Shape_L6Geom__Potential__CLASS_BASE_DOC_ATTRS \
		Cg2_Shape_Shape_L6Geom__Potential,Cg2_Any_Any_L6Geom__Base,"Compute contact configuration from potential functions for respective colliding particles.", \
		((shared_ptr<PotentialDispatcher>,potentialDispatcher,make_shared<PotentialDispatcher>(),,"Dispatcher for potential functions."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Shape_Shape_L6Geom__Potential__CLASS_BASE_DOC_ATTRS);
	DEFINE_FUNCTOR_ORDER_2D(Shape,Shape);
	FUNCTOR2D(Shape,Shape);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Shape_Shape_L6Geom__Potential);
#endif
