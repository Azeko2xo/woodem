// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/Sphere.hpp>

/*! Object representing infinite plane aligned with the coordinate system (axis-aligned wall). */
struct InfCylinder: public Shape{
	int numNodes() const WOO_CXX11_OVERRIDE { return 1; }
	void updateMassInertia(const Real& density) const WOO_CXX11_OVERRIDE;
	#define woo_dem_InfCylinder__CLASS_BASE_DOC_ATTRS_CTOR \
		InfCylinder,Shape,"Object representing infinite plane aligned with the coordinate system (axis-aligned wall).", \
		((Real,radius,NaN,,"Radius of the cylinder")) \
		((int,axis,0,,"Axis of the normal; can be 0,1,2 for +x, +y, +z respectively (Node's orientation is disregarded for walls)")) \
		((Vector2r,glAB,Vector2r(NaN,NaN),,"Endpoints between which the infinite cylinder is drawn; if NaN, taken from scene view to be visible")) \
		,/*ctor*/createIndex();
	
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_InfCylinder__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(InfCylinder,Shape);
};	
WOO_REGISTER_OBJECT(InfCylinder);

struct Bo1_InfCylinder_Aabb: public BoundFunctor{
	virtual void go(const shared_ptr<Shape>&);
	FUNCTOR1D(InfCylinder);
	WOO_CLASS_BASE_DOC(Bo1_InfCylinder_Aabb,BoundFunctor,"Creates/updates an :obj:`Aabb` of a :obj:`InfCylinder`");
};
WOO_REGISTER_OBJECT(Bo1_InfCylinder_Aabb);

struct Cg2_InfCylinder_Sphere_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be InfCylinder+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	#define woo_dem_Cg2_InfCylinder_Sphere_L6Geom__CLASS_BASE_DOC \
		Cg2_InfCylinder_Sphere_L6Geom,Cg2_Any_Any_L6Geom__Base,"Incrementally compute :obj:`L6Geom` for contact between :obj:`InfCylinder` and :obj:`Sphere`. Uses attributes of :obj:`Cg2_Sphere_Sphere_L6Geom`."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_InfCylinder_Sphere_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(InfCylinder,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(InfCylinder,Sphere);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_InfCylinder_Sphere_L6Geom);



#ifdef WOO_OPENGL

#include<woo/pkg/gl/Functors.hpp>
struct Gl1_InfCylinder: public GlShapeFunctor{	
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool,const GLViewInfo&);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_InfCylinder,GlShapeFunctor,"Renders :obj:`InfCylinder` object",
		((bool,wire,false,,"Render Cylinders with wireframe"))
		((bool,spokes,true,,"Render spokes between the cylinder axis and edge, at the position of :obj:`InfCylinder.glAB`."))
		((int,slices,12,,"Number of circumferential division of circular sections"))
		((int,stacks,20,,"Number of rings on the cylinder inside the visible scene part."))
	);
	RENDERS(InfCylinder);
};
WOO_REGISTER_OBJECT(Gl1_InfCylinder);

#endif


