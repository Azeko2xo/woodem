#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/Sphere.hpp>

struct G3Geom: public CGeom{
	// rotates any contact-local vector expressed inglobal coordinates to keep track of local contact rotation in last step
	void rotateVectorWithContact(Vector3r& v);
	static Vector3r getIncidentVel(const DemData& dd1, const DemData& dd2, Real dt, const Vector3r& shift2, const Vector3r& shiftVel2, bool avoidGranularRatcheting, bool useAlpha);
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(G3Geom,CGeom,"Geometry of particles in contact, defining relative velocities.",
		((Real,uN,NaN,,"Normal displacement, distace of separation of particles (mathematically equal to integral of vel[0], but given here for numerically more stable results, as this component can be usually computed directly)."))
		((Vector3r,dShear,Vector3r::Zero(),,"Shear displacement delta during last step."))
		((Vector3r,twistAxis,Vector3r(NaN,NaN,NaN),Attr::readonly,"Axis of twisting rotation"))
		((Vector3r,orthonormalAxis,Vector3r(NaN,NaN,NaN),Attr::readonly,"Axis normal to twisting axis"))
		((Vector3r,normal,Vector3r(NaN,NaN,NaN),Attr::readonly,"Contact normal in global coordinates; G3Geom doesn't touch Contact.node.ori (which is identity), therefore orientation must be kep separately"))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(G3Geom,CGeom);
};
REGISTER_SERIALIZABLE(G3Geom);

struct Cg2_Sphere_Sphere_G3Geom: public CGeomFunctor{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	YADE_CLASS_BASE_DOC_ATTRS(Cg2_Sphere_Sphere_G3Geom,CGeomFunctor,"Incrementally compute :yref:`G3Geom` for contact of 2 spheres. Detailed documentation in py/_extraDocs.py",
		((bool,noRatch,true,,"FIXME: document what it really does."))
		((bool,useAlpha,true,,"Use alpha correction proposed by McNamara, see source code for details"))
	);
	FUNCTOR2D(Sphere,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Sphere,Sphere);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Cg2_Sphere_Sphere_G3Geom);

struct G3GeomCData: public CData{
	YADE_CLASS_BASE_DOC_ATTRS(G3GeomCData,CData,"Internal variables for use with G3Geom",
		((Vector3r,shearForce,Vector3r::Zero(),,"Shear force in global coordinates"))
	);
};
REGISTER_SERIALIZABLE(G3GeomCData);

struct Law2_G3Geom_FrictPhys_IdealElPl: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	FUNCTOR2D(G3Geom,FrictPhys);
	YADE_CLASS_BASE_DOC_ATTRS(Law2_G3Geom_FrictPhys_IdealElPl,LawFunctor,"Ideally elastic-plastic behavior, for use with G3Geom.",
		((bool,noSlip,false,,"Disable plastic slipping"))
		((bool,noBreak,false,,"Disable removal of contacts when in tension."))
		((int,plastDissipIx,-1,(Attr::noSave|Attr::hidden),"Index of plastically dissipated energy"))
		((int,elastPotIx,-1,(Attr::hidden|Attr::noSave),"Index for elastic potential energy"))
		#ifdef YADE_DEBUG
			((Vector2i,watch,Vector2i(-1,-1),,"Print debug information for this couple of IDs"))
		#endif
	);
};
REGISTER_SERIALIZABLE(Law2_G3Geom_FrictPhys_IdealElPl);


