#include<yade/pkg/dem/Particle.hpp>
#include<boost/range/numeric.hpp>
#include<boost/range/algorithm/fill.hpp>


#ifdef YADE_OPENGL
	#include<yade/lib/opengl/GLUtils.hpp>
	#include<yade/lib/base/CompUtils.hpp>
#endif

struct ParticleGenerator: public Serializable{
	// particle and two extents sizes (bbox if p is at origin)
	struct ParticleExtExt{ shared_ptr<Particle> par; AlignedBox3r extents; };
	// return (one or multiple, for clump) particles and extents (min and max)
	// extents are computed for position of (0,0,0)
	virtual vector<ParticleExtExt> operator()(const shared_ptr<Material>& m){ throw std::runtime_error("Calling ParticleGenerator.operator() (abstract method); use derived classes."); }
	YADE_CLASS_BASE_DOC(ParticleGenerator,Serializable,"Abstract class for generating particles");
};
REGISTER_SERIALIZABLE(ParticleGenerator);

struct MinMaxSphereGenerator: public ParticleGenerator{
	vector<ParticleExtExt> operator()(const shared_ptr<Material>&m);
	YADE_CLASS_BASE_DOC_ATTRS(MinMaxSphereGenerator,ParticleGenerator,"Generate particles with given minimum and maximum radius",
		((Vector2r,rRange,Vector2r(NaN,NaN),,"Minimum and maximum radius of generated spheres"))
	);
};
REGISTER_SERIALIZABLE(MinMaxSphereGenerator);

struct PsdSphereGenerator: public ParticleGenerator{
	DECLARE_LOGGER;
	vector<ParticleExtExt> operator()(const shared_ptr<Material>&m);
	void postLoad(PsdSphereGenerator&);
	py::tuple pyPsd() const;
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(PsdSphereGenerator,ParticleGenerator,"Generate particles following a discrete Particle Size Distribution (PSD)",
		((vector<Vector2r>,psdPts,,,"Points of the PSD curve; the first component is particle diameter [m] (not radius!), the second component is passing percentage. Both diameter and passing values must be increasing (diameters must be strictly increasing). Passing values are normalized so that the last value is 1.0 (therefore, you can enter the values in percents if you like)"))
		((vector<int>,numPerBin,,Attr::noGui,"Keep track of how much particles were generated for each point on the PSD so that we get as close to the curve as possible."))
		((int,numTot,,Attr::noGui,"Total number of particles generated"))
		, /* ctor */
		, /* py */
			.def("psd",&PsdSphereGenerator::pyPsd,"Return points of the PSD suitable for plotting")
	);
};
REGISTER_SERIALIZABLE(PsdSphereGenerator);

struct ParticleShooter: public Serializable{
	virtual void operator()(Vector3r& vel, Vector3r& angVel){ throw std::runtime_error("Calling ParticleShooter.setVelocities (abstract method); use derived classes"); }
	YADE_CLASS_BASE_DOC(ParticleShooter,Serializable,"Abstract class for assigning initial velocities to generated particles.");
};
REGISTER_SERIALIZABLE(ParticleShooter);

struct AlignedMinMaxShooter: public ParticleShooter{
	void operator()(Vector3r& vel, Vector3r& angVel){
		if(isnan(vRange[0]) || isnan(vRange[0])){ throw std::runtime_error("AlignedMinMaxShooter.vRange must not be NaN."); }
		vel=dir*(vRange[0]+Mathr::UnitRandom()*(vRange[1]-vRange[0]));
		angVel=Vector3r::Zero();
	}
	void postLoad(AlignedMinMaxShooter&){ dir.normalize(); }
	YADE_CLASS_BASE_DOC_ATTRS(AlignedMinMaxShooter,ParticleShooter,"Shoot particles in one direction, with velocity magnitude constrained by vRange values",
		((Vector3r,dir,Vector3r::UnitX(),Attr::triggerPostLoad,"Direction (will be normalized)"))
		((Vector2r,vRange,Vector2r(NaN,NaN),,"Minimum velocity magnitude"))
	);
};
REGISTER_SERIALIZABLE(AlignedMinMaxShooter);

struct Collider;

struct ParticleFactory: public PeriodicEngine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual Vector3r randomPosition(){ throw std::runtime_error("Calling ParticleFactor.randomPosition	(abstract method); use derived classes."); }
	virtual bool validateBox(const AlignedBox3r& b) { throw std::runtime_error("Calling ParticleFactor.validateBox (abstract method); use derived classes."); }
	void run();
	shared_ptr<Collider> collider;
	YADE_CLASS_BASE_DOC_ATTRS(ParticleFactory,PeriodicEngine,"Factory generating new particles.",
		((Real,massFlowRate,NaN,,"Mass flow rate [kg/s]"))
		((Real,maxMass,-1,,"Mass at which the engine will not produce any particles (inactive if negative)"))
		((long,maxNum,-1,,"Number of generated particles after which no more will be produced (inacitve if negative)"))
		((Real,mass,0,,"Generated mass total"))
		((long,num,0,,"Number of generated particles"))
		((vector<shared_ptr<Material>>,materials,,,"Set of materials for new particles, randomly picked from"))
		((shared_ptr<ParticleGenerator>,generator,,,"Particle generator instance"))
		((shared_ptr<ParticleShooter>,shooter,,,"Particle shooter instance (assigns velocities to generated particles"))
		((int,maxAttempts,5000,,"Maximum attempts to position a new particle randomly. If 0, no limit is imposed. If negative, the engine will be made *dead* after reaching the number, but without raising an exception."))
		((int,mask,1,,"Groupmask for new particles"))
		((Real,color,NaN,,"Color for new particles (NaN for random)"))
		//
		((Real,stepMass,0,Attr::readonly,"Mass to be attained in this step"))
		((long,stepPrev,-1,Attr::readonly,"Step in which we were run for the last time"))
	);
};
REGISTER_SERIALIZABLE(ParticleFactory);

