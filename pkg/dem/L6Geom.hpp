
#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Functors.hpp>
#endif

struct L6Geom: public CGeom{
	Real getMinRefLen(){ return (lens[0]<=0?lens[1]:(lens[1]<=0?lens[0]:lens.minCoeff())); }
	// set trsf with locX, and orient y and z arbitrarily
	void setInitialLocalCoords(const Vector3r& locX);
	#define woo_dem_L6Geom__CLASS_BASE_DOC_ATTRS_CTOR \
		L6Geom,CGeom,"Geometry of particles in contact, defining relative velocities.", \
		((Vector3r,vel,Vector3r::Zero(),AttrTrait<>().velUnit(),"Relative displacement rate in local coordinates, defined by :obj:`CGeom.node`")) \
		((Vector3r,angVel,Vector3r::Zero(),AttrTrait<>().angVelUnit(),"Relative rotation rate in local coordinates")) \
		((Real,uN,NaN,,"Normal displacement, distace of separation of particles (mathematically equal to integral of vel[0], but given here for numerically more stable results, as this component can be usually computed directly).")) \
		((Vector2r,lens,Vector2r::Zero(),AttrTrait<>().lenUnit(),"Hint for Cp2 functor on how to distribute material stiffnesses according to lengths on both sides of the contact; their sum should be equal to the initial contact length.")) \
		((Real,contA,NaN,AttrTrait<>().areaUnit(),"(Fictious) contact area, used by Cp2 functor to compute stiffness.")) \
		((Matrix3r,trsf,Matrix3r::Identity(),,"Transformation (rotation) from global to local coordinates; only used internally, and is synchronized with :obj:`woo.core.Node.ori` automatically. If the algorithm works with pure quaternions at some point (it is not stable now), can be removed safely.")) \
		, /*ctor*/ createIndex();
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_L6Geom__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(L6Geom,CGeom);
};
WOO_REGISTER_OBJECT(L6Geom);

struct Cg2_Any_Any_L6Geom__Base: public CGeomFunctor{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE { throw std::logic_error("Cg2_Any_Any_L6Geom__Base::go: Cg2_Any_Any_L6Geom__Base is an 'abstract' class which should not be used as-is; use derived classes."); }
	bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, the order is "+s1->getClassName()+"+"+s2->getClassName()+" (goReverse should never be called)."); }
	void handleSpheresLikeContact(const shared_ptr<Contact>& C, const Vector3r& pos1, const Vector3r& vel1, const Vector3r& angVel1, const Vector3r& pos2, const Vector3r& vel2, const Vector3r& angVel2, const Vector3r& normal, const Vector3r& contPt, Real uN, Real r1, Real r2);
	enum { APPROX_NO_MID_NORMAL=1, APPROX_NO_RENORM_MID_NORMAL=2, APPROX_NO_MID_TRSF=4, APPROX_NO_MID_BRANCH=8 };
	#define woo_dem_Cg2_Any_Any_L6Geom__Base__CLASS_BASE_DOC_ATTRS \
		Cg2_Any_Any_L6Geom__Base,CGeomFunctor,"Common base for L6Geom-computing functors such as :obj:`Cg2_SPhere_Sphere_L6Geom`, holding common approximation flags.", \
		((bool,noRatch,false,,"FIXME: document what it really does.")) \
		((int,trsfRenorm,100,,"How often to renormalize :obj:`trsf <L6Geom.trsf>`; if non-positive, never renormalized (simulation might be unstable)")) \
		((int,approxMask,0,AttrTrait<>().range(Vector2i(0,15)),"Selectively enable geometrical approximations (bitmask); add the values for approximations to be enabled.\n\n" \
		"== ===============================================================\n" \
		"1  use previous normal instead of mid-step normal for computing tangent velocity\n" \
		"2  do not re-normalize average (mid-step) normal, if used.\n" \
		"4  use previous rotation instead of mid-step rotation to transform velocities\n" \
		"8  use current branches instead of mid-step branches to evaluate incident velocity (used without noRatch)\n" \
		"== ===============================================================\n\n" \
		"By default, the mask is zero, wherefore none of these approximations is used.\n" ))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Any_Any_L6Geom__Base__CLASS_BASE_DOC_ATTRS);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Any_Any_L6Geom__Base);



#if 0
#ifdef WOO_OPENGL
struct Gl1_L6Geom: public GlCGeomFunctor{
	RENDERS(L6Geom);
	void go(const shared_ptr<CGeom>&, const shared_ptr<Contact>&, bool);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_L3Geom,GlCGeomFunctor,"Render :obj:`L3Geom` geometry.",
		((bool,axesLabels,false,,"Whether to display labels for local axes (x,y,z)"))
		((Real,axesScale,1.,,"Scale local axes, their reference length being half of the minimum radius."))
		((int,axesWd,1,,"Width of axes lines, in pixels; not drawn if non-positive"))
		((Vector2i,axesWd_range,Vector2i(0,10),AttrTrait<>().noGui(),"Range for axesWd."))
		//((int,uPhiWd,2,,"Width of lines for drawing displacements (and rotations for :obj:`L6Geom`); not drawn if non-positive."))
		//((Vector2i,uPhiWd_range,Vector2i(0,10),AttrTrait<>().noGui(),"Range for uPhiWd."))
		//((Real,uScale,1.,,"Scale local displacements (:obj:`u <L3Geom.u>` - :obj:`u0 <L3Geom.u0>`); 1 means the true scale, 0 disables drawing local displacements; negative values are permissible."))
	);
};
WOO_REGISTER_OBJECT(Gl1_L6Geom);
#endif
#endif
