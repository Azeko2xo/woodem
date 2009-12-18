// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<yade/core/Shape.hpp>
#include<yade/pkg-common/BoundFunctor.hpp>


/*! Object representing infinite plane aligned with the coordinate system (axis-aligned wall). */
class Wall: public Shape{
	public:
		Wall(){ createIndex(); }
		virtual ~Wall(); // vtable
		//! Which side of the wall interacts: -1 for negative only, 0 for both, +1 for positive only
		int sense;
		//! Axis of the normal; can be 0,1,2 for +x, +y, +z respectively (Body's orientation is disregarded for walls)
		int axis;
	REGISTER_ATTRIBUTES(Shape,(sense)(axis));
	REGISTER_CLASS_AND_BASE(Wall,Shape);
	REGISTER_CLASS_INDEX(Wall,Shape);
};	
REGISTER_SERIALIZABLE(Wall);

/*! Functor for computing axis-aligned bounding box
    from axis-aligned wall. Has no parameters. */
class Bo1_Wall_Aabb: public BoundFunctor{
	public:
		virtual void go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r& se3, const Body*);
	FUNCTOR2D(Wall,Aabb);
	REGISTER_CLASS_AND_BASE(Bo1_Wall_Aabb,BoundFunctor);
};
REGISTER_SERIALIZABLE(Bo1_Wall_Aabb);
#ifdef YADE_OPENGL
	#include<yade/pkg-common/GLDrawFunctors.hpp>
	class Gl1_Wall: public GlShapeFunctor{	
		//! Number of divisions
		static int div;
		public:
			virtual void go(const shared_ptr<Shape>&, const shared_ptr<State>&,bool,const GLViewInfo&);
		RENDERS(Wall);
		REGISTER_ATTRIBUTES(GlShapeFunctor,(div));
		REGISTER_CLASS_AND_BASE(Gl1_Wall,GlShapeFunctor);
	};
	REGISTER_SERIALIZABLE(Gl1_Wall);
#endif

