// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/pkg/dem/IntraForce.hpp>



/*! Object representing infinite plane aligned with the coordinate system (axis-aligned wall). */
struct Wall: public Shape{
	bool numNodesOk() const { return nodes.size()==1; }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Wall,Shape,"Object representing infinite plane aligned with the coordinate system (axis-aligned wall).",
		((int,sense,0,,"Which side of the wall interacts: -1 for negative only, 0 for both, +1 for positive only"))
		((int,axis,0,,"Axis of the normal; can be 0,1,2 for +x, +y, +z respectively (Node's orientation is disregarded for walls)")),
		/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(Wall,Shape);
};	
REGISTER_SERIALIZABLE(Wall);

struct Bo1_Wall_Aabb: public BoundFunctor{
	virtual void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Wall);
	YADE_CLASS_BASE_DOC(Bo1_Wall_Aabb,BoundFunctor,"Creates/updates an :yref:`Aabb` of a :yref:`Wall`");
};
REGISTER_SERIALIZABLE(Bo1_Wall_Aabb);

struct In2_Wall_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&);
	FUNCTOR2D(Wall,ElastMat);
	YADE_CLASS_BASE_DOC_ATTRS(In2_Wall_ElastMat,IntraFunctor,"Apply contact forces on wall. Wall generates no internal forces as such. Torque from applied forces is discarded, as Wall does not rotate.",
		/*attrs*/
	);
};
REGISTER_SERIALIZABLE(In2_Wall_ElastMat);


#ifdef YADE_OPENGL

#include<yade/pkg/gl/Functors.hpp>
struct Gl1_Wall: public GlShapeFunctor{	
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool,const GLViewInfo&);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_Wall,GlShapeFunctor,"Renders :yref:`Wall` object",
		((int,div,20,,"Number of divisions of the wall inside visible scene part."))
	);
	RENDERS(Wall);
};
REGISTER_SERIALIZABLE(Gl1_Wall);

#endif

