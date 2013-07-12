#pragma once
#include<woo/pkg/dem/Collision.hpp>

struct GridCollider;
struct GridStore;
struct Shape;

// #define WOO_GRID_BOUND_DEBUG

struct GridBound: public Bound{
	// set *nodePlay* based on current nodal positions and verletDist
	void setNodePlay(const shared_ptr<Shape>& s, const Real& verletDist);
	// check if all nodes are inside their respective nodePlay boxes
	bool insideNodePlay(const shared_ptr<Shape>& s) const;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(GridBound,Bound,"Bound defined via grid cell indices (used with :obj:`GridCollider`)",/*attrs*/
		#ifdef WOO_GRID_BOUND_DEBUG
			((vector<Vector3i>,cells,,AttrTrait<>().noGui(),"Cells touched by this particle"))
		#endif
		((vector<AlignedBox3r>,nodePlay,,AttrTrait<>().readonly(),"Space in which respective nodes of the shapes may be without triggering new contact detection"))
		,
		/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(GridBound,Bound);
};
WOO_REGISTER_OBJECT(GridBound);

#ifdef WOO_OPENGL
struct Gl1_GridBound: public GlBoundFunctor{
	void go(const shared_ptr<Bound>&){}
	RENDERS(GridBound);
	WOO_CLASS_BASE_DOC(Gl1_GridBound,GlBoundFunctor,"Render :obj:`GridBound` objects).");
};
WOO_REGISTER_OBJECT(Gl1_GridBound);
#endif

// TODO: remove the force param if it won't be ever used (i.e. always true)
// does not have to return bool in that case, either
struct GridBoundFunctor: public Functor1D</*dispatch types*/ Shape,/*return type*/ bool, /*argument types*/ TYPELIST_5(const shared_ptr<Shape>&, const Particle::id_t&, const shared_ptr<GridCollider>&, const shared_ptr<GridStore>&, const bool&)>{
	WOO_CLASS_BASE_DOC(GridBoundFunctor,Functor,"Functor for creating/updating :obj:`woo.dem.GridBound`.");
};
WOO_REGISTER_OBJECT(GridBoundFunctor);

struct GridBoundDispatcher: public Dispatcher1D</* functor type*/GridBoundFunctor>{
	void run(){}
	WOO_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(GridBoundDispatcher,GridBoundFunctor,/*optional doc*/,
		/*additional attrs*/
		,/*ctor*/,/*py*/
	);
};
WOO_REGISTER_OBJECT(GridBoundDispatcher);


#include<woo/pkg/dem/Sphere.hpp>
struct Grid1_Sphere: public GridBoundFunctor{
	bool go(const shared_ptr<Shape>&, const Particle::id_t&, const shared_ptr<GridCollider>&, const shared_ptr<GridStore>&, const bool& force);
	FUNCTOR1D(Sphere);
	WOO_CLASS_BASE_DOC_ATTRS(Grid1_Sphere,GridBoundFunctor,"Functor filling :obj:`GridStore` from :obj:`Sphere`, used with :obj:`GridCollider`.",
		((Real,distFactor,((void)"deactivated",-1),,"Relative enlargement of the bounding box; deactivated if negative."))
	);
};
WOO_REGISTER_OBJECT(Grid1_Sphere);

#include<woo/pkg/dem/Wall.hpp>
struct Grid1_Wall: public GridBoundFunctor{
	bool go(const shared_ptr<Shape>&, const Particle::id_t&, const shared_ptr<GridCollider>&, const shared_ptr<GridStore>&, const bool& force);
	FUNCTOR1D(Wall);
	WOO_CLASS_BASE_DOC_ATTRS(Grid1_Wall,GridBoundFunctor,"Functor filling :obj:`GridStore` from :obj:`Wall`, used with :obj:`GridCollider`.",
		((bool,movable,false,,"Set to allow movable walls (with grid enlarged by :obj:`GridCollider.verletDist`. If false and a movable wall is encountered, an exception is raised."))
	);
};
WOO_REGISTER_OBJECT(Grid1_Wall);


