#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<boost/range/numeric.hpp>
#include<boost/range/algorithm/fill.hpp>


#ifdef WOO_OPENGL
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<woo/lib/base/CompUtils.hpp>
#endif

struct ParticleFactory: public PeriodicEngine{
	//bool isActivated(){ return !((maxMass>0 && mass>maxMass) || (maxNum>0 && num>maxNum)); }
	// set current smoothed mass flow rate, given unsmoothed value from the current step
	// handles NaN values if there is no previous value
	void setCurrRate(Real currRateNoSmooth){
		if(isnan(currRate)||stepPrev<0) currRate=currRateNoSmooth;
		else currRate=(1-currRateSmooth)*currRate+currRateSmooth*currRateNoSmooth;
	}
	WOO_CLASS_BASE_DOC_ATTRS(ParticleFactory,PeriodicEngine,"Factory generating new particles. This is an abstract base class which in itself does not generate anything, but provides some unified interface to derived classes.",
		((Real,maxMass,-1,,"Mass at which the engine will not produce any particles (inactive if not positive)"))
		((long,maxNum,-1,,"Number of generated particles after which no more will be produced (inactive if not positive)"))
		((Real,mass,0,,"Generated mass total"))
		((long,num,0,,"Number of generated particles"))
		((Real,currRate,NaN,AttrTrait<>().readonly(),"Current value of mass flow rate"))
		((Real,currRateSmooth,1,AttrTrait<>().noGui().range(Vector2r(0,1)),"Smoothing factor for currRate ∈〈0,1〉"))
		((Real,glColor,0,AttrTrait<>().noGui(),"Color for rendering (nan disables rendering)"))
	);
};
REGISTER_SERIALIZABLE(ParticleFactory);

struct ParticleGenerator: public Object{
	// particle and two extents sizes (bbox if p is at origin)
	struct ParticleAndBox{ shared_ptr<Particle> par; AlignedBox3r extents; };
	// return (one or multiple, for clump) particles and extents (min and max)
	// extents are computed for position of (0,0,0)
	virtual vector<ParticleAndBox> operator()(const shared_ptr<Material>& m){ throw std::runtime_error("Calling ParticleGenerator.operator() (abstract method); use derived classes."); }
	virtual void clear(){ genDiamMass.clear(); }
	py::tuple pyPsd(bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) const;
	py::object pyDiamMass();
	py::list pyCall(const shared_ptr<Material>& m){ vector<ParticleAndBox> pee=(*this)(m); py::list ret; for(const auto& pe: pee) ret.append(py::make_tuple(pe.par,pe.extents)); return ret; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ParticleGenerator,Object,"Abstract class for generating particles",
		((vector<Vector2r>,genDiamMass,,AttrTrait<Attr::readonly>().noGui(),"List of generated particle's (equivalent) radii and masses (for making granulometry)"))
		((bool,save,true,,"Save generated particles so that PSD can be generated afterwards"))
		,/*ctor*/
		,/*py*/
			.def("psd",&ParticleGenerator::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("num")=80),"Return PSD for particles generated.")
			.def("diamMass",&ParticleGenerator::pyDiamMass,"Return tuple of 2 arrays, diameters and masses.")
			.def("clear",&ParticleGenerator::clear,"Clear stored data about generated particles; only subsequently generated particles will be considered in the PSD.")
			.def("__call__",&ParticleGenerator::pyCall,"Call the generation routine, returning one particle (at origin) and its bounding-box when at origin. Useful for debugging.")
	);
};
REGISTER_SERIALIZABLE(ParticleGenerator);

struct MinMaxSphereGenerator: public ParticleGenerator{
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m);
	WOO_CLASS_BASE_DOC_ATTRS(MinMaxSphereGenerator,ParticleGenerator,"Generate particles with given minimum and maximum radius",
		((Vector2r,dRange,Vector2r(NaN,NaN),,"Minimum and maximum radius of generated spheres"))
	);
};
REGISTER_SERIALIZABLE(MinMaxSphereGenerator);

