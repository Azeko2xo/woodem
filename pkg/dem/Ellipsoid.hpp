#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/L6Geom.hpp>

#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/Facet.hpp>

namespace woo{
	struct Ellipsoid: public Shape{
		void selfTest(const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
		int numNodes() const { return 1; }
		// return transformation matrix transforming unit sphere to this ellipsoid
		Matrix3r trsfFromUnitSphere() const;
		Matrix3r trsfFromUnitSphere(const Quaternionr& ori) const; // with additional rotation (local coords)
		// return extent along one global axis
		Real axisExtent(short axis) const;
		Real rotatedExtent(short axis, const Quaternionr& ori) const; //with additional rotation
		// update dynamic properties (mass, intertia) of the sphere based on current radius
		void updateMassInertia(const Real& density) const WOO_CXX11_OVERRIDE;
		Real equivRadius() const WOO_CXX11_OVERRIDE;
		Real volume() const WOO_CXX11_OVERRIDE;
		bool isInside(const Vector3r& pt) const WOO_CXX11_OVERRIDE;
		// compute axis-aligned bounding box
		AlignedBox3r alignedBox() const WOO_CXX11_OVERRIDE;
		void applyScale(Real scale) WOO_CXX11_OVERRIDE;
		//
		void setFromRaw(const Vector3r& _center, const Real& _radius, const vector<Real>& raw) WOO_CXX11_OVERRIDE;
		void asRaw(Vector3r& _center, Real& _radius, vector<Real>& raw) const WOO_CXX11_OVERRIDE;

		WOO_CLASS_BASE_DOC_ATTRS_CTOR(Ellipsoid,Shape,"Ellipsoidal particle.",
			((Vector3r,semiAxes,Vector3r(NaN,NaN,NaN),AttrTrait<>().lenUnit(),"Semi-principal axes.")),
			createIndex(); /*ctor*/
		);
		REGISTER_CLASS_INDEX(Ellipsoid,Shape);
	};
};
WOO_REGISTER_OBJECT(Ellipsoid);

struct Bo1_Ellipsoid_Aabb: public Bo1_Sphere_Aabb{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Ellipsoid);
	WOO_CLASS_BASE_DOC_ATTRS(Bo1_Ellipsoid_Aabb,Bo1_Sphere_Aabb,"Functor creating :obj:`Aabb` from :obj:`Ellipsoid`.\n\n.. todo:: Handle rotation which is not detected by verlet distance!\n\n.. warning:: :obj:`woo.dem.Bo1_Sphere_Aabb.distFactor` is ignored.",
	);
};
WOO_REGISTER_OBJECT(Bo1_Ellipsoid_Aabb);

struct Cg2_Ellipsoid_Ellipsoid_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	// this can be called for sphere+ellipsoid and ellipsoid+ellipsoid collisions
	bool go_Ellipsoid_or_Sphere(const shared_ptr<Shape>& s1, const Vector3r& semiAxesA, const shared_ptr<Shape>& s2, const Vector3r& semiAxesB, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	void setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Ellipsoid_Ellipsoid_L6Geom__CLASS_BASE_DOC_ATTRS \
		Cg2_Ellipsoid_Ellipsoid_L6Geom,Cg2_Any_Any_L6Geom__Base,"Incrementally compute :obj:`L6Geom` for contact of 2 :obj:`ellipsoids <woo.dem.Ellipsoid>`. Uses the Perram-Wertheim potential function (:cite:`Perram1985`, :cite:`Perram1996`, :cite:`Donev2005`). See example scripts :woosrc:`examples/ell0.py` and :woosrc:`examples/ell1.py`.\n\n.. youtube:: cBnz4el4qX8\n\n", \
		((bool,brent,true,,"Use Brent iteration for finding maximum of the Perram-Wertheim potential. If false, use Newton-Raphson (not yet implemented).")) \
		((int,brentBits,4*sizeof(Real),,"Precision for the Brent method, as number of bits."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Ellipsoid_Ellipsoid_L6Geom__CLASS_BASE_DOC_ATTRS);
	FUNCTOR2D(Ellipsoid,Ellipsoid);
	DEFINE_FUNCTOR_ORDER_2D(Ellipsoid,Ellipsoid);
	//WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Ellipsoid_Ellipsoid_L6Geom);

struct Cg2_Sphere_Ellipsoid_L6Geom: public Cg2_Ellipsoid_Ellipsoid_L6Geom{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	void setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Sphere_Ellipsoid_L6Geom__CLASS_BASE_DOC \
		Cg2_Sphere_Ellipsoid_L6Geom,Cg2_Ellipsoid_Ellipsoid_L6Geom,"Compute the geometry of :obj:`~woo.dem.Ellipsoid` + :obj:`~woo.dem.Sphere` collision. Uses the code from :obj:`Cg2_Ellipsoid_Ellipsoid_L6Geom`, representing the sphere as an ellipsoid with all semiaxes equal to the radius."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Sphere_Ellipsoid_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Sphere,Ellipsoid);
	DEFINE_FUNCTOR_ORDER_2D(Sphere,Ellipsoid);
};
WOO_REGISTER_OBJECT(Cg2_Sphere_Ellipsoid_L6Geom);

struct Cg2_Wall_Ellipsoid_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Wall_Ellipsoid_L6Geom__CLASS_BASE_DOC \
		Cg2_Wall_Ellipsoid_L6Geom,Cg2_Any_Any_L6Geom__Base,"Compute :obj:`L6Geom` for contact of :obj:`ellipsoid <woo.dem.Ellipsoid>` and :obj:`wall <woo.dem.Wall>` (axis-aligned plane)."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Ellipsoid_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Wall,Ellipsoid);
	DEFINE_FUNCTOR_ORDER_2D(Wall,Ellipsoid);
};
WOO_REGISTER_OBJECT(Cg2_Wall_Ellipsoid_L6Geom);

struct Cg2_Facet_Ellipsoid_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Facet_Ellipsoid_L6Geom__CLASS_BASE_DOC \
		Cg2_Facet_Ellipsoid_L6Geom,Cg2_Any_Any_L6Geom__Base,"Compute :obj:`L6Geom` for contact of :obj:`ellipsoid <woo.dem.Ellipsoid>` and :obj:`facet <woo.dem.Facet>` (axis-aligned plane)."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Ellipsoid_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Facet,Ellipsoid);
	DEFINE_FUNCTOR_ORDER_2D(Facet,Ellipsoid);
};
WOO_REGISTER_OBJECT(Cg2_Facet_Ellipsoid_L6Geom);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Ellipsoid: public Gl1_Sphere{
	virtual void go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2,const GLViewInfo& glInfo){
		Gl1_Sphere::renderScaledSphere(shape,shift,wire2,glInfo,/*radius*/1.0,shape->cast<Ellipsoid>().semiAxes);
	}
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Ellipsoid,Gl1_Sphere,"Renders :obj:`woo.dem.Ellipsoid` object",);
	RENDERS(Ellipsoid);
};
WOO_REGISTER_OBJECT(Gl1_Ellipsoid);
#endif

