#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/L6Geom.hpp>

namespace woo{
	struct Ellipsoid: public Shape{
		void selfTest(const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
		bool numNodesOk() const { return nodes.size()==1; }
		// update dynamic properties (mass, intertia) of the sphere based on current radius
		void updateDyn(const Real& density) const;
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
	WOO_CLASS_BASE_DOC_ATTRS(Bo1_Ellipsoid_Aabb,Bo1_Sphere_Aabb,"Functor creating :obj:`Aabb` from :obj:`Ellipsoid`.\n\n.. todo:: For now, this is a simplified implementation returning bounding box of of the bounding spheres.\n\n.. warning:: :obj:`woo.dem.Bo1_Sphere_Aabb.distFactor` is ignored.",
	);
};
WOO_REGISTER_OBJECT(Bo1_Ellipsoid_Aabb);

struct Cg2_Ellipsoid_Ellipsoid_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	void setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	WOO_CLASS_BASE_DOC_ATTRS(Cg2_Ellipsoid_Ellipsoid_L6Geom,Cg2_Any_Any_L6Geom__Base,"Incrementally compute :obj:`L6Geom` for contact of 2 :obj:`ellipsoids <woo.dem.Ellipsoids>`. Uses the Perram-Wertheim potential function (:cite:`Perram1985`, :cite:`Perram1996`, :cite:`Donev2005`).\n\n.. youtube:: cBnz4el4qX8\n\n",
		((bool,brent,true,,"Use Brent iteration for finding maximum of the Perram-Wertheim potential. If false, use Newton-Raphson (not yet implemented)."))
		((int,brentBits,4*sizeof(Real),,"Precision for the Brent method, as number of bits."))
	);
	FUNCTOR2D(Ellipsoid,Ellipsoid);
	DEFINE_FUNCTOR_ORDER_2D(Ellipsoid,Ellipsoid);
	//DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Ellipsoid_Ellipsoid_L6Geom);


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