struct PsdSphereGenerator: public ParticleGenerator{
	DECLARE_LOGGER;
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m);
	void postLoad(PsdSphereGenerator&);
	void clear(){ ParticleGenerator::clear(); weightTotal=0.; std::fill(weightPerBin.begin(),weightPerBin.end(),0.); }
	py::tuple pyInputPsd(bool scale, bool cumulative, int num) const;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(PsdSphereGenerator,ParticleGenerator,"Generate particles following a discrete Particle Size Distribution (PSD)",
		((bool,discrete,true,,"The points on the PSD curve will be interpreted as the only allowed diameter values; if *false*, linear interpolation between them is assumed instead. Do not change once the generator is running."))
		((vector<Vector2r>,psdPts,,AttrTrait<Attr::triggerPostLoad>(),"Points of the PSD curve; the first component is particle diameter [m] (not radius!), the second component is passing percentage. Both diameter and passing values must be increasing (diameters must be strictly increasing). Passing values are normalized so that the last value is 1.0 (therefore, you can enter the values in percents if you like)."))
		((bool,mass,true,,"PSD has mass percentages; if false, number of particles percentages are assumed. Do not change once the generator is running."))
		((vector<Real>,weightPerBin,,AttrTrait<>().noGui(),"Keep track of mass/number of particles for each point on the PSD so that we get as close to the curve as possible. Only used for discrete PSD."))
		((Real,weightTotal,,AttrTrait<>().noGui(),"Total mass/number of of particles generated. Only used for discrete PSD."))
		, /* ctor */
		, /* py */
			.def("inputPsd",&PsdSphereGenerator::pyInputPsd,(py::arg("scale")=false,py::arg("cumulative")=true,py::arg("num")=80),"Return input PSD; it will be a staircase function if *discrete* is true, otherwise linearly interpolated. With *scale*, the curve is multiplied with the actually generated mass/numer of particles (depending on whether *mass* is true or false); the result should then be very similar to the psd() output with actually generated spheres. Discrete non-cumulative PSDs are handled specially: discrete distributions return skypline plot with peaks represented as plateaus of the relative width 1/*num*; continuous distributions return ideal histogram computed for relative bin with 1/*num*; thus returned histogram will match non-cummulative histogram returned by `ParticleGenerator.psd(cumulative=False)`, provided *num* is the same in both cases.")
	);
};
REGISTER_SERIALIZABLE(PsdSphereGenerator);

struct ParticleShooter: public Object{
	virtual void operator()(Vector3r& vel, Vector3r& angVel){ throw std::runtime_error("Calling ParticleShooter.setVelocities (abstract method); use derived classes"); }
	WOO_CLASS_BASE_DOC(ParticleShooter,Object,"Abstract class for assigning initial velocities to generated particles.");
};
REGISTER_SERIALIZABLE(ParticleShooter);

struct AlignedMinMaxShooter: public ParticleShooter{
	void operator()(Vector3r& vel, Vector3r& angVel){
		if(isnan(vRange[0]) || isnan(vRange[0])){ throw std::runtime_error("AlignedMinMaxShooter.vRange must not be NaN."); }
		vel=dir*(vRange[0]+Mathr::UnitRandom()*(vRange[1]-vRange[0]));
		angVel=Vector3r::Zero();
	}
	void postLoad(AlignedMinMaxShooter&){ dir.normalize(); }
	WOO_CLASS_BASE_DOC_ATTRS(AlignedMinMaxShooter,ParticleShooter,"Shoot particles in one direction, with velocity magnitude constrained by vRange values",
		((Vector3r,dir,Vector3r::UnitX(),AttrTrait<Attr::triggerPostLoad>(),"Direction (will be normalized)"))
		((Vector2r,vRange,Vector2r(NaN,NaN),,"Minimum velocity magnitude"))
	);
};
REGISTER_SERIALIZABLE(AlignedMinMaxShooter);

struct Collider;

