#pragma once

#include<yade/pkg/dem/Particle.hpp>
#include<yade/core/Engine.hpp>

#include<yade/lib/sphere-pack/SpherePack.hpp>

struct AnisoPorosityAnalyzer: public GlobalEngine, private DemField::Engine{
	DemField* dem;
	SpherePack pack;
	virtual void run();
	static vector<Vector3r> splitRay(Real theta, Real phi, Vector3r pt0=Vector3r::Zero(), const Matrix3r& T=Matrix3r::Identity());
	Real relSolid(Real theta, Real phi, Vector3r pt0=Vector3r::Zero());
	// _check variants to be called from python (safe scene setup etc)
	Real computeOneRay_check(const Vector3r& A, const Vector3r& B, bool vis=true);
	Real computeOneRay_angles_check(Real theta, Real phi, bool vis=true);

	Real computeOneRay(const Vector3r& A, const Vector3r& B, bool vis=false);
	void initialize();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(AnisoPorosityAnalyzer,GlobalEngine,"Engine which analyzes current scene and computes directionaly porosity value by intersecting spheres with lines. The algorithm only works on periodic simulations.",
		((Matrix3r,poro,Matrix3r::Zero(),Attr::readonly,"Store analysis result here"))
		((int,div,10,,"Fineness of division of interval (0…1) for $u$,$v$ ∈〈0…1〉, which are used for uniform distribution over the positive octant as $\\theta=\frac{\\pi}{2}u$, $\\phi=\\acos v$ (see http://mathworld.wolfram.com/SpherePointPicking.html)"))
		// check that data are up-to-date
		((long,initStep,-1,Attr::hidden,"Step in which internal data were last updated"))
		((size_t,initNum,-1,Attr::hidden,"Number of particles at last update"))
		((vector<Particle::id_t>,rayIds,,Attr::readonly,"Particles intersected with ray when *oneRay* was last called from python."))
		((vector<Vector3r>,rayPts,,Attr::readonly,"Starting and ending points of segments intersecting particles."))
		,/*ctor*/
		,/*py*/
			.def("oneRay",&AnisoPorosityAnalyzer::computeOneRay_check,(py::arg("A"),py::arg("B")=Vector3r(Vector3r::Zero()),py::arg("vis")=true))
			.def("oneRay",&AnisoPorosityAnalyzer::computeOneRay_angles_check,(py::arg("theta"),py::arg("phi"),py::arg("vis")=true))
			.def("splitRay",&AnisoPorosityAnalyzer::splitRay,(py::arg("theta"),py::arg("phi"),py::arg("pt0")=Vector3r::Zero().eval(),py::arg("T")=Matrix3r::Identity().eval())).staticmethod("splitRay")
			.def("relSolid",&AnisoPorosityAnalyzer::relSolid,(py::arg("theta"),py::arg("phi"),py::arg("pt0")=Vector3r::Zero().eval()))
	);
};
REGISTER_SERIALIZABLE(AnisoPorosityAnalyzer);


#ifdef YADE_OPENGL
#include<yade/pkg/gl/Renderer.hpp>

class GlExtra_AnisoPorosityAnalyzer: public GlExtraDrawer{
	public:
	DECLARE_LOGGER;
	virtual void render();
	YADE_CLASS_BASE_DOC_ATTRS(GlExtra_AnisoPorosityAnalyzer,GlExtraDrawer,"Find an instance of :yref:`LawTester` and show visually its data.",
		((shared_ptr<AnisoPorosityAnalyzer>,analyzer,,Attr::noGui,"Associated :yref:`AnisoPorosityAnalyzer` object."))
		((int,wd,2,,"Segment line width"))
		((bool,ids,true,,"Show particle id in the middle of the segment"))
		((Vector2i,wd_range,Vector2i(1,10),Attr::noGui,"Range for wd"))
	);
};
REGISTER_SERIALIZABLE(GlExtra_AnisoPorosityAnalyzer);
#endif
