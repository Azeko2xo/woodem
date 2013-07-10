// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/IntraForce.hpp>

/*! Object representing infinite plane aligned with the coordinate system (axis-aligned wall). */
struct InfCylinder: public Shape{
	bool numNodesOk() const { return nodes.size()==1; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(InfCylinder,Shape,"Object representing infinite plane aligned with the coordinate system (axis-aligned wall).",
		((Real,radius,NaN,,"Radius of the cylinder"))
		((int,axis,0,,"Axis of the normal; can be 0,1,2 for +x, +y, +z respectively (Node's orientation is disregarded for walls)"))
		((Vector2r,glAB,Vector2r(NaN,NaN),,"Endpoints between which the infinite cylinder is drawn; if NaN, taken from scene view to be visible"))
		,/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(InfCylinder,Shape);
};	
WOO_REGISTER_OBJECT(InfCylinder);

struct Bo1_InfCylinder_Aabb: public BoundFunctor{
	virtual void go(const shared_ptr<Shape>&);
	FUNCTOR1D(InfCylinder);
	WOO_CLASS_BASE_DOC(Bo1_InfCylinder_Aabb,BoundFunctor,"Creates/updates an :ref:`Aabb` of a :ref:`InfCylinder`");
};
WOO_REGISTER_OBJECT(Bo1_InfCylinder_Aabb);

#ifdef WOO_OPENGL

#include<woo/pkg/gl/Functors.hpp>
struct Gl1_InfCylinder: public GlShapeFunctor{	
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool,const GLViewInfo&);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_InfCylinder,GlShapeFunctor,"Renders :ref:`InfCylinder` object",
		((bool,wire,false,,"Render Cylinders with wireframe"))
		((bool,spokes,true,,"Render spokes between the cylinder axis and edge, at the position of :obj:`InfCylinder.glAB`."))
		((int,slices,12,,"Number of circumferential division of circular sections"))
		((int,stacks,20,,"Number of rings on the cylinder inside the visible scene part."))
	);
	RENDERS(InfCylinder);
};
WOO_REGISTER_OBJECT(Gl1_InfCylinder);

#endif