struct BoxFactory: public ParticleFactory{
	Vector3r randomPosition(){ return box.sample(); }
	bool validateBox(const AlignedBox3r& b) { return box.contains(b); }
	#ifdef YADE_OPENGL
		void render(const GLViewInfo&){ if(!isnan(color)) GLUtils::AlignedBox(box,CompUtils::mapColor(color)); }
	#endif
	YADE_CLASS_BASE_DOC_ATTRS(BoxFactory,ParticleFactory,"Generate particle inside axis-aligned box volume.",
		((AlignedBox3r,box,AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),,"Box volume specification (lower and upper corners)"))
		((Real,color,0,Attr::noGui,"Color for rendering (nan disables rendering)"))
	);
};
REGISTER_SERIALIZABLE(BoxFactory);

struct BoxDeleter: public PeriodicEngine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	#ifdef YADE_OPENGL
		void render(const GLViewInfo&){ if(!isnan(color)) GLUtils::AlignedBox(box,CompUtils::mapColor(color)); }
	#endif
	void run();
	py::object pyPsd(int num, const Vector2r& rRange, bool zip);
	void pyClear(){ deleted.clear(); mass=0.; num=0; }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(BoxDeleter,PeriodicEngine,"Delete particles which fall outside (or inside, if *inside* is True) given box. Deleted particles are optionally stored in the *deleted* array for later processing, if needed.",
		((AlignedBox3r,box,AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),,"Box volume specification (lower and upper corners)"))
		((int,mask,0,,"If non-zero, only particles matching the mask will be candidates for removal"))
		((bool,inside,false,,"Delete particles which fall inside the volume rather than outside"))
		((bool,save,false,,"Save particles which are deleted in the *deleted* list"))
		((vector<shared_ptr<Particle>>,deleted,,(Attr::noGui|Attr::readonly),"Deleted particle's list; can be cleared with BoxDeleter.clear()"))
		((int,num,0,Attr::readonly,"Number of deleted particles"))
		((Real,mass,0.,Attr::readonly,"Total mass of deleted particles"))
		((Real,color,0,Attr::noGui,"Color for rendering (nan disables rendering)"))
		,/*ctor*/
		,/*py*/
		.def("psd",&BoxDeleter::pyPsd,(py::arg("num")=80,py::arg("rRange")=Vector2r(NaN,NaN),py::arg("zip")=false),"Return particle size distribution of deleted particles (only useful with *save*), spaced between *rRange* (a 2-tuple of minimum and maximum radius); )")
		.def("clear",&BoxDeleter::pyClear,"Clear information about saved particles (particle list, if saved, mass and number)")
	);
};
REGISTER_SERIALIZABLE(BoxDeleter);

#if 0
struct MultiBoxDeleter: public PeriodicEngine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void postLoad(MultiBoxDeleter&){
		colors.resize(boxes.size());
		masses.resize(boxes.size());
		nums.resize(boxes.size());
		deleted.resize(boxes.size());
	}
	int pyNum(){ return boost::accumulate(nums,0); }
	int pyMass(){ return boost::accumulate(masses,0.); }
	void pyClear(){ boost::fill(masses,0.); boost::fill(nums,0); for(auto& d:deleted) c.clear(); }
	#ifdef YADE_OPENGL
		void render(const GLViewInfo&){ for(int i=0;i<colors.size();i++) if(!isnan(color)) GLUtils::AlignedBox(box,CompUtils::mapColor(color)); }
	#endif
	py::object pyPsd(int boxNo, int num);
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(MultiBoxDeleter,PeriodicEngine,"Delete particles which fall inside one of the many boxes. The advantage of this engine over multiple BoxDeleters is easier setup and the possibility of reporting individual or combined PSD's",
		((bool,save,false,"Save particles in the *deleted* list."))
		((vector<vector<shared_ptr<Particle>>,deleted,,(Attr::hidden),"deleted particle's list; can be cleared with MultiBoxDeleter.clear()"))
		((vector<int>,nums,,Attr::readonly,"Number of deleted particles for each contained box"))
		((vector<Real>,masses,,Attr::readonly,"Mass of deleted particles for each box"))
		((vector<Real>,colors,,Attr::readonly,"Color for each box"))
		,/*ctor*/
		,/*py*/
		.add_property("mass",&MultiBoxDeleter::pyMass)
		.add_property("num",&MultiBoxDeleter::pyNum)
		.def("clear",&BoxDeleter::pyClear,"Clear information about saved particles")
		.def("psd",&BoxDeleter::pyPsd,(py::arg("boxNo")=-1,py::arg("num")=80),"Return particle size distribution of deleted particles. When no box number is provided, all boxes combined are returned")
	)
}
#endif
