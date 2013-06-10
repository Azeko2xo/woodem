// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/IntraForce.hpp>



/*! Object representing infinite plane aligned with the coordinate system (axis-aligned wall). */
struct Wall: public Shape{
	bool numNodesOk() const { return nodes.size()==1; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(Wall,Shape,"Object representing infinite plane aligned with the coordinate system (axis-aligned wall).",
		((int,sense,0,,"Which side of the wall interacts: -1 for negative only, 0 for both, +1 for positive only"))
		((int,axis,0,,"Axis of the normal; can be 0,1,2 for +x, +y, +z respectively (Node's orientation is disregarded for walls)"))
		((AlignedBox2r,glAB,AlignedBox2r(Vector2r(NaN,NaN),Vector2r(NaN,NaN)),,"Points between which the wall is drawn (if NaN, computed automatically to cover the visible part of the scene)"))
		,/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(Wall,Shape);
};	
WOO_REGISTER_OBJECT(Wall);

struct Bo1_Wall_Aabb: public BoundFunctor{
	virtual void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Wall);
	WOO_CLASS_BASE_DOC(Bo1_Wall_Aabb,BoundFunctor,"Creates/updates an :ref:`Aabb` of a :ref:`Wall`");
};
WOO_REGISTER_OBJECT(Bo1_Wall_Aabb);

struct In2_Wall_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&);
	FUNCTOR2D(Wall,ElastMat);
	WOO_CLASS_BASE_DOC_ATTRS(In2_Wall_ElastMat,IntraFunctor,"Apply contact forces on wall. Wall generates no internal forces as such. Torque from applied forces is discarded, as Wall does not rotate.",
		/*attrs*/
	);
};
WOO_REGISTER_OBJECT(In2_Wall_ElastMat);


#ifdef WOO_OPENGL

#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Wall: public GlShapeFunctor{	
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool,const GLViewInfo&);
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Wall,GlShapeFunctor,"Renders :ref:`Wall` object",
		((int,div,20,,"Number of divisions of the wall inside visible scene part."))
	);
	RENDERS(Wall);
};
WOO_REGISTER_OBJECT(Gl1_Wall);

#endif

