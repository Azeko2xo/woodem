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
		((bool,zeroRateAtStop,true,,"When the generator stops (mass/number of particles reached, ...), set :ret:`currRate` to zero immediately"))
		((Real,currRateSmooth,1,AttrTrait<>().noGui().range(Vector2r(0,1)),"Smoothing factor for currRate ∈〈0,1〉"))
		((Real,glColor,0,AttrTrait<>().noGui(),"Color for rendering (nan disables rendering)"))
	);
};
WOO_REGISTER_OBJECT(ParticleFactory);

struct ParticleGenerator: public Object{
	// particle and two extents sizes (bbox if p is at origin)
	struct ParticleAndBox{ shared_ptr<Particle> par; AlignedBox3r extents; };
	// return (one or multiple, for clump) particles and extents (min and max)
	// extents are computed for position of (0,0,0)
	virtual vector<ParticleAndBox> operator()(const shared_ptr<Material>& m){ throw std::runtime_error("Calling ParticleGenerator.operator() (abstract method); use derived classes."); }
	// called when the particle placement failed; the generator must revoke it and update its bookkeeping information (e.g. PSD, generated radii and diameters etc)
	virtual void revokeLast(){ if(save && !genDiamMass.empty()) genDiamMass.resize(genDiamMass.size()-1); }
	virtual void clear(){ genDiamMass.clear(); }
	py::tuple pyPsd(bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) const;
	py::object pyDiamMass() const;
	Real pyMassOfDiam(Real min, Real max) const;
	py::list pyCall(const shared_ptr<Material>& m){ vector<ParticleAndBox> pee=(*this)(m); py::list ret; for(const auto& pe: pee) ret.append(py::make_tuple(pe.par,pe.extents)); return ret; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ParticleGenerator,Object,"Abstract class for generating particles",
		((vector<Vector2r>,genDiamMass,,AttrTrait<Attr::readonly>().noGui(),"List of generated particle's (equivalent) radii and masses (for making granulometry)"))
		((bool,save,true,,"Save generated particles so that PSD can be generated afterwards"))
		,/*ctor*/
		,/*py*/
			.def("psd",&ParticleGenerator::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("num")=80),"Return PSD for particles generated.")
			.def("diamMass",&ParticleGenerator::pyDiamMass,"Return tuple of 2 arrays, diameters and masses.")
			.def("massOfDiam",&ParticleGenerator::pyMassOfDiam,(py::arg("min")=0,py::arg("max")=Inf),"Return mass of particles of which diameters are between *min* and *max*.")
			.def("clear",&ParticleGenerator::clear,"Clear stored data about generated particles; only subsequently generated particles will be considered in the PSD.")
			.def("__call__",&ParticleGenerator::pyCall,"Call the generation routine, returning one particle (at origin) and its bounding-box when at origin. Useful for debugging.")
	);
};
WOO_REGISTER_OBJECT(ParticleGenerator);

struct MinMaxSphereGenerator: public ParticleGenerator{
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m);
	WOO_CLASS_BASE_DOC_ATTRS(MinMaxSphereGenerator,ParticleGenerator,"Generate particles with given minimum and maximum radius",
		((Vector2r,dRange,Vector2r(NaN,NaN),,"Minimum and maximum radius of generated spheres"))
	);
};
WOO_REGISTER_OBJECT(MinMaxSphereGenerator);

struct ParticleShooter: public Object{
	virtual void operator()(Vector3r& vel, Vector3r& angVel){ throw std::runtime_error("Calling ParticleShooter.setVelocities (abstract method); use derived classes"); }
	WOO_CLASS_BASE_DOC(ParticleShooter,Object,"Abstract class for assigning initial velocities to generated particles.");
};
WOO_REGISTER_OBJECT(ParticleShooter);

struct AlignedMinMaxShooter: public ParticleShooter{
	void operator()(Vector3r& vel, Vector3r& angVel){
		if(isnan(vRange[0]) || isnan(vRange[0])){ throw std::runtime_error("AlignedMinMaxShooter.vRange must not be NaN."); }
		vel=dir*(vRange[0]+Mathr::UnitRandom()*(vRange[1]-vRange[0]));
		angVel=Vector3r::Zero();
	}
	void postLoad(AlignedMinMaxShooter&, void*){ dir.normalize(); }
	WOO_CLASS_BASE_DOC_ATTRS(AlignedMinMaxShooter,ParticleShooter,"Shoot particles in one direction, with velocity magnitude constrained by vRange values",
		((Vector3r,dir,Vector3r::UnitX(),AttrTrait<Attr::triggerPostLoad>(),"Direction (will be normalized)"))
		((Vector2r,vRange,Vector2r(NaN,NaN),,"Minimum velocity magnitude"))
	);
};
WOO_REGISTER_OBJECT(AlignedMinMaxShooter);

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
		((int,maxAttempts,5000,,"Maximum attempts to position new particles randomly, without collision. If 0, no limit is imposed. When reached, :ref:`atMaxAttempts` determines, what will be done. Each particle is tried maxAttempts/maxMetaAttempts times, then a new particle is tried."))
		((int,attemptPar,5,,"Number of trying a different particle to position (each will be tried maxAttempts/attemptPar times)"))
		((int,atMaxAttempts,MAXATT_ERROR,AttrTrait<>().choice({{MAXATT_ERROR,"error"},{MAXATT_DEAD,"dead"},{MAXATT_WARN,"warn"},{MAXATT_SILENT,"silent"}}),"What to do when maxAttempts is reached."))
		((int,mask,1,,"Groupmask for new particles"))
		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene.energy"))
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
WOO_REGISTER_OBJECT(RandomFactory);

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
WOO_REGISTER_OBJECT(BoxFactory);

