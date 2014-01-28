#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/lib/sphere-pack/SpherePack.hpp>

struct SphereClumpGeom: public Object{
	DECLARE_LOGGER;
	void postLoad(SphereClumpGeom&,void*);
	// may fail when called from postLoad, but not from ensureOk()
	void recompute(int div, bool failOk=false, bool fastOnly=false);
	void makeInvalid(){ volume=equivRad=NaN; inertia=Vector3r(NaN,NaN,NaN); pos=Vector3r::Zero(); ori=Quaternionr::Identity(); }
	bool isOk() const { return !isnan(volume); }
	void ensureOk() { if(!isOk()) recompute(div,/*failOk*/false); }
	std::tuple<shared_ptr<Node>,vector<shared_ptr<Particle>>> makeClump(const shared_ptr<Material>&, const Vector3r& pos, const Quaternionr& ori, int mask, Real scale=1.);
	py::tuple pyMakeClump(const shared_ptr<Material>& m, const Vector3r& p, const Quaternionr& o=Quaternionr::Identity(), Real scale=1., int mask=0){
		const auto& tup=makeClump(m,p,o,mask,scale); return py::make_tuple(std::get<0>(tup),std::get<1>(tup));
	}
	static vector<shared_ptr<SphereClumpGeom>> fromSpherePack(const shared_ptr<SpherePack>& sp, int div=5);
	
	WOO_CLASS_BASE_DOC_ATTRS_PY(SphereClumpGeom,Object,"Defines geometry of spherical clumps. Each clump is described by spheres it is made of (position and radius).",
		((vector<Vector3r>,centers,,AttrTrait<Attr::triggerPostLoad>(),"Centers of constituent spheres, in clump-local coordinates."))
		((vector<Real>,radii,,AttrTrait<Attr::triggerPostLoad>(),"Radii of constituent spheres"))
		((vector<Vector2r>,scaleProb,,,"Used by particle generators: piecewise-linear function probability(equivRad) given as a sequence of x,y coordinates. If not given, constant function $p(d)=1$ is assumed. See the documentation of :obj:`woo.dem.PsdClumpGenerator` for details."))
		((Vector3r,pos,Vector3r::Zero(),AttrTrait<>().readonly().noDump(),"Centroid position (computed automatically)"))
		((Quaternionr,ori,Quaternionr::Identity(),AttrTrait<>().readonly().noDump(),"Principal axes orientation (computed automatically)"))
		((Real,volume,NaN,AttrTrait<>().readonly().noDump().volUnit(),"Volume (computed automatically)"))
		((Real,equivRad,NaN,AttrTrait<>().readonly().noDump().lenUnit(),"Equivalent radius of the clump (computed automatically) -- mean of radii of gyration, i.e. $\\frac{1}{3}\\sum \\sqrt{I_{ii}/V}$."))
		((Vector3r,inertia,Vector3r(NaN,NaN,NaN),AttrTrait<>().readonly().noDump(),"Geometrical inertia (computed with unit density)"))
		((int,div,5,AttrTrait<Attr::triggerPostLoad>().noDump(),"Sampling grid fineness, when computing volume and other properties, relative to the smallest sphere's radius. When zero or negative, assume spheres don't intersect and use a different algorithm (Steiner's theorem)."))
		, /* py*/
		.def("recompute",&SphereClumpGeom::recompute,(py::arg("div")=5,py::arg("failOk")=false,py::arg("fastOnly")=false),"Recompute principal axes of the clump, using *div* for subdivision (see :obj:`div` for the semantics). *failOk* (silently return in case of invalid data) and *fastOnly* (return if there is lots of cells in subdivision) are only to be used internally.")
		.def("makeClump",&SphereClumpGeom::pyMakeClump,(py::arg("mat"),py::arg("pos"),py::arg("ori")=Quaternionr::Identity(),py::arg("scale")=1.,py::arg("mask")=0),"Create particles as described by this clump geometry, positioned in *pos* and rotated with *ori*. Geometry will be scaled by *scale*. Returns tuple (Node,[Particle]).")
		.def("fromSpherePack",&SphereClumpGeom::fromSpherePack,(py::arg("pack"),py::arg("div")=5),"Return [ :obj:`SphereClumpGeom` ] which contain all clumps and spheres from given :obj:`SpherePack`.").staticmethod("fromSpherePack")
	);
};
WOO_REGISTER_OBJECT(SphereClumpGeom);



struct ClumpData: public DemData{
	static shared_ptr<Node> makeClump(const vector<shared_ptr<Node>>& nodes, shared_ptr<Node> centralNode=shared_ptr<Node>(), bool intersecting=false);
	// sum forces and torques from members; does not touch our data, adds to passed references F, T
	// only the integrator should modify DemData.{force,torque} directly
	static void collectFromMembers(const shared_ptr<Node>& node, Vector3r& F, Vector3r& T);
	// update member's positions and velocities
	static void applyToMembers(const shared_ptr<Node>&, bool resetForceTorque=false);
	static void resetForceTorque(const shared_ptr<Node>&);

	//! Recalculates inertia tensor of a body after translation away from (default) or towards its centroid.
	static Matrix3r inertiaTensorTranslate(const Matrix3r& I,const Real m, const Vector3r& off);
	//! Recalculate body's inertia tensor in rotated coordinates.
	static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Matrix3r& T);
	//! Recalculate body's inertia tensor in rotated coordinates.
	static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Quaternionr& rot);

	// compute pos, ori, inertia given mass, first and second-order momenum
	static void computePrincipalAxes(const Real& m, const Vector3r& Sg, const Matrix3r& Ig, Vector3r& pos, Quaternionr& ori, Vector3r& inertia);

	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS(ClumpData,DemData,"Data of a DEM particle which binds multiple particles together.",
		((vector<shared_ptr<Node>>,nodes,,AttrTrait<Attr::readonly>(),"Member nodes"))
		((vector<Vector3r>,relPos,,AttrTrait<Attr::readonly>(),"Relative member's positions"))
		((vector<Quaternionr>,relOri,,AttrTrait<Attr::readonly>(),"Relative member's orientations"))
		((Real,equivRad,NaN,,"Equivalent radius, for PSD statistics (e.g. in :obj:`BoxDeleter`)."))
	);
};
WOO_REGISTER_OBJECT(ClumpData);

