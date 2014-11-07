#pragma once
#include<woo/core/Engine.hpp>
#include<woo/pkg/dem/Particle.hpp>

#ifdef WOO_OPENGL
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<woo/lib/base/CompUtils.hpp>
#endif

struct Outlet: public PeriodicEngine{
	WOO_DECL_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual bool isInside(const Vector3r& p) { throw std::runtime_error(pyStr()+" did not override Outlet::isInside."); }
	void run();
	py::object pyPsd(bool mass, bool cumulative, bool normalize, int num, const Vector2r& dRange, const Vector2r& tRange, bool zip, bool emptyOk);
	py::object pyDiamMass(bool zipped=false) const;
	py::object pyDiamMassTime(bool zipped=false) const;
	Real pyMassOfDiam(Real min, Real max) const ;
	void pyClear(){ diamMassTime.clear(); rDivR0.clear(); mass=0.; num=0; }
	#ifdef WOO_OPENGL
		void renderMassAndRate(const Vector3r& pos);
	#endif
	#define woo_dem_Outlet__CLASS_BASE_DOC_ATTRS_PY \
		Outlet,PeriodicEngine,"Delete/mark particles which fall outside (or inside, if *inside* is True) given box. Deleted/mark particles are optionally stored in the *diamMassTime* array for later processing, if needed.\n\nParticle are deleted when :obj:`markMask` is 0, otherwise they are only marked with :obj:`markMask` and not deleted.", \
		((uint,markMask,0,,"When non-zero, switch to marking mode -- particles of which :obj:`Particle.mask` does not comtain :obj:`markMask` (i.e. ``(mask&markMask)!=markMask``) have :obj:`markMask` bit-added to :obj:`Particle.mask` (this can happen only once for each particle); particles are not deleted, but their diameter/mass added to :obj:`diamMassTime` if :obj:`save` is True.")) \
		((uint,mask,((void)":obj:`DemField.defaultOutletMask`",DemField::defaultOutletMask),,"If non-zero, only particles matching the mask will be candidates for removal")) \
		((bool,inside,false,,"Delete particles which fall inside the volume rather than outside")) \
		((bool,save,false,,"Save particles which are deleted in the *diamMass* list")) \
		((bool,recoverRadius,false,,"Recover radius of Spheres by computing it back from particle's mass and its material density (used when radius is changed due to radius thinning (in Law2_L6Geom_PelletPhys_Pellet.thinningFactor). When radius is recovered, the :math:`r/r_0` ratio is added to :obj:`rDivR0` for further processing.")) \
		((vector<Real>,rDivR0,,AttrTrait<>().noGui().readonly(),"List of the :math:`r/r_0` ratio of deleted particles, when :obj:`recoverRadius` is true.")) \
		((vector<Vector3r>,diamMassTime,,AttrTrait<>().noGui().readonly(),"Radii and masses of deleted particles; not accessible from python (shadowed by the :obj:`diamMassTime` method).")) \
		((vector<Real>,timeOut,,AttrTrait<>().noGui().readonly(),"Time when particles were deleted.")) \
		((int,num,0,AttrTrait<Attr::readonly>(),"Number of deleted particles")) \
		((Real,mass,0.,AttrTrait<Attr::readonly>(),"Total mass of deleted particles")) \
		((Real,glColor,0,AttrTrait<>().noGui(),"Color for rendering (NaN disables rendering)")) \
		((bool,glHideZero,false,,"Show numbers (mass and rate) even if they are zero.")) \
		((Real,currRate,NaN,AttrTrait<>().readonly(),"Current value of mass flow rate")) \
		((Real,currRateSmooth,1,AttrTrait<>().range(Vector2r(0,1)),"Smoothing factor for currRate ∈〈0,1〉")) \
		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene.energy")) \
		,/*py*/ \
		.def("psd",&Outlet::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=true,py::arg("num")=80,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("tRange")=Vector2r(NaN,NaN),py::arg("zip")=false,py::arg("emptyOk")=false),"Return particle size distribution of deleted particles (only useful with *save*), spaced between *dRange* (a 2-tuple of minimum and maximum radius); )") \
		.def("clear",&Outlet::pyClear,"Clear information about saved particles (particle list, if saved, mass and number, rDivR0)") \
		.def("diamMass",&Outlet::pyDiamMass,(py::arg("zipped")=false),"With *zipped*, return list of (diameter, mass); without *zipped*, return tuple of 2 arrays, diameters and masses.") \
		.def("diamMassTime",&Outlet::pyDiamMassTime,(py::arg("zipped")=false),"With *zipped*, return list of (diameter, mass, time); without *zipped*, return tuple of 3 arrays: diameters, masses, times.") \
		.def("massOfDiam",&Outlet::pyMassOfDiam,(py::arg("min")=0,py::arg("max")=Inf),"Return mass of particles of which diameters are between *min* and *max*.") 

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Outlet__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Outlet);


struct BoxOutlet: public Outlet{
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&);
	#endif
	bool isInside(const Vector3r& p) WOO_CXX11_OVERRIDE { return box.contains(node?node->glob2loc(p):p); }
	#define woo_dem_BoxOutlet__CLASS_BASE_DOC_ATTRS \
		BoxOutlet,Outlet,"Outlet with box geometry", \
		((AlignedBox3r,box,AlignedBox3r(),,"Box volume specification (lower and upper corners). If :obj:`node` is specified, the box is in local coordinates; otherwise, global coorinates are used.")) \
		((shared_ptr<Node>,node,,,"Node specifying local coordinates; if not given :obj:`box` is in global coords."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxOutlet__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(BoxOutlet);

struct ArcOutlet: public Outlet{
	bool isInside(const Vector3r& p) WOO_CXX11_OVERRIDE;
	void postLoad(ArcOutlet&, void* attr);
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&) WOO_CXX11_OVERRIDE;
	#endif
	#define woo_dem_ArcOutlet__CLASS_BASE_DOC_ATTRS \
		ArcOutlet,Outlet,"Outlet detnig/marking particles in prismatic arc (revolved rectangle) specified using `cylindrical coordinates <http://en.wikipedia.org/wiki/Cylindrical_coordinate_system>`__ (with the ``ISO 31-11`` convention, as mentioned at the Wikipedia page) in a local system. See also analogous :obj:`ArcInlet`.", \
		((shared_ptr<Node>,node,make_shared<Node>(),AttrTrait<Attr::triggerPostLoad>(),"Node defining local coordinates system. *Must* be given.")) \
		((AlignedBox3r,cylBox,,,"Box in cylindrical coordinates, as: (ρ₀,φ₀,z₀),(ρ₁,φ₁,z₁). ρ must be non-negative, (φ₁-φ₀)≤2π.")) \
		((int,glSlices,32,,"Number of slices for rendering circle (the arc takes the proportionate value"))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_ArcOutlet__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(ArcOutlet);
