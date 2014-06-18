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
	// return true when maximum mass/num of particles (or other termination condition) has been reached
	// runs doneHook inside and sets dead=True
	bool everythingDone();
	#define woo_dem_ParticleFactory__CLASS_BASE_DOC_ATTRS \
		ParticleFactory,PeriodicEngine,ClassTrait().doc("Factory generating new particles. This is an abstract base class which in itself does not generate anything, but provides some unified interface to derived classes.").section("Factories","TODO",{"ParticleGenerator","ParticleShooter","BoxDeleter"}), \
		((int,mask,((void)":obj:`DemField.defaultFactoryMask`",DemField::defaultFactoryMask),,":obj:`~woo.dem.Particle.mask` for new particles.")) \
		((Real,maxMass,-1,,"Mass at which the engine will not produce any particles (inactive if not positive)")) \
		((long,maxNum,-1,,"Number of generated particles after which no more will be produced (inactive if not positive)")) \
		((string,doneHook,"",,"Python string to be evaluated when :obj:`maxMass` or :obj:`maxNum` have been reached. The engine is made dead automatically even if doneHook is not specified.")) \
		((Real,mass,0,,"Generated mass total")) \
		((long,num,0,,"Number of generated particles")) \
		((Real,currRate,NaN,AttrTrait<>().readonly(),"Current value of mass flow rate")) \
		((bool,zeroRateAtStop,true,,"When the generator stops (mass/number of particles reached, ...), set :obj:`currRate` to zero immediately")) \
		((Real,currRateSmooth,1,AttrTrait<>().range(Vector2r(0,1)),"Smoothing factor for currRate ∈〈0,1〉")) \
		((Real,glColor,0,AttrTrait<>().noGui(),"Color for rendering (nan disables rendering)"))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_ParticleFactory__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(ParticleFactory);

struct ParticleGenerator: public Object{
	// particle and two extents sizes (bbox if p is at origin)
	struct ParticleAndBox{ shared_ptr<Particle> par; AlignedBox3r extents; };
	// return (one or multiple, for clump) particles and extents (min and max)
	// extents are computed for position of (0,0,0)
	virtual vector<ParticleAndBox> operator()(const shared_ptr<Material>& m){ throw std::runtime_error("Calling ParticleGenerator.operator() (abstract method); use derived classes."); }
	virtual Real critDt(Real density, Real young) { return Inf; }
	// called when the particle placement failed; the generator must revoke it and update its bookkeeping information (e.g. PSD, generated radii and diameters etc)
	virtual void revokeLast(){ if(save && !genDiamMass.empty()) genDiamMass.resize(genDiamMass.size()-1); }
	virtual void clear(){ genDiamMass.clear(); }
	// spheres-only generators override this to enable some optimizations
	virtual bool isSpheresOnly() const { throw std::logic_error(pyStr()+" should override ParticleGenerator.isSpheresOnly."); }
	virtual Real padDist() const {  throw std::logic_error(pyStr()+" should override ParticleGenerator.padDist."); }
	virtual Vector2r minMaxDiam() const { throw std::logic_error(pyStr()+" should override ParticleGenerator.minMaxDiam."); }
	py::tuple pyPsd(bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) const;
	py::object pyDiamMass(bool zipped=false) const;
	Real pyMassOfDiam(Real min, Real max) const;
	py::list pyCall(const shared_ptr<Material>& m){ vector<ParticleAndBox> pee=(*this)(m); py::list ret; for(const auto& pe: pee) ret.append(py::make_tuple(pe.par,pe.extents)); return ret; }
	#define woo_dem_ParticleGenerator__CLASS_BASE_DOC_ATTRS_PY \
		ParticleGenerator,Object,"Abstract class for generating particles", \
		((vector<Vector2r>,genDiamMass,,AttrTrait<Attr::readonly>().noGui().noDump(),"List of generated particle's (equivalent) radii and masses (for making granulometry)")) \
		((bool,save,true,,"Save generated particles so that PSD can be generated afterwards")) \
		,/*py*/ \
			.def("psd",&ParticleGenerator::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=true,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("num")=80),"Return PSD for particles generated.") \
			.def("diamMass",&ParticleGenerator::pyDiamMass,(py::arg("zipped")=false),"With *zipped*, return list of (diameter, mass); without *zipped*, return tuple of 2 arrays, diameters and masses.") \
			.def("massOfDiam",&ParticleGenerator::pyMassOfDiam,(py::arg("min")=0,py::arg("max")=Inf),"Return mass of particles of which diameters are between *min* and *max*.") \
			.def("clear",&ParticleGenerator::clear,"Clear stored data about generated particles; only subsequently generated particles will be considered in the PSD.") \
			.def("critDt",&ParticleGenerator::critDt,(py::arg("density"),py::arg("young")),"Return critical timestep for particles generated by us, given that their density and Young's modulus are as given in arguments.") \
			.def("padDist",&ParticleGenerator::padDist,"Return padding distance by which the factory geometry should be shrunk before generating a random point.") \
			.def("__call__",&ParticleGenerator::pyCall,"Call the generation routine, returning one particle (at origin) and its bounding-box when at origin. Useful for debugging.") \
			.def("minMaxDiam",&ParticleGenerator::minMaxDiam,"Return the range of minimum and maximum diameters.")
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ParticleGenerator__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(ParticleGenerator);

