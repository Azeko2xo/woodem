
#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/Sphere.hpp>
#include<yade/pkg/dem/Facet.hpp>
#include<yade/pkg/dem/Truss.hpp>
#include<yade/pkg/dem/Wall.hpp>
#include<yade/pkg/dem/InfCylinder.hpp>

#ifdef YADE_OPENGL
	#include<yade/pkg/gl/Functors.hpp>
#endif

struct L6Geom: public CGeom{
	Real getMinRefLen(){ return (lens[0]<=0?lens[1]:(lens[1]<=0?lens[0]:lens.minCoeff())); }
	// set trsf with locX, and orient y and z arbitrarily
	void setInitialLocalCoords(const Vector3r& locX);
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(L6Geom,CGeom,"Geometry of particles in contact, defining relative velocities.",
		((Vector3r,vel,Vector3r::Zero(),,"Relative displacement rate in local coordinates, defined by :yref:`CGeom.node`"))
		((Vector3r,angVel,Vector3r::Zero(),,"Relative rotation rate in local coordinates"))
		((Real,uN,NaN,,"Normal displacement, distace of separation of particles (mathematically equal to integral of vel[0], but given here for numerically more stable results, as this component can be usually computed directly)."))
		((Vector2r,lens,Vector2r::Zero(),,"Hint for Gp2 functor on how to distribute material stiffnesses according to lengths on both sides of the contact; their sum should be equal to the initial contact length."))
		((Real,contA,NaN,,"(Fictious) contact area, used by Gp2 functor to compute stiffness."))
		((Matrix3r,trsf,Matrix3r::Identity(),,"Transformation (rotation) from global to local coordinates; only used internally, and is synchronized with :yref:`Node.ori` automatically. If the algorithm works with pure quaternions at some point (it is not stable now), can be removed safely."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(L6Geom,CGeom);
};
REGISTER_SERIALIZABLE(L6Geom);

struct Cg2_Sphere_Sphere_L6Geom: public CGeomFunctor{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	// common code for {sphere,facet,wall}+sphere contacts
	void handleSpheresLikeContact(const shared_ptr<Contact>& C, const Vector3r& pos1, const Vector3r& vel1, const Vector3r& angVel1, const Vector3r& pos2, const Vector3r& vel2, const Vector3r& angVel2, const Vector3r& normal, const Vector3r& contPt, Real uN, Real r1, Real r2);

	enum { APPROX_NO_MID_NORMAL=1, APPROX_NO_RENORM_MID_NORMAL=2, APPROX_NO_MID_TRSF=4, APPROX_NO_MID_BRANCH=8 };

	YADE_CLASS_BASE_DOC_ATTRS(Cg2_Sphere_Sphere_L6Geom,CGeomFunctor,"Incrementally compute :yref:`L6Geom` for contact of 2 spheres. Detailed documentation in py/_extraDocs.py",
		((bool,noRatch,false,,"FIXME: document what it really does."))
		((Real,distFactor,1,,"Create interaction if spheres are not futher than |distFactor|*(r1+r2). If negative, zero normal deformation will be set to be the initial value (otherwise, the geometrical distance is the 'zero' one)."))
		((int,trsfRenorm,100,,"How often to renormalize :yref:`trsf<L6Geom.trsf>`; if non-positive, never renormalized (simulation might be unstable)"))
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
REGISTER_SERIALIZABLE(Cg2_Sphere_Sphere_L6Geom);

struct Cg2_Facet_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	YADE_CLASS_BASE_DOC_ATTRS(Cg2_Facet_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :yref:`L6Geom` for contact between :yref:`Facet` and :yref:`Sphere`. Uses attributes of :yref:`Cg2_Sphere_Sphere_L6Geom`.",
		#if 0
			((unsigned int,centralSphereMask,0,,"If non-zero, for particles with sphere's matching mask compute the contact such that the equilibrium position is when the sphere's center is on the facet."))
		#endif
	);
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Facet+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	FUNCTOR2D(Facet,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Facet,Sphere);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Cg2_Facet_Sphere_L6Geom);

struct Cg2_Wall_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	YADE_CLASS_BASE_DOC(Cg2_Wall_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :yref:`L6Geom` for contact between :yref:`Wall` and :yref:`Sphere`. Uses attributes of :yref:`Cg2_Sphere_Sphere_L6Geom`.");
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Wall+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	FUNCTOR2D(Wall,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Wall,Sphere);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Cg2_Wall_Sphere_L6Geom);

struct Cg2_InfCylinder_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	YADE_CLASS_BASE_DOC(Cg2_InfCylinder_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :yref:`L6Geom` for contact between :yref:`InfCylinder` and :yref:`Sphere`. Uses attributes of :yref:`Cg2_Sphere_Sphere_L6Geom`.");
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be InfCylinder+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	FUNCTOR2D(InfCylinder,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(InfCylinder,Sphere);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Cg2_InfCylinder_Sphere_L6Geom);

struct Cg2_Truss_Sphere_L6Geom: public Cg2_Sphere_Sphere_L6Geom{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Truss+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }

	YADE_CLASS_BASE_DOC(Cg2_Truss_Sphere_L6Geom,Cg2_Sphere_Sphere_L6Geom,"Incrementally compute :yref:`L6Geom` for contact between :yref:`Truss` and :yref:`Sphere`. Uses attributes of :yref:`Cg2_Sphere_Sphere_L6Geom`.");
	FUNCTOR2D(Truss,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Truss,Sphere);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Cg2_Truss_Sphere_L6Geom);

#if 0

#ifdef L3GEOM_SPHERESLIKE
struct Ig2_Facet_Sphere_L3Geom: public Ig2_Sphere_Sphere_L3Geom{
	// get point on segment A..B closest to P; algo: http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
	static Vector3r getClosestSegmentPt(const Vector3r& P, const Vector3r& A, const Vector3r& B){ Vector3r BA=B-A; Real u=(P.dot(BA)-A.dot(BA))/(BA.squaredNorm()); return A+min(1.,max(0.,u))*BA; }
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const State& state1, const State& state2, const Vector3r& shift2, const bool& force, const shared_ptr<Interaction>& I);
	YADE_CLASS_BASE_DOC(Ig2_Facet_Sphere_L3Geom,Ig2_Sphere_Sphere_L3Geom,"Incrementally compute :yref:`L3Geom` for contact between :yref:`Facet` and :yref:`Sphere`. Uses attributes of :yref:`Ig2_Sphere_Sphere_L3Geom`.");
	FUNCTOR2D(Facet,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Facet,Sphere);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ig2_Facet_Sphere_L3Geom);
#endif

struct Ig2_Sphere_Sphere_L6Geom: public Ig2_Sphere_Sphere_L3Geom{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const State& state1, const State& state2, const Vector3r& shift2, const bool& force, const shared_ptr<Interaction>& I);
	YADE_CLASS_BASE_DOC(Ig2_Sphere_Sphere_L6Geom,Ig2_Sphere_Sphere_L3Geom,"Incrementally compute :yref:`L6Geom` for contact of 2 spheres.");
	FUNCTOR2D(Sphere,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Sphere,Sphere);
};
REGISTER_SERIALIZABLE(Ig2_Sphere_Sphere_L6Geom);


struct Law2_L3Geom_FrictPhys_ElPerfPl: public LawFunctor{
	virtual void go(shared_ptr<IGeom>&, shared_ptr<IPhys>&, Interaction*);
	FUNCTOR2D(L3Geom,FrictPhys);
	YADE_CLASS_BASE_DOC_ATTRS(Law2_L3Geom_FrictPhys_ElPerfPl,LawFunctor,"Basic law for testing :yref:`L3Geom`; it bears no cohesion (unless *noBreak* is ``True``), and plastic slip obeys the Mohr-Coulomb criterion (unless *noSlip* is ``True``).",
		((bool,noBreak,false,,"Do not break contacts when particles separate."))
		((bool,noSlip,false,,"No plastic slipping."))
		((int,plastDissipIx,-1,(Attr::noSave|Attr::hidden),"Index of plastically dissipated energy"))
		((int,elastPotentialIx,-1,(Attr::hidden|Attr::noSave),"Index for elastic potential energy (with O.trackEnergy)"))
	);
};
REGISTER_SERIALIZABLE(Law2_L3Geom_FrictPhys_ElPerfPl);

struct Law2_L6Geom_FrictPhys_Linear: public Law2_L3Geom_FrictPhys_ElPerfPl{
	virtual void go(shared_ptr<IGeom>&, shared_ptr<IPhys>&, Interaction*);
	FUNCTOR2D(L6Geom,FrictPhys);
	YADE_CLASS_BASE_DOC_ATTRS(Law2_L6Geom_FrictPhys_Linear,Law2_L3Geom_FrictPhys_ElPerfPl,"Basic law for testing :yref:`L6Geom` -- linear in both normal and shear sense, without slip or breakage.",
		((Real,charLen,1,,"Characteristic length with the meaning of the stiffness ratios bending/shear and torsion/normal."))
	);
};
REGISTER_SERIALIZABLE(Law2_L6Geom_FrictPhys_Linear);
#endif

#if 0
#ifdef YADE_OPENGL
struct Gl1_L6Geom: public GlCGeomFunctor{
	RENDERS(L6Geom);
	void go(const shared_ptr<CGeom>&, const shared_ptr<Contact>&, bool);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_L3Geom,GlCGeomFunctor,"Render :yref:`L3Geom` geometry.",
		((bool,axesLabels,false,,"Whether to display labels for local axes (x,y,z)"))
		((Real,axesScale,1.,,"Scale local axes, their reference length being half of the minimum radius."))
		((int,axesWd,1,,"Width of axes lines, in pixels; not drawn if non-positive"))
		((Vector2i,axesWd_range,Vector2i(0,10),Attr::noGui,"Range for axesWd."))
		//((int,uPhiWd,2,,"Width of lines for drawing displacements (and rotations for :yref:`L6Geom`); not drawn if non-positive."))
		//((Vector2i,uPhiWd_range,Vector2i(0,10),Attr::noGui,"Range for uPhiWd."))
		//((Real,uScale,1.,,"Scale local displacements (:yref:`u<L3Geom.u>` - :yref:`u0<L3Geom.u0>`); 1 means the true scale, 0 disables drawing local displacements; negative values are permissible."))
	);
};
REGISTER_SERIALIZABLE(Gl1_L6Geom);
#endif
#endif
