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
			#ifndef WOO_OLDABI
				if(node){ glPushMatrix(); GLUtils::setLocalCoords(node->pos,node->ori); }
			#endif
			GLUtils::AlignedBox(box,CompUtils::mapColor(glColor));
			if(glHideZero && (isnan(currRate) || currRate==0) && mass==0) return;
			std::ostringstream oss; oss.precision(4); oss<<mass;
			if(!isnan(currRate)){ oss.precision(3); oss<<"\n("<<currRate<<")"; }
			GLUtils::GLDrawText(oss.str(),box.center(),CompUtils::mapColor(glColor));
			#ifndef WOO_OLDABI
				if(node) glPopMatrix();
			#endif
		}
	#endif

	#ifndef WOO_OLDABI
	bool boxContains(const Vector3r& p){ if(!node) return box.contains(p); else return box.contains(node->ori.conjugate()*(p-node->pos)); }
	#else
		bool boxContains(const Vector3r& p){ return box.contains(p); }
	#endif
	void run();
	py::object pyPsd(bool mass, bool cumulative, bool normalize, int num, const Vector2r& dRange, bool zip, bool emptyOk);
	py::object pyDiamMass(bool zipped=false) const;
	Real pyMassOfDiam(Real min, Real max) const ;
	void pyClear(){ diamMass.clear(); rDivR0.clear(); mass=0.; num=0; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(BoxDeleter,PeriodicEngine,"Delete/mark particles which fall outside (or inside, if *inside* is True) given box. Deleted/mark particles are optionally stored in the *diamMass* array for later processing, if needed.\n\nParticle are deleted when :obj:`markMask` is 0, otherwise they are only marked with :obj:`markMask` and not deleted.",
		((uint,markMask,0,,"When non-zero, switch to marking mode -- particles of which :obj:`Particle.mask` does not comtain :obj:`mark` (i.e. ``(mask&mark)!=mark``) have :obj:`mark` bit-added to :obj:`Particle.mask` (this can happen only once for each particle); particles are not deleted, but their diameter/mass added to :obj:`diamMass` if :obj:`save` is True."))
		((AlignedBox3r,box,AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),,"Box volume specification (lower and upper corners). If :obj:`node` is specified, the box is in local coordinates; otherwise, global coorinates are used."))
		#ifndef WOO_OLDABI
			((shared_ptr<Node>,node,,,"Node specifying local coordinates; if not given :obj:`box` is in global coords."))
		#endif
		((uint,mask,0,,"If non-zero, only particles matching the mask will be candidates for removal"))
		((bool,inside,false,,"Delete particles which fall inside the volume rather than outside"))
		((bool,save,false,,"Save particles which are deleted in the *diamMass* list"))
		((bool,recoverRadius,false,,"Recover radius of Spheres by computing it back from particle's mass and its material density (used when radius is changed due to radius thinning (in Law2_L6Geom_PelletPhys_Pellet.thinningFactor). When radius is recovered, the :math:`r/r_0` ratio is added to :obj:`rDivR0` for further processing."))
		((vector<Real>,rDivR0,,AttrTrait<>().noGui().readonly(),"List of the :math:`r/r_0` ratio of deleted particles, when :obj:`recoverRadius` is true."))
		((vector<Vector2r>,diamMass,,AttrTrait<>().noGui().readonly(),"Radii and masses of deleted particles; not accessible from python (shadowed by the diamMass method)."))
		((int,num,0,AttrTrait<Attr::readonly>(),"Number of deleted particles"))
		((Real,mass,0.,AttrTrait<Attr::readonly>(),"Total mass of deleted particles"))
		((Real,glColor,0,AttrTrait<>().noGui(),"Color for rendering (NaN disables rendering)"))
		((bool,glHideZero,false,,"Show numbers (mass and rate) even if they are zero."))
		//
		((Real,currRate,NaN,AttrTrait<>().readonly(),"Current value of mass flow rate"))
		((Real,currRateSmooth,1,AttrTrait<>().range(Vector2r(0,1)),"Smoothing factor for currRate ∈〈0,1〉"))
		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene.energy"))
		,/*ctor*/
		,/*py*/
		.def("psd",&BoxDeleter::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("num")=80,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("zip")=false,py::arg("emptyOk")=false),"Return particle size distribution of deleted particles (only useful with *save*), spaced between *dRange* (a 2-tuple of minimum and maximum radius); )")
		.def("clear",&BoxDeleter::pyClear,"Clear information about saved particles (particle list, if saved, mass and number, rDivR0)")
		.def("diamMass",&BoxDeleter::pyDiamMass,(py::arg("zipped")=false),"With *zipped*, return list of (diameter, mass); without *zipped*, return tuple of 2 arrays, diameters and masses.")
		.def("massOfDiam",&BoxDeleter::pyMassOfDiam,(py::arg("min")=0,py::arg("max")=Inf),"Return mass of particles of which diameters are between *min* and *max*.")
	);
};
WOO_REGISTER_OBJECT(BoxDeleter);