struct RandomFactory: public ParticleFactory{
	#define WOO_FACTORY_SPHERES_ONLY
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual Vector3r randomPosition(){ throw std::runtime_error("Calling ParticleFactor.randomPosition	(abstract method); use derived classes."); }
	virtual bool validateBox(const AlignedBox3r& b) { throw std::runtime_error("Calling ParticleFactor.validateBox (abstract method); use derived classes."); }
	void run();
	void pyClear(){ if(generator) generator->clear(); num=0; mass=0; stepGoalMass=0; /* do not reset stepPrev! */ }
	shared_ptr<Collider> collider;
	enum{MAXATT_ERROR=0,MAXATT_DEAD,MAXATT_WARN,MAXATT_SILENT};
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(RandomFactory,ParticleFactory,"Factory generating new particles.",
		((Real,massFlowRate,NaN,AttrTrait<>().massFlowRateUnit(),"Mass flow rate; if zero, generate as many particles as possible, until maxAttemps is reached."))
		((vector<shared_ptr<Material>>,materials,,,"Set of materials for new particles, randomly picked from"))
		((shared_ptr<ParticleGenerator>,generator,,,"Particle generator instance"))
		((shared_ptr<ParticleShooter>,shooter,,,"Particle shooter instance (assigns velocities to generated particles. If not given, particles have zero velocities initially."))
		((int,maxAttempts,5000,,"Maximum attempts to position a new particle randomly. If 0, no limit is imposed. When reached, :ref:`atMaxAttempts` determines, what will be done."))
		((int,atMaxAttempts,MAXATT_ERROR,AttrTrait<>().choice({{MAXATT_ERROR,"error"},{MAXATT_DEAD,"dead"},{MAXATT_WARN,"warn"},{MAXATT_SILENT,"silent"}}),"What to do when maxAttempts is reached."))
		((int,mask,1,,"Groupmask for new particles"))
		((Real,color,NaN,,"Color for new particles (NaN for random)"))
		//
		((Real,stepGoalMass,0,AttrTrait<Attr::readonly>(),"Mass to be attained in this step"))
		// ((long,stepPrev,-1,AttrTrait<Attr::readonly>(),"Step in which we were run for the last time"))
		,/*ctor*/
		,/*py*/
			.def("clear",&RandomFactory::pyClear);
			_classObj.attr("maxAttError")=(int)MAXATT_ERROR;
			_classObj.attr("maxAttDead")=(int)MAXATT_DEAD;
			_classObj.attr("maxAttWarn")=(int)MAXATT_WARN;
			_classObj.attr("maxAttSilent")=(int)MAXATT_SILENT;
	);
};
REGISTER_SERIALIZABLE(RandomFactory);

/*
repsect periodic boundaries when this is defined; this makes configurable side of BoxFactory transparent, i.e.
new particles can cross the boundary; note however that this is not properly supported in RandomFactory, where periodicity is not take in account when detecting overlap with particles generated within the same step
*/
	// #define BOX_FACTORY_PERI
struct BoxFactory: public RandomFactory{
	Vector3r randomPosition(){ return box.sample(); }
	#ifdef BOX_FACTORY_PERI
		bool validateBox(const AlignedBox3r& b) { return scene->isPeriodic?box.contains(b):validatePeriodicBox(b); }
		bool validatePeriodicBox(const AlignedBox3r& b) const;
	#else
		bool validateBox(const AlignedBox3r& b) { return box.contains(b); }
	#endif

	#ifdef WOO_OPENGL
		void render(const GLViewInfo&){
			if(isnan(glColor)) return;
			GLUtils::AlignedBox(box,CompUtils::mapColor(glColor));
			std::ostringstream oss; oss.precision(4); oss<<mass;
			if(maxMass>0){ oss<<"/"; oss.precision(4); oss<<maxMass; }
			if(!isnan(currRate)){ oss.precision(3); oss<<"\n("<<currRate<<")"; }
			GLUtils::GLDrawText(oss.str(),box.center(),CompUtils::mapColor(glColor));
		}
	#endif
	WOO_CLASS_BASE_DOC_ATTRS(BoxFactory,RandomFactory,"Generate particle inside axis-aligned box volume.",
		((AlignedBox3r,box,AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),,"Box volume specification (lower and upper corners)"))
		#ifdef BOX_FACTORY_PERI
			((int,periSpanMask,0,,"When running in periodic scene, particles bboxes will be allowed to stick out of the factory in those directions (used to specify that the factory itself should be periodic along those axes). 1=x, 2=y, 4=z."))
		#endif
	);
};
REGISTER_SERIALIZABLE(BoxFactory);

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
	py::tuple pyDiamMass();
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
		,/*ctor*/
		,/*py*/
		.def("psd",&BoxDeleter::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("num")=80,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("zip")=false),"Return particle size distribution of deleted particles (only useful with *save*), spaced between *dRange* (a 2-tuple of minimum and maximum radius); )")
		.def("clear",&BoxDeleter::pyClear,"Clear information about saved particles (particle list, if saved, mass and number)")
		.def("diamMass",&BoxDeleter::pyDiamMass,"Return 2-tuple of same-length list of diameters and masses.")
	);
};
REGISTER_SERIALIZABLE(BoxDeleter);

