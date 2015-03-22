#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<boost/range/numeric.hpp>
#include<boost/range/algorithm/fill.hpp>


#ifdef WOO_OPENGL
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<woo/lib/base/CompUtils.hpp>
#endif

struct Inlet: public PeriodicEngine{
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
	#ifdef WOO_OPENGL
		void renderMassAndRate(const Vector3r& pos);
	#endif
	#define woo_dem_Inlet__CLASS_BASE_DOC_ATTRS \
		Inlet,PeriodicEngine,ClassTrait().doc("Inlet generating new particles. This is an abstract base class which in itself does not generate anything, but provides some unified interface to derived classes.").section("Inlets & Outlets","TODO",{"ParticleGenerator","SpatialBias","ParticleShooter","Outlet"}), \
		((int,mask,((void)":obj:`DemField.defaultInletMask`",DemField::defaultInletMask),,":obj:`~woo.dem.Particle.mask` for new particles.")) \
		((Real,maxMass,-1,,"Mass at which the engine will not produce any particles (inactive if not positive)")) \
		((long,maxNum,-1,,"Number of generated particles after which no more will be produced (inactive if not positive)")) \
		((string,doneHook,"",,"Python string to be evaluated when :obj:`maxMass` or :obj:`maxNum` have been reached. The engine is made dead automatically even if doneHook is not specified.")) \
		((Real,mass,0,,"Generated mass total")) \
		((long,num,0,,"Number of generated particles")) \
		((Real,currRate,NaN,AttrTrait<>().readonly(),"Current value of mass flow rate")) \
		((bool,zeroRateAtStop,true,,"When the generator stops (mass/number of particles reached, ...), set :obj:`currRate` to zero immediately")) \
		((Real,currRateSmooth,1,AttrTrait<>().range(Vector2r(0,1)),"Smoothing factor for currRate ∈〈0,1〉")) \
		((Real,glColor,0,AttrTrait<>().noGui(),"Color for rendering (nan disables rendering)"))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Inlet__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(Inlet);

struct ParticleGenerator: public Object{
	// particle and two extents sizes (bbox if p is at origin)
	struct ParticleAndBox{ shared_ptr<Particle> par; AlignedBox3r extents; };
	// return (one or multiple, for clump) particles and extents (min and max)
	// extents are computed for position of (0,0,0)
	virtual std::tuple<Real,vector<ParticleAndBox>> operator()(const shared_ptr<Material>& m, const Real& time){ throw std::runtime_error("Calling ParticleGenerator.operator() (abstract method); use derived classes."); }
	virtual Real critDt(Real density, Real young) { return Inf; }
	// called when the particle placement failed; the generator must revoke it and update its bookkeeping information (e.g. PSD, generated radii and diameters etc)
	virtual void revokeLast(){ if(save && !genDiamMassTime.empty()) genDiamMassTime.resize(genDiamMassTime.size()-1); }
	virtual void clear(){ genDiamMassTime.clear(); }
	// spheres-only generators override this to enable some optimizations
	virtual bool isSpheresOnly() const { throw std::logic_error(pyStr()+" should override ParticleGenerator.isSpheresOnly."); }
	virtual Real padDist() const {  throw std::logic_error(pyStr()+" should override ParticleGenerator.padDist."); }
	virtual Vector2r minMaxDiam() const { throw std::logic_error(pyStr()+" should override ParticleGenerator.minMaxDiam."); }
	py::object pyPsd(bool mass, bool cumulative, bool normalize, const Vector2r& dRange, const Vector2r& tRange, int num) const;
	py::object pyDiamMass(bool zipped=false) const;
	Real pyMassOfDiam(Real min, Real max) const;
	py::tuple pyCall(const shared_ptr<Material>& m, const Real& time){ Real diam; vector<ParticleAndBox> pee; std::tie(diam,pee)=(*this)(m,time); py::list ret; for(const auto& pe: pee) ret.append(py::make_tuple(pe.par,pe.extents)); return py::make_tuple(diam,ret); }
	#define woo_dem_ParticleGenerator__CLASS_BASE_DOC_ATTRS_PY \
		ParticleGenerator,Object,"Abstract class for generating particles", \
		((vector<Vector3r>,genDiamMassTime,,AttrTrait<Attr::readonly>().noGui().noDump(),"List of generated particle's (equivalent) radii and masses (for making granulometry)")) \
		((bool,save,true,,"Save generated particles so that PSD can be generated afterwards")) \
		,/*py*/ \
			.def("psd",&ParticleGenerator::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=true,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("tRange")=Vector2r(NaN,NaN),py::arg("num")=80),"Return PSD for particles generated.") \
			.def("diamMass",&ParticleGenerator::pyDiamMass,(py::arg("zipped")=false),"With *zipped*, return list of (diameter, mass); without *zipped*, return tuple of 2 arrays, diameters and masses.") \
			.def("massOfDiam",&ParticleGenerator::pyMassOfDiam,(py::arg("min")=0,py::arg("max")=Inf),"Return mass of particles of which diameters are between *min* and *max*.") \
			.def("clear",&ParticleGenerator::clear,"Clear stored data about generated particles; only subsequently generated particles will be considered in the PSD.") \
			.def("critDt",&ParticleGenerator::critDt,(py::arg("density"),py::arg("young")),"Return critical timestep for particles generated by us, given that their density and Young's modulus are as given in arguments.") \
			.def("padDist",&ParticleGenerator::padDist,"Return padding distance by which the factory geometry should be shrunk before generating a random point.") \
			.def("__call__",&ParticleGenerator::pyCall,(py::arg("mat"),py::arg("time")=0),"Call the generation routine, returning one particle (at origin) and its bounding-box when at origin. Useful for debugging.") \
			.def("minMaxDiam",&ParticleGenerator::minMaxDiam,"Return the range of minimum and maximum diameters.")
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ParticleGenerator__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(ParticleGenerator);

struct MinMaxSphereGenerator: public ParticleGenerator{
	std::tuple<Real,vector<ParticleAndBox>> operator()(const shared_ptr<Material>&m, const Real& time);
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
	virtual void operator()(const shared_ptr<Node>& node){ throw std::runtime_error("Calling ParticleShooter.setVelocities (abstract method); use derived classes"); }
	#define woo_dem_ParticleShooter__CLASS_BASE_DOC \
		ParticleShooter,Object,"Abstract class for assigning initial velocities to generated particles."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_ParticleShooter__CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(ParticleShooter);

struct AlignedMinMaxShooter: public ParticleShooter{
	void operator()(const shared_ptr<Node>& n) WOO_CXX11_OVERRIDE {
		if(isnan(vRange.maxCoeff())){ throw std::runtime_error("AlignedMinMaxShooter.vRange: must not contain NaN."); }
		n->getData<DemData>().vel=dir*(vRange[0]+Mathr::UnitRandom()*(vRange[1]-vRange[0]));
		n->getData<DemData>().angVel=Vector3r::Zero();
	}
	void postLoad(AlignedMinMaxShooter&, void*){ dir.normalize(); }
	#define woo_dem_AlignedMinMaxShooter__CLASS_BASE_DOC_ATTRS \
		AlignedMinMaxShooter,ParticleShooter,"Shoot particles in one direction, with velocity magnitude constrained by vRange values", \
		((Vector3r,dir,Vector3r::UnitX(),AttrTrait<Attr::triggerPostLoad>(),"Direction (will be normalized).")) \
		((Vector2r,vRange,Vector2r(NaN,NaN),,"Minimum and maximum velocities."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_AlignedMinMaxShooter__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(AlignedMinMaxShooter);

struct SpatialBias: public Object{
	// returns biased position in unit cube, based on particle radius
	virtual Vector3r unitPos(const Real& diam){ throw std::runtime_error(pyStr()+": SpatialBias.biasedPos not overridden."); }
	#define woo_dem_SpatialBias__CLASS_BASE_DOC_PY \
		SpatialBias,Object,"Functor which biases random position (in unit cube) based on particle diameter, returning the biased position in unit cube. These objects are used with :obj:`RandomInlet.spatialBias` to make possible things like sptially graded distribution of random particles." \
		,/*py*/ .def("unitPos",&SpatialBias::unitPos,(py::arg("diam")),"Return biased position inside a unit cube (caller transforms this to cartesian coordinates in the simulation space.")

	WOO_DECL__CLASS_BASE_DOC_PY(woo_dem_SpatialBias__CLASS_BASE_DOC_PY);
};
WOO_REGISTER_OBJECT(SpatialBias);

struct AxialBias: public SpatialBias{
	void postLoad(AxialBias&,void*);
	Vector3r unitPos(const Real& diam) WOO_CXX11_OVERRIDE;
	#define woo_dem_AxialBias__CLASS_BASE_DOC_ATTRS \
		AxialBias,SpatialBias,"Bias position (within unit interval) along :obj:`axis` :math:`p`, so that radii are distributed along :obj:`axis`, as in :math:`p=\\frac{d-d_0}{d_1-d_0}+f\\left(a-\\frac{1}{2}\\right)`, where :math:`f` is the :obj:`fuzz`, :math:`a` is unit random number, :math:`d` is the current particle diameter and :math:`d_0` and :math:`d_1` are diameters at the lower and upper end (:obj:`d01`). :math:`p` is clamped to :math:`\\rangle0,1\\langle` after the computation.", \
			((int,axis,0,AttrTrait<Attr::triggerPostLoad>(),"Axis which is biased.")) \
			((Vector2r,d01,Vector2r(NaN,NaN),,"Diameter at the lower and upper end (the order matters); it is possible that :math:`r_0>r_1`, in which case the bias is reversed (bigger radii have smaller coordinate).")) \
			((Real,fuzz,0,,"Allow for random variations around the position determined from diameter."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_AxialBias__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(AxialBias);

struct PsdAxialBias: public AxialBias {
	void postLoad(PsdAxialBias&,void*);
	Vector3r unitPos(const Real& diam) WOO_CXX11_OVERRIDE;
	#define woo_dem_PsdAxialBias__CLASS_BASE_DOC_ATTRS \
		PsdAxialBias,AxialBias,"Bias position based on PSD curve, so that fractions get the amount of space they require according to their percentage. The PSD must be specified with mass fractions, using With :obj:`discrete` (suitable for use with :obj:`PsdParticleGenerator.discrete`), the whole interval between the current and previous radius will be used with uniform probability, allowing for layered particle arangement. The :obj:`d01` attribute is ignored with PSD.", \
			((vector<Vector2r>,psdPts,,AttrTrait<Attr::triggerPostLoad>(),"Points of the mapping function, similar to :obj:`PsdParticleGenerator.psdPts`."))\
			((bool,invert,false,,"Reverse the ordering along the axis, which makes the bigger particles be close to zero.")) \
			((bool,discrete,false,,"Interpret :obj:`psdPts` as piecewise-constant (rather than piecewise-linear) function. Each diameter will be distributed uniformly in the whole interval between percentage of the current and previous points.")) \
			((vector<int>,reorder,,AttrTrait<Attr::triggerPostLoad>(),"Reorder the PSD fractions; this is mainly useful for discrete distributions, where the order can be non-increasing, such as with ``reorder=[1,0,2]`` which will put the finest fraction in the middle of the other two"))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_PsdAxialBias__CLASS_BASE_DOC_ATTRS);
	WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(PsdAxialBias);

struct LayeredAxialBias: public AxialBias {
	void postLoad(LayeredAxialBias&,void*);
	Vector3r unitPos(const Real& diam) WOO_CXX11_OVERRIDE;
	#define woo_dem_LayeredAxialBias__CLASS_BASE_DOC_ATTRS \
		LayeredAxialBias,AxialBias,"Bias position so that particles occupy different layers based on their diameter. This is similar to :obj:`PsdAxialBias` with :obj:`~PsdAxialBias.discrete`, but allows for more flexibility, such as one fraction occupying multiple non-adjacent layers.", \
		((vector<VectorXr>,layerSpec,,AttrTrait<Attr::triggerPostLoad>(),"Vector specifying layering; each item contains the following numbers: ``dMin, dMax, xMin0, xMax0, xMin1, xMax1, ...``. A particle which falls within ``dMin, dMax`` will be placed, with uniform probability, into intervals specified by other couples. Coordinates are given in normalized space, so ``xMin..xMax`` must lie in in 〈0,1〉. Particles which do not fall into any fraction will not be biased (thus placed uniformly randomly), and a warning will be issued.")) \
		((vector<Real>,xRangeSum,,AttrTrait<Attr::readonly|Attr::noSave>().noGui().noDump(),"Sum of ``xMax_i-xMin_i`` for each fraction, for faster lookup. Internal/debugging use only."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_LayeredAxialBias__CLASS_BASE_DOC_ATTRS);
	WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(LayeredAxialBias);


struct Collider;

struct RandomInlet: public Inlet{
	WOO_DECL_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual Vector3r randomPosition(const Real& read, const Real& padDist){ throw std::runtime_error("Calling RandomInlet.randomPosition	(abstract method); use derived classes."); }
	virtual bool validateBox(const AlignedBox3r& b) { throw std::runtime_error("Calling ParticleFactor.validateBox (abstract method); use derived classes."); }
	void run();
	void pyClear(){ if(generator) generator->clear(); num=0; mass=0; stepGoalMass=0; /* do not reset stepPrev! */ }
	Real critDt() WOO_CXX11_OVERRIDE; 
	shared_ptr<Collider> collider;
	enum{MAXATT_ERROR=0,MAXATT_DEAD,MAXATT_WARN,MAXATT_SILENT};
	bool spheresOnly; // set at each step, queried from the generator
	#define woo_dem_RandomInlet__CLASS_BASE_DOC_ATTRS_PY \
		RandomInlet,Inlet,"Inlet generating new particles. This class overrides :obj:`woo.core.Engine.critDt`, which in turn calls :obj:`woo.dem.ParticleGenerator.critDt` with all possible :obj:`materials` one by one.", \
		((Real,massRate,NaN,AttrTrait<>().massRateUnit(),"Mass flow rate; if zero, generate as many particles as possible, until maxAttemps is reached.")) \
		((vector<shared_ptr<Material>>,materials,,,"Set of materials for new particles, randomly picked from")) \
		((shared_ptr<ParticleGenerator>,generator,,,"Particle generator instance")) \
		((shared_ptr<ParticleShooter>,shooter,,,"Particle shooter instance (assigns velocities to generated particles. If not given, particles have zero velocities initially.")) \
		((shared_ptr<SpatialBias>,spatialBias,,,"Make random position biased based on the radius of the current particle; if unset, distribute uniformly.")) \
		((int,maxAttempts,5000,,"Maximum attempts to position new particles randomly, without collision. If 0, no limit is imposed. When reached, :obj:`atMaxAttempts` determines, what will be done. Each particle is tried maxAttempts/maxMetaAttempts times, then a new particle is tried.")) \
		((int,attemptPar,5,,"Number of trying a different particle to position (each will be tried maxAttempts/attemptPar times)")) \
		((int,atMaxAttempts,MAXATT_ERROR,AttrTrait<Attr::namedEnum>().namedEnum({{MAXATT_ERROR,{"error"}},{MAXATT_DEAD,{"dead"}},{MAXATT_WARN,{"warn"}},{MAXATT_SILENT,{"silent",""}}}),"What to do when maxAttempts is reached.")) \
		((Real,padDist,0.,AttrTrait<Attr::readonly>(),"Pad geometry by this distance inside; random points will be chosen inside the shrunk geometry, whereas boxes will be validated in the larger one. This attribute must be set by the generator.")) \
		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene.energy")) \
		((Real,color,NaN,,"Color for new particles (NaN for random; negative for keeping color assigned by the generator).")) \
		((Real,stepGoalMass,0,AttrTrait<Attr::readonly>(),"Mass to be attained in this step")) \
		((bool,collideExisting,true,,"Consider collisions with pre-existing particle; this is generally a good idea, though if e.g. there are no pre-existing particles, it is useful to set to ``False`` to avoid having to define collider for no other reason than make :obj:`RandomInlet` happy.")) \
		,/*py*/ \
			.def("clear",&RandomInlet::pyClear) \
			.def("randomPosition",&RandomInlet::randomPosition) \
			.def("validateBox",&RandomInlet::validateBox) \
			; \
			_classObj.attr("maxAttError")=(int)MAXATT_ERROR; \
			_classObj.attr("maxAttDead")=(int)MAXATT_DEAD; \
			_classObj.attr("maxAttWarn")=(int)MAXATT_WARN; \
			_classObj.attr("maxAttSilent")=(int)MAXATT_SILENT;
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_RandomInlet__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(RandomInlet);

/*
repsect periodic boundaries when this is defined; this makes configurable side of BoxInlet transparent, i.e.
new particles can cross the boundary; note however that this is not properly supported in RandomInlet, where periodicity is not take in account when detecting overlap with particles generated within the same step
*/
	// #define BOX_FACTORY_PERI
struct BoxInlet: public RandomInlet{
	Vector3r randomPosition(const Real& rad, const Real& padDist) WOO_CXX11_OVERRIDE;
	#ifdef BOX_FACTORY_PERI
		bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE { return scene->isPeriodic?box.contains(b):validatePeriodicBox(b); }
		bool validatePeriodicBox(const AlignedBox3r& b) const;
	#else
		bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE { return box.contains(b); }
	#endif

	#ifdef WOO_OPENGL
		void render(const GLViewInfo&) WOO_CXX11_OVERRIDE {
			if(isnan(glColor)) return;
			GLUtils::AlignedBox(box,CompUtils::mapColor(glColor));
			renderMassAndRate(box.center());
		}
	#endif

	#ifdef BOX_FACTORY_PERI
		#define woo_dem_BoxInlet__periSpanMask__BOX_FACTORY_PERI ((int,periSpanMask,0,,"When running in periodic scene, particles bboxes will be allowed to stick out of the factory in those directions (used to specify that the factory itself should be periodic along those axes). 1=x, 2=y, 4=z."))
	#else
		#define woo_dem_BoxInlet__periSpanMask__BOX_FACTORY_PERI		
	#endif

	#define woo_dem_BoxInlet__CLASS_BASE_DOC_ATTRS \
		BoxInlet,RandomInlet,"Generate particle inside axis-aligned box volume.", \
		((AlignedBox3r,box,AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),,"Box volume specification (lower and upper corners)")) \
		woo_dem_BoxInlet__periSpanMask__BOX_FACTORY_PERI 
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxInlet__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(BoxInlet);

struct CylinderInlet: public RandomInlet{
	Vector3r randomPosition(const Real& rad, const Real& padDist) WOO_CXX11_OVERRIDE; /* http://stackoverflow.com/questions/5837572/generate-a-random-point-within-a-circle-uniformly */
	bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE; /* check all corners are inside the cylinder */
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&) WOO_CXX11_OVERRIDE;
	#endif

	#define woo_dem_CylinderInlet__CLASS_BASE_DOC_ATTRS \
		CylinderInlet,RandomInlet,"Generate particle inside an arbitrarily oriented cylinder.", \
		((shared_ptr<Node>,node,,,"Node defining local coordinate system. If not given, global coordinates are used.")) \
		((Real,height,NaN,AttrTrait<>().lenUnit(),"Height along the local :math:`x`-axis.")) \
		((Real,radius,NaN,AttrTrait<>().lenUnit(),"Radius of the cylinder (perpendicular to the local :math:`x`-axis).")) \
		((int,glSlices,16,,"Number of subdivision slices for rendering."))

		// ((vector<AlignedBox3r>,boxesTried,,AttrTrait<>().noGui(),"Bounding boxes being attempted to validate -- debugging only."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_CylinderInlet__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(CylinderInlet);

struct BoxInlet2d: public BoxInlet{
	Vector2r flatten(const Vector3r& v){ return Vector2r(v[(axis+1)%3],v[(axis+2)%3]); }
	bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE { return AlignedBox2r(flatten(box.min()),flatten(box.max())).contains(AlignedBox2r(flatten(b.min()),flatten(b.max()))); }
	#define woo_dem_BoxInlet2d__CLASS_BASE_DOC_ATTRS \
		BoxInlet2d,BoxInlet,"Generate particles inside axis-aligned plane (its normal axis is given via the :obj:`axis` attribute; particles are allowed to stick out of that plane.", \
		((short,axis,2,,"Axis normal to the plane in which particles are generated.")) 
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxInlet2d__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(BoxInlet2d);

struct ArcInlet: public RandomInlet{
	Vector3r randomPosition(const Real& rad, const Real& padDist) WOO_CXX11_OVERRIDE;
	bool validateBox(const AlignedBox3r& b) WOO_CXX11_OVERRIDE;
	void postLoad(ArcInlet&, void* attr);
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&) WOO_CXX11_OVERRIDE;
	#endif
	#define woo_dem_ArcInlet__CLASS_BASE_DOC_ATTRS \
		ArcInlet,RandomInlet,"Inlet generating particles in prismatic arc (revolved rectangle) specified using `cylindrical coordinates <http://en.wikipedia.org/wiki/Cylindrical_coordinate_system>`__ (with the ``ISO 31-11`` convention, as mentioned at the Wikipedia page) in a local system.", \
		((shared_ptr<Node>,node,make_shared<Node>(),AttrTrait<Attr::triggerPostLoad>(),"Node defining local coordinates system. *Must* be given.")) \
		((AlignedBox3r,cylBox,,,"Box in cylindrical coordinates, as: (ρ₀,φ₀,z₀),(ρ₁,φ₁,z₁). ρ must be non-negative, (φ₁-φ₀)≤2π.")) \
		((int,glSlices,32,,"Number of slices for rendering circle (the arc takes the proportionate value"))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_ArcInlet__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(ArcInlet);

#if 0
struct ArcShooter: public ParticleShooter{
	void operator()(const shared_ptr<Node>& node) WOO_CXX11_OVERRIDE {
	}
	// void postLoad(ArcShooter&, void*){ dir.normalize(); }
	#define woo_dem_ArcShooter__CLASS_BASE_DOC_ATTRS \
		ArcShooter,ParticleShooter,"Shoot particles in one direction, with velocity magnitude constrained by vRange values", \
		/*attrs*/

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_ArcShooter__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(ArcShooter);
#endif
