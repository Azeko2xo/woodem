#pragma once
#include<woo/core/Engine.hpp>
#include<woo/pkg/dem/Particle.hpp>

#ifdef WOO_OPENGL
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<woo/lib/base/CompUtils.hpp>
#endif

struct BoxDeleter: public PeriodicEngine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&){
			if(isnan(glColor)) return;
			GLUtils::AlignedBox(box,CompUtils::mapColor(glColor));
			std::ostringstream oss; oss.precision(4); oss<<mass;
			if(!isnan(currRate)){ oss.precision(3); oss<<"\n("<<currRate<<")"; }
			GLUtils::GLDrawText(oss.str(),box.center(),CompUtils::mapColor(glColor));
		}
	#endif
	void run();
	py::object pyPsd(bool mass, bool cumulative, bool normalize, int num, const Vector2r& dRange, bool zip);
	py::tuple pyDiamMass() const;
	Real pyMassOfDiam(Real min, Real max) const ;
	void pyClear(){ deleted.clear(); mass=0.; num=0; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(BoxDeleter,PeriodicEngine,"Delete particles which fall outside (or inside, if *inside* is True) given box. Deleted particles are optionally stored in the *deleted* array for later processing, if needed.",
		((AlignedBox3r,box,AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),,"Box volume specification (lower and upper corners)"))
		((int,mask,0,,"If non-zero, only particles matching the mask will be candidates for removal"))
		((bool,inside,false,,"Delete particles which fall inside the volume rather than outside"))
		((bool,save,false,,"Save particles which are deleted in the *deleted* list"))
		((bool,recoverRadius,false,,"Recover radius of Spheres by computing it back from particle's mass and its material density (used when radius is changed due to radius thinning (in Law2_L6Geom_PelletPhys_Pellet.thinningFactor)."))
		((vector<shared_ptr<Particle>>,deleted,,AttrTrait<Attr::readonly>().noGui(),"Deleted particle's list; can be cleared with BoxDeleter.clear()"))
		((int,num,0,AttrTrait<Attr::readonly>(),"Number of deleted particles"))
		((Real,mass,0.,AttrTrait<Attr::readonly>(),"Total mass of deleted particles"))
		((Real,glColor,0,AttrTrait<>().noGui(),"Color for rendering (NaN disables rendering)"))
		//
		((Real,currRate,NaN,AttrTrait<>().readonly(),"Current value of mass flow rate"))
		((Real,currRateSmooth,1,AttrTrait<>().range(Vector2r(0,1)),"Smoothing factor for currRate ∈〈0,1〉"))
		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene.energy"))
		,/*ctor*/
		,/*py*/
		.def("psd",&BoxDeleter::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("num")=80,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("zip")=false),"Return particle size distribution of deleted particles (only useful with *save*), spaced between *dRange* (a 2-tuple of minimum and maximum radius); )")
		.def("clear",&BoxDeleter::pyClear,"Clear information about saved particles (particle list, if saved, mass and number)")
		.def("diamMass",&BoxDeleter::pyDiamMass,"Return 2-tuple of same-length list of diameters and masses.")
		.def("massOfDiam",&BoxDeleter::pyMassOfDiam,(py::arg("min")=0,py::arg("max")=Inf),"Return mass of particles of which diameters are between *min* and *max*.")
	);
};
WOO_REGISTER_OBJECT(BoxDeleter);