struct MinMaxSphereGenerator: public ParticleGenerator{
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m);
	Real critDt(Real density, Real young) WOO_CXX11_OVERRIDE; 
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return true; }
	Real padDist() const WOO_CXX11_OVERRIDE{ return dRange.minCoeff()/2.; }
	Vector2r minMaxDiam() const WOO_CXX11_OVERRIDE { return dRange; }
	#define woo_dem_MinMaxSphereGenerator_CLASS_BASE_DOC_ATTRS \
		MinMaxSphereGenerator,ParticleGenerator,"Generate particles with given minimum and maximum radius", \
		((Vector2r,dRange,Vector2r(NaN,NaN),,"Minimum and maximum radius of generated spheres")) 
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_MinMaxSphereGenerator_CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(MinMaxSphereGenerator);

struct ParticleShooter: public Object{
	virtual void operator()(Vector3r& vel, Vector3r& angVel){ throw std::runtime_error("Calling ParticleShooter.setVelocities (abstract method); use derived classes"); }
	#define woo_dem_ParticleShooter__CLASS_BASE_DOC \
		ParticleShooter,Object,"Abstract class for assigning initial velocities to generated particles."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_ParticleShooter__CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(ParticleShooter);

struct AlignedMinMaxShooter: public ParticleShooter{
	void operator()(Vector3r& vel, Vector3r& angVel){
		if(isnan(vRange.maxCoeff())){ throw std::runtime_error("AlignedMinMaxShooter.vRange: must not contain NaN."); }
		vel=dir*(vRange[0]+Mathr::UnitRandom()*(vRange[1]-vRange[0]));
		angVel=Vector3r::Zero();
	}
	void postLoad(AlignedMinMaxShooter&, void*){ dir.normalize(); }
	#define woo_dem_AlignedMinMaxShooter__CLASS_BASE_DOC_ATTRS \
		AlignedMinMaxShooter,ParticleShooter,"Shoot particles in one direction, with velocity magnitude constrained by vRange values", \
		((Vector3r,dir,Vector3r::UnitX(),AttrTrait<Attr::triggerPostLoad>(),"Direction (will be normalized).")) \
		((Vector2r,vRange,Vector2r(NaN,NaN),,"Minimum and maximum velocities."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_AlignedMinMaxShooter__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(AlignedMinMaxShooter);

struct Collider;

struct RandomFactory: public ParticleFactory{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual Vector3r randomPosition(const Real& padDist){ throw std::runtime_error("Calling ParticleFactor.randomPosition	(abstract method); use derived classes."); }
	virtual bool validateBox(const AlignedBox3r& b) { throw std::runtime_error("Calling ParticleFactor.validateBox (abstract method); use derived classes."); }
	void run();
	void pyClear(){ if(generator) generator->clear(); num=0; mass=0; stepGoalMass=0; /* do not reset stepPrev! */ }
	Real critDt() WOO_CXX11_OVERRIDE; 
	shared_ptr<Collider> collider;
	enum{MAXATT_ERROR=0,MAXATT_DEAD,MAXATT_WARN,MAXATT_SILENT};
	bool spheresOnly; // set at each step, queried from the generator
	#define woo_dem_RandomFactory__CLASS_BASE_DOC_ATTRS_PY \
		RandomFactory,ParticleFactory,"Factory generating new particles. This class overrides :obj:`woo.core.Engine.critDt`, which in turn calls :obj:`woo.dem.ParticleGenerator.critDt` with all possible :obj:`materials` one by one.", \
		((Real,massRate,NaN,AttrTrait<>().massRateUnit(),"Mass flow rate; if zero, generate as many particles as possible, until maxAttemps is reached.")) \
		((vector<shared_ptr<Material>>,materials,,,"Set of materials for new particles, randomly picked from")) \
		((shared_ptr<ParticleGenerator>,generator,,,"Particle generator instance")) \
		((shared_ptr<ParticleShooter>,shooter,,,"Particle shooter instance (assigns velocities to generated particles. If not given, particles have zero velocities initially.")) \
		((int,maxAttempts,5000,,"Maximum attempts to position new particles randomly, without collision. If 0, no limit is imposed. When reached, :obj:`atMaxAttempts` determines, what will be done. Each particle is tried maxAttempts/maxMetaAttempts times, then a new particle is tried.")) \
		((int,attemptPar,5,,"Number of trying a different particle to position (each will be tried maxAttempts/attemptPar times)")) \
		((int,atMaxAttempts,MAXATT_ERROR,AttrTrait<>().choice({{MAXATT_ERROR,"error"},{MAXATT_DEAD,"dead"},{MAXATT_WARN,"warn"},{MAXATT_SILENT,"silent"}}),"What to do when maxAttempts is reached.")) \
		((Real,padDist,0.,AttrTrait<Attr::readonly>(),"Pad geometry by this distance inside; random points will be chosen inside the shrunk geometry, whereas boxes will be validated in the larger one. This attribute must be set by the generator.")) \
		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene.energy")) \
		((Real,color,NaN,,"Color for new particles (NaN for random; negative for keeping color assigned by the generator).")) \
		((Real,stepGoalMass,0,AttrTrait<Attr::readonly>(),"Mass to be attained in this step")) \
		((bool,collideExisting,true,,"Consider collisions with pre-existing particle; this is generally a good idea, though if e.g. there are no pre-existing particles, it is useful to set to ``False`` to avoid having to define collider for no other reason than make :obj:`RandomFactory` happy.")) \
		,/*py*/ \
			.def("clear",&RandomFactory::pyClear) \
			.def("randomPosition",&RandomFactory::randomPosition) \
			.def("validateBox",&RandomFactory::validateBox) \
			; \
			_classObj.attr("maxAttError")=(int)MAXATT_ERROR; \
			_classObj.attr("maxAttDead")=(int)MAXATT_DEAD; \
			_classObj.attr("maxAttWarn")=(int)MAXATT_WARN; \
			_classObj.attr("maxAttSilent")=(int)MAXATT_SILENT;
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_RandomFactory__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(RandomFactory);

/*
repsect periodic boundaries when this is defined; this makes configurable side of BoxFactory transparent, i.e.
new particles can cross the boundary; note however that this is not properly supported in RandomFactory, where periodicity is not take in account when detecting overlap with particles generated within the same step
*/
	// #define BOX_FACTORY_PERI
struct BoxFactory: public RandomFactory{
	Vector3r randomPosition(const Real& padDist) WOO_CXX11_OVERRIDE;
	#ifdef BOX_FACTORY_PERI
		bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE { return scene->isPeriodic?box.contains(b):validatePeriodicBox(b); }
		bool validatePeriodicBox(const AlignedBox3r& b) const;
	#else
		bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE { return box.contains(b); }
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

	#ifdef BOX_FACTORY_PERI
		#define woo_dem_BoxFactory__periSpanMask__BOX_FACTORY_PERI ((int,periSpanMask,0,,"When running in periodic scene, particles bboxes will be allowed to stick out of the factory in those directions (used to specify that the factory itself should be periodic along those axes). 1=x, 2=y, 4=z."))
	#else
		#define woo_dem_BoxFactory__periSpanMask__BOX_FACTORY_PERI		
	#endif

	#define woo_dem_BoxFactory__CLASS_BASE_DOC_ATTRS \
		BoxFactory,RandomFactory,"Generate particle inside axis-aligned box volume.", \
		((AlignedBox3r,box,AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),,"Box volume specification (lower and upper corners)")) \
		woo_dem_BoxFactory__periSpanMask__BOX_FACTORY_PERI 
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxFactory__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(BoxFactory);

struct CylinderFactory: public RandomFactory{
	Vector3r randomPosition(const Real& padDist) WOO_CXX11_OVERRIDE; /* http://stackoverflow.com/questions/5837572/generate-a-random-point-within-a-circle-uniformly */
	bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE; /* check all corners are inside the cylinder */
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&);
	#endif

	#define woo_dem_CylinderFactory__CLASS_BASE_DOC_ATTRS \
		CylinderFactory,RandomFactory,"Generate particle inside an arbitrarily oriented cylinder.", \
		((shared_ptr<Node>,node,,,"Node defining local coordinate system. If not given, global coordinates are used.")) \
		((Real,height,NaN,AttrTrait<>().lenUnit(),"Height along the local :math:`x`-axis.")) \
		((Real,radius,NaN,AttrTrait<>().lenUnit(),"Radius of the cylinder (perpendicular to the local :math:`x`-axis).")) \
		((int,glSlices,16,,"Number of subdivision slices for rendering."))

		// ((vector<AlignedBox3r>,boxesTried,,AttrTrait<>().noGui(),"Bounding boxes being attempted to validate -- debugging only."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_CylinderFactory__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(CylinderFactory);

struct BoxFactory2d: public BoxFactory{
	Vector2r flatten(const Vector3r& v){ return Vector2r(v[(axis+1)%3],v[(axis+2)%3]); }
	bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE { return AlignedBox2r(flatten(box.min()),flatten(box.max())).contains(AlignedBox2r(flatten(b.min()),flatten(b.max()))); }
	#define woo_dem_BoxFactory2d__CLASS_BASE_DOC_ATTRS \
		BoxFactory2d,BoxFactory,"Generate particles inside axis-aligned plane (its normal axis is given via the :obj:`axis` attribute; particles are allowed to stick out of that plane.", \
		((short,axis,2,,"Axis normal to the plane in which particles are generated.")) 
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxFactory2d__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(BoxFactory2d);

