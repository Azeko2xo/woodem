// © 2014 Václav Šmilauer <eu@doxos.eu>
#pragma once

#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/Facet.hpp>


struct Capsule: public Shape{
	void selfTest(const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
	int numNodes() const WOO_CXX11_OVERRIDE { return 1; }
	void setFromRaw(const Vector3r& _center, const Real& _radius, const vector<Real>& raw) WOO_CXX11_OVERRIDE;
	void asRaw(Vector3r& _center, Real& _radius, vector<Real>& raw) const WOO_CXX11_OVERRIDE;
	// recompute inertia and mass
	void updateMassInertia(const Real& density) const WOO_CXX11_OVERRIDE;
	Real equivRadius() const WOO_CXX11_OVERRIDE;
	Real volume() const WOO_CXX11_OVERRIDE;
	bool isInside(const Vector3r& pt) const WOO_CXX11_OVERRIDE;
	// compute axis-aligned bounding box
	AlignedBox3r alignedBox() const WOO_CXX11_OVERRIDE;
	void applyScale(Real scale) WOO_CXX11_OVERRIDE;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(Capsule,Shape,"Cylinder with half-spherical caps on both sides, Mindowski sum of segment with sphere.",
		((Real,radius,NaN,AttrTrait<>().lenUnit(),"Radius of the capsule -- of half-spherical caps and also of the middle part."))
		((Real,shaft,NaN,AttrTrait<>().lenUnit(),"Length of the middle segment"))
		,/*ctor*/createIndex();
	);
	REGISTER_CLASS_INDEX(Capsule,Shape);
};
WOO_REGISTER_OBJECT(Capsule);

struct Bo1_Capsule_Aabb: public BoundFunctor{
	virtual void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Capsule);
	WOO_CLASS_BASE_DOC(Bo1_Capsule_Aabb,BoundFunctor,"Creates/updates an :obj:`Aabb` of a :obj:`Capsule`");
};
WOO_REGISTER_OBJECT(Bo1_Capsule_Aabb);


struct Cg2_Capsule_Capsule_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	void setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Capsule_Capsule_L6Geom__CLASS_BASE_DOC \
		Cg2_Capsule_Capsule_L6Geom,Cg2_Any_Any_L6Geom__Base,"Collision of two :obj:`Capsule` shapes."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Capsule_Capsule_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Capsule,Capsule);
	DEFINE_FUNCTOR_ORDER_2D(Capsule,Capsule);
	//DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Capsule_Capsule_L6Geom);

struct Cg2_Wall_Capsule_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Wall_Capsule_L6Geom__CLASS_BASE_DOC \
		Cg2_Wall_Capsule_L6Geom,Cg2_Any_Any_L6Geom__Base,"Compute :obj:`L6Geom` for contact of :obj:`ellipsoid <woo.dem.Capsule>` and :obj:`wall <woo.dem.Wall>` (axis-aligned plane)."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Capsule_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Wall,Capsule);
	DEFINE_FUNCTOR_ORDER_2D(Wall,Capsule);
};
WOO_REGISTER_OBJECT(Cg2_Wall_Capsule_L6Geom);

struct Cg2_Facet_Capsule_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Facet_Capsule_L6Geom__CLASS_BASE_DOC \
		Cg2_Facet_Capsule_L6Geom,Cg2_Any_Any_L6Geom__Base,"Compute :obj:`L6Geom` for contact of :obj:`ellipsoid <woo.dem.Capsule>` and :obj:`facet <woo.dem.Facet>` (axis-aligned plane)."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Capsule_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Facet,Capsule);
	DEFINE_FUNCTOR_ORDER_2D(Facet,Capsule);
};
WOO_REGISTER_OBJECT(Cg2_Facet_Capsule_L6Geom);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Capsule: public Gl1_Sphere{
	virtual void go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2,const GLViewInfo& glInfo);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Capsule,Gl1_Sphere,"Renders :obj:`woo.dem.Capsule` object",);
	RENDERS(Capsule);
};
WOO_REGISTER_OBJECT(Gl1_Capsule);
#endif

