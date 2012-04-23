#include<yade/pkg/dem/Particle.hpp>

struct ParticleGenerator: public Serializable{
	// particle and two extents sizes (bbox if p is at origin)
	struct ParticleExtExt{ shared_ptr<Particle> par; Vector3r extMin; Vector3r extMax; };
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
	YADE_CLASS_BASE_DOC_ATTRS(PsdSphereGenerator,ParticleGenerator,"Generate particles following a discrete Particle Size Distribution (PSD)",
		((vector<Vector2r>,psd,,,"Points of the PSD curve; the first component is particle diameter [m] (not radius!), the second component is passing percentage. Both diameter and passing values must be increasing (diameters must be strictly increasing). Passing values are normalized so that the last value is 1.0 (therefore, you can enter the values in percents if you like)"))
		((vector<int>,numPerBin,,Attr::noGui,"Keep track of how much particles were generated for each point on the PSD so that we get as close to the curve as possible."))
		((int,numTot,,Attr::noGui,"Total number of particles generated"))
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
	void run();
	shared_ptr<Collider> collider;
	YADE_CLASS_BASE_DOC_ATTRS(ParticleFactory,PeriodicEngine,"Factory generating new particles.",
		((Real,massFlowRate,NaN,,"Mass flow rate [kg/s]"))
		((Real,maxMass,-1,,"Mass at which the engine will not produce any particles (inactive if negative)"))
		((long,maxNum,-1,,"Number of generated particles after which no more will be produced (inacitve if negative)"))
		((Real,totalMass,0,,"Mass generated so far"))
		((long,totalNum,0,,"Number of particles generated so far"))
		((vector<shared_ptr<Material>>,materials,,,"Set of materials for new particles, randomly picked from"))
		((shared_ptr<ParticleGenerator>,generator,,,"Particle generator instance"))
		((shared_ptr<ParticleShooter>,shooter,,,"Particle shooter instance (assigns velocities to generated particles"))
		((int,maxAttempts,-5000,,"Maximum attempts to position a new particle randomly. If 0, no limit is imposed. If negative, the engine will be made *dead* after reaching the number, but without raising an exception."))
		((int,mask,1,,"Groupmask for new particles"))
		((Real,color,NaN,,"Color for new particles (NaN for random)"))
		//
		((Real,goalMass,0,Attr::readonly,"Mass to be attained in this step"))
		((long,stepPrev,-1,Attr::readonly,"Step in which we were run for the last time"))
	);
};
REGISTER_SERIALIZABLE(ParticleFactory);

struct BoxFactory: public ParticleFactory{
	Vector3r randomPosition(){
		Vector3r ret;
		for(int i=0; i<3; i++) ret[i]=min[i]+Mathr::UnitRandom()*(max[i]-min[i]);
		return ret;
	};
	YADE_CLASS_BASE_DOC_ATTRS(BoxFactory,ParticleFactory,"Generate particle inside axis-aligned box volume.",
		((Vector3r,min,Vector3r(NaN,NaN,NaN),,"Lower corner of the box"))
		((Vector3r,max,Vector3r(NaN,NaN,NaN),,"Upper corner of the box"))
	);
};
REGISTER_SERIALIZABLE(BoxFactory);
