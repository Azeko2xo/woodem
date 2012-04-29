// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/pkg/dem/IntraForce.hpp>

/*! Object representing infinite plane aligned with the coordinate system (axis-aligned wall). */
struct InfCylinder: public Shape{
	bool numNodesOk() const { return nodes.size()==1; }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(InfCylinder,Shape,"Object representing infinite plane aligned with the coordinate system (axis-aligned wall).",
		((Real,radius,NaN,,"Radius of the cylinder"))
		((int,axis,0,,"Axis of the normal; can be 0,1,2 for +x, +y, +z respectively (Node's orientation is disregarded for walls)"))
		((Vector2r,glAB,Vector2r(NaN,NaN),,"Endpoints between which the infinite cylinder is drawn; if NaN, taken from scene view to be visible"))
		,/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(InfCylinder,Shape);
};	
REGISTER_SERIALIZABLE(InfCylinder);

struct Bo1_InfCylinder_Aabb: public BoundFunctor{
	virtual void go(const shared_ptr<Shape>&);
	FUNCTOR1D(InfCylinder);
	YADE_CLASS_BASE_DOC(Bo1_InfCylinder_Aabb,BoundFunctor,"Creates/updates an :yref:`Aabb` of a :yref:`InfCylinder`");
};
REGISTER_SERIALIZABLE(Bo1_InfCylinder_Aabb);

#ifdef YADE_OPENGL

#include<yade/pkg/gl/Functors.hpp>
struct Gl1_InfCylinder: public GlShapeFunctor{	
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool,const GLViewInfo&);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_InfCylinder,GlShapeFunctor,"Renders :yref:`InfCylinder` object",
		((bool,wire,false,,"Render Cylinders with wireframe"))
		((int,slices,12,,"Number of circumferential division of circular sections"))
		((int,stacks,20,,"Number of rings on the cylinder inside the visible scene part."))
	);
	RENDERS(InfCylinder);
};
REGISTER_SERIALIZABLE(Gl1_InfCylinder);

#endif


