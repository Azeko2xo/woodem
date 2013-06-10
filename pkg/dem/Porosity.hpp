#pragma once

#include<woo/pkg/dem/Particle.hpp>
#include<woo/core/Engine.hpp>

#include<woo/lib/sphere-pack/SpherePack.hpp>

struct AnisoPorosityAnalyzer: public GlobalEngine {
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	DemField* dem;
	SpherePack pack;
	virtual void run();
	static vector<Vector3r> splitRay(Real theta, Real phi, Vector3r pt0=Vector3r::Zero(), const Matrix3r& T=Matrix3r::Identity());
	Real relSolid(Real theta, Real phi, Vector3r pt0=Vector3r::Zero(), bool vis=false);
	// _check variants to be called from python (safe scene setup etc)
	Real computeOneRay_check(const Vector3r& A, const Vector3r& B, bool vis=true);
	Real computeOneRay_angles_check(Real theta, Real phi, bool vis=true);
	void clearVis(){ rayIds.clear(); rayPts.clear(); }

	Real computeOneRay(const Vector3r& A, const Vector3r& B, bool vis=false);
	void initialize();
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(AnisoPorosityAnalyzer,GlobalEngine,"Engine which analyzes current scene and computes directionaly porosity value by intersecting spheres with lines. The algorithm only works on periodic simulations.",
		((Matrix3r,poro,Matrix3r::Zero(),AttrTrait<Attr::readonly>(),"Store analysis result here"))
		((int,div,10,,"Fineness of division of interval (0…1) for $u$,$v$ ∈〈0…1〉, which are used for uniform distribution over the positive octant as $\\theta=\frac{\\pi}{2}u$, $\\phi=\\acos v$ (see http://mathworld.wolfram.com/SpherePointPicking.html)"))
		// check that data are up-to-date
		((long,initStep,-1,AttrTrait<Attr::hidden>(),"Step in which internal data were last updated"))
		((size_t,initNum,-1,AttrTrait<Attr::hidden>(),"Number of particles at last update"))
		((vector<Particle::id_t>,rayIds,,AttrTrait<Attr::readonly>(),"Particles intersected with ray when *oneRay* was last called from python."))
		((vector<Vector3r>,rayPts,,AttrTrait<Attr::readonly>(),"Starting and ending points of segments intersecting particles."))
		,/*ctor*/
		,/*py*/
			.def("oneRay",&AnisoPorosityAnalyzer::computeOneRay_check,(py::arg("A"),py::arg("B")=Vector3r(Vector3r::Zero()),py::arg("vis")=true))
			.def("oneRay",&AnisoPorosityAnalyzer::computeOneRay_angles_check,(py::arg("theta"),py::arg("phi"),py::arg("vis")=true))
			.def("splitRay",&AnisoPorosityAnalyzer::splitRay,(py::arg("theta"),py::arg("phi"),py::arg("pt0")=Vector3r::Zero().eval(),py::arg("T")=Matrix3r::Identity().eval())).staticmethod("splitRay")
			.def("relSolid",&AnisoPorosityAnalyzer::relSolid,(py::arg("theta"),py::arg("phi"),py::arg("pt0")=Vector3r::Zero().eval(),py::arg("vis")=false))
			.def("clearVis",&AnisoPorosityAnalyzer::clearVis,"Clear visualizable intersection segments")
	);
};
WOO_REGISTER_OBJECT(AnisoPorosityAnalyzer);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Renderer.hpp>

class GlExtra_AnisoPorosityAnalyzer: public GlExtraDrawer{
	public:
	DECLARE_LOGGER;
	virtual void render();
	Real idColor(int id){ return (id%idMod)*1./(idMod-1); }
	WOO_CLASS_BASE_DOC_ATTRS(GlExtra_AnisoPorosityAnalyzer,GlExtraDrawer,"Find an instance of :ref:`LawTester` and show visually its data.",
		((shared_ptr<AnisoPorosityAnalyzer>,analyzer,,AttrTrait<>().noGui(),"Associated :ref:`AnisoPorosityAnalyzer` object."))
		((int,wd,2,,"Segment line width"))
		((Vector2i,wd_range,Vector2i(1,10),AttrTrait<>().noGui(),"Range for wd"))
		((int,num,2,,"Number to show at the segment middle: 0 = nothing, 1 = particle id, 2 = intersected length"))
		((Vector2i,num_range,Vector2i(0,2),AttrTrait<>().noGui(),"Range for num"))
		((int,idMod,5,,"Modulate particle id by this number to get segment color"))
	);
};
WOO_REGISTER_OBJECT(GlExtra_AnisoPorosityAnalyzer);
#endif