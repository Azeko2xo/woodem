
#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Facet.hpp>
#include<woo/pkg/dem/Truss.hpp>
#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/InfCylinder.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Functors.hpp>
#endif

struct L6Geom: public CGeom{
	Real getMinRefLen(){ return (lens[0]<=0?lens[1]:(lens[1]<=0?lens[0]:lens.minCoeff())); }
	// set trsf with locX, and orient y and z arbitrarily
	void setInitialLocalCoords(const Vector3r& locX);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(L6Geom,CGeom,"Geometry of particles in contact, defining relative velocities.",
		((Vector3r,vel,Vector3r::Zero(),AttrTrait<>().velUnit(),"Relative displacement rate in local coordinates, defined by :ref:`CGeom.node`"))
		((Vector3r,angVel,Vector3r::Zero(),AttrTrait<>().angVelUnit(),"Relative rotation rate in local coordinates"))
		((Real,uN,NaN,,"Normal displacement, distace of separation of particles (mathematically equal to integral of vel[0], but given here for numerically more stable results, as this component can be usually computed directly)."))
		((Vector2r,lens,Vector2r::Zero(),AttrTrait<>().lenUnit(),"Hint for Gp2 functor on how to distribute material stiffnesses according to lengths on both sides of the contact; their sum should be equal to the initial contact length."))
		((Real,contA,NaN,AttrTrait<>().areaUnit(),"(Fictious) contact area, used by Gp2 functor to compute stiffness."))
		((Matrix3r,trsf,Matrix3r::Identity(),,"Transformation (rotation) from global to local coordinates; only used internally, and is synchronized with :ref:`Node.ori` automatically. If the algorithm works with pure quaternions at some point (it is not stable now), can be removed safely."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(L6Geom,CGeom);
};
WOO_REGISTER_OBJECT(L6Geom);

struct Cg2_Sphere_Sphere_L6Geom: public CGeomFunctor{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	// common code for {sphere,facet,wall}+sphere contacts
	void handleSpheresLikeContact(const shared_ptr<Contact>& C, const Vector3r& pos1, const Vector3r& vel1, const Vector3r& angVel1, const Vector3r& pos2, const Vector3r& vel2, const Vector3r& angVel2, const Vector3r& normal, const Vector3r& contPt, Real uN, Real r1, Real r2);

	enum { APPROX_NO_MID_NORMAL=1, APPROX_NO_RENORM_MID_NORMAL=2, APPROX_NO_MID_TRSF=4, APPROX_NO_MID_BRANCH=8 };

	WOO_CLASS_BASE_DOC_ATTRS(Cg2_Sphere_Sphere_L6Geom,CGeomFunctor,"Incrementally compute :ref:`L6Geom` for contact of 2 spheres. Detailed documentation in py/_extraDocs.py",
		((bool,noRatch,false,,"FIXME: document what it really does."))
		((Real,distFactor,1,,"Create interaction if spheres are not futher than |distFactor|*(r1+r2). If negative, zero normal deformation will be set to be the initial value (otherwise, the geometrical distance is the 'zero' one)."))
		((int,trsfRenorm,100,,"How often to renormalize :ref:`trsf<L6Geom.trsf>`; if non-positive, never renormalized (simulation might be unstable)"))
		((int,approxMask,0,AttrTrait<>().range(Vector2i(0,15)),"Selectively enable geometrical approximations (bitmask); add the values for approximations to be enabled.\n\n"
		"== ===============================================================\n"
		"1  use previous normal instead of mid-step normal for computing tangent velocity\n"
		"2  do not re-normalize average (mid-step) normal, if used.\n"
		"4  use previous rotation instead of mid-step rotation to transform velocities\n"
		"8  use current branches instead of mid-step branches to evaluate incident velocity (used without noRatch)\n"
		"== ===============================================================\n\n"
		"By default, the mask is zero, wherefore none of these approximations is used.\n"
		))
	);
	FUNCTOR2D(Sphere,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Sphere,Sphere);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Sphere_Sphere_L6Geom);

struct Cg2_Facet_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	WOO_CLASS_BASE_DOC_ATTRS(Cg2_Facet_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :ref:`L6Geom` for contact between :ref:`Facet` and :ref:`Sphere`. Uses attributes of :ref:`Cg2_Sphere_Sphere_L6Geom`.",
	);
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Facet+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	FUNCTOR2D(Facet,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Facet,Sphere);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Facet_Sphere_L6Geom);

struct Cg2_Wall_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	WOO_CLASS_BASE_DOC(Cg2_Wall_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :ref:`L6Geom` for contact between :ref:`Wall` and :ref:`Sphere`. Uses attributes of :ref:`Cg2_Sphere_Sphere_L6Geom`.");
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Wall+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	FUNCTOR2D(Wall,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Wall,Sphere);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Wall_Sphere_L6Geom);

struct Cg2_InfCylinder_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	WOO_CLASS_BASE_DOC(Cg2_InfCylinder_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :ref:`L6Geom` for contact between :ref:`InfCylinder` and :ref:`Sphere`. Uses attributes of :ref:`Cg2_Sphere_Sphere_L6Geom`.");
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be InfCylinder+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	FUNCTOR2D(InfCylinder,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(InfCylinder,Sphere);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_InfCylinder_Sphere_L6Geom);

struct Cg2_Truss_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Truss+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }

	WOO_CLASS_BASE_DOC(Cg2_Truss_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :ref:`L6Geom` for contact between :ref:`Truss` and :ref:`Sphere`. Uses attributes of :ref:`Cg2_Sphere_Sphere_L6Geom`.");
	FUNCTOR2D(Truss,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Truss,Sphere);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Truss_Sphere_L6Geom);


#if 0
#ifdef WOO_OPENGL
struct Gl1_L6Geom: public GlCGeomFunctor{
	RENDERS(L6Geom);
	void go(const shared_ptr<CGeom>&, const shared_ptr<Contact>&, bool);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_L3Geom,GlCGeomFunctor,"Render :ref:`L3Geom` geometry.",
		((bool,axesLabels,false,,"Whether to display labels for local axes (x,y,z)"))
		((Real,axesScale,1.,,"Scale local axes, their reference length being half of the minimum radius."))
		((int,axesWd,1,,"Width of axes lines, in pixels; not drawn if non-positive"))
		((Vector2i,axesWd_range,Vector2i(0,10),AttrTrait<>().noGui(),"Range for axesWd."))
		//((int,uPhiWd,2,,"Width of lines for drawing displacements (and rotations for :ref:`L6Geom`); not drawn if non-positive."))
		//((Vector2i,uPhiWd_range,Vector2i(0,10),AttrTrait<>().noGui(),"Range for uPhiWd."))
		//((Real,uScale,1.,,"Scale local displacements (:ref:`u<L3Geom.u>` - :ref:`u0<L3Geom.u0>`); 1 means the true scale, 0 disables drawing local displacements; negative values are permissible."))
	);
};
WOO_REGISTER_OBJECT(Gl1_L6Geom);
#endif
#endif