struct ConveyorFactory: public ParticleFactory{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void sortPacking();
	void postLoad(ConveyorFactory&);
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&){
			if(isnan(glColor)) return;
			std::ostringstream oss; oss.precision(4); oss<<mass;
			if(maxMass>0){ oss<<"/"; oss.precision(4); oss<<maxMass; }
			if(!isnan(currRate)){ oss.precision(3); oss<<"\n("<<currRate<<")"; }
			GLUtils::GLDrawText(oss.str(),node->pos,CompUtils::mapColor(glColor));
		}
	#endif
	void run();
	vector<shared_ptr<Particle>> pyBarrier() const { return vector<shared_ptr<Particle>>(barrier.begin(),barrier.end()); }
	void pyClear(){ mass=0; num=0; genDiamMass.clear(); }
	py::object pyDiamMass() const;
	py::tuple pyPsd(bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) const;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ConveyorFactory,ParticleFactory,"Factory producing infinite band of particles from packing periodic in the x-direction. (Clumps are not supported (yet?), only spheres).",
		((shared_ptr<Material>,material,,,"Material for new particles"))
		((Real,cellLen,,,"Length of the band cell, which is repeated periodically"))
		((vector<Real>,radii,,AttrTrait<>().noGui().triggerPostLoad(),"Radii for the packing"))
		((vector<Vector3r>,centers,,AttrTrait<>().noGui().triggerPostLoad(),"Centers of spheres in the packing"))
		((int,nextIx,-1,AttrTrait<>().readonly(),"Index of last-generated particles in the packing"))
		((Real,lastX,0,AttrTrait<>().readonly(),"X-coordinate of last-generated particles in the packing"))
		((Real,vel,NaN,,"Velocity of the feed"))
		((Real,prescribedRate,NaN,AttrTrait<>().triggerPostLoad(),"Prescribed average mass flow rate; if given, overwrites *vel*"))
		((int,mask,1,,"Mask for new particles"))
		((Real,startLen,NaN,,"Band to be created at the very start; if NaN, only the usual starting amount is created (depending on feed velocity)"))
		((Real,barrierColor,.2,,"Color for barrier particles (NaN for random)"))
		((Real,color,NaN,,"Color for non-barrier particles (NaN for random)"))
		((Real,barrierLayer,-3.,,"Some length of the last part of new particles has all DoFs blocked, so that when new particles are created, there are no sudden contacts in the band; in the next step, DoFs in this layer are unblocked. If *barrierLayer* is negative, it is relative to the maximum radius in the given packing, and is automatically set to the correct value at the first run"))
		((list<shared_ptr<Particle>>,barrier,,AttrTrait<>().readonly().noGui(),"Particles which make up the barrier and will be unblocked once they leave barrierLayer."))
		((shared_ptr<Node>,node,make_shared<Node>(),AttrTrait<>().readonly(),"Position and orientation of the factory; local x-axis is the feed direction."))
		((Real,avgRate,NaN,AttrTrait<>().readonly(),"Average feed rate (computed from :ref:`material` density, packing and  and :ref:`vel`"))

		((vector<Vector2r>,genDiamMass,,AttrTrait<Attr::readonly>().noGui(),"List of generated diameters and masses (for making granulometry)"))
		((bool,save,true,,"Save generated particles so that PSD can be generated afterwards"))
		,/*ctor*/
		,/*py*/
			.def("barrier",&ConveyorFactory::pyBarrier)
			.def("clear",&ConveyorFactory::pyClear)
			.def("diamMass",&ConveyorFactory::pyDiamMass,"Return 2-tuple of same-length list of diameters and masses.")
			.def("psd",&ConveyorFactory::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("num")=80),"Return PSD for particles generated.")
	);
};
REGISTER_SERIALIZABLE(ConveyorFactory);
