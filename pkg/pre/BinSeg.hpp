#pragma once 

#include<woo/core/Preprocessor.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

struct BinSeg: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("woo.pre.BinSeg_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	void postLoad(BinSeg&){
		if(!psd.empty() && material){
			ccaDt=pWaveSafety*.5*psd[0][0]/sqrt(material->young/material->density);
		} else {
			ccaDt=NaN;
		}
	}

	WOO_CLASS_BASE_DOC_ATTRS_CTOR(BinSeg,Preprocessor,"Preprocessor for the bin segregation simulation.",
		((Vector3r,size,Vector3r(1,1,.2),AttrTrait<>().lenUnit().prefUnit("mm").startGroup("Geometry").buttons({
			"Open hole 1","import woo.pre.BinSeg_; woo.pre.BinSeg_.openHole(1)","Open first (left) hole",
			"Open hole 2","import woo.pre.BinSeg_; woo.pre.BinSeg_.openHole(2)","Open second (right) hole",
			"Open both holes","import woo.pre.BinSeg_; woo.pre.BinSeg_.openHole(1); woo.pre.BinSeg_.openHole(2)","Open both holes",
			"Save spheres","import woo.pre.BinSeg_; woo.pre.BinSeg_.saveSpheres()","Save spheres to file",
			"Finish simulation","import woo.pre.BinSeg_; woo.pre.BinSeg_.finishSimulation()","End simulation, write report",
			}),"Size of the bin: width, height and depth (thickness)"))
		((Vector2r,hole1,Vector2r(.1,.1),AttrTrait<>().lenUnit().prefUnit("mm"),"Distance from the left wall and width of the first hole"))
		((Vector2r,hole2,Vector2r(.25,.1),AttrTrait<>().lenUnit().prefUnit("mm"),"Distance from the right wall and width of the second hole"))
		((Real,holeLedge,.03,AttrTrait<>().lenUnit().prefUnit("mm"),"Distance between front wall and hole, and backwall and hole"))
		((Vector2r,feedPos,Vector2r(.1,.1),AttrTrait<>().lenUnit().prefUnit("mm"),"Distance of the feed from the right wall and its width"))
		((Real,halfThick,0.,AttrTrait<>().lenUnit().prefUnit("mm"),"Half-thickenss of boundary plates; all dimensions are given as inner sizes (including hole widths), there is no influence of halfThick on geometry"))
		((bool,halfDp,true,,"Only simulate half of the bin, by putting frictionless wall in the middle"))

		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(0.015,.0),Vector2r(.025,.2),Vector2r(.03,1.)}),AttrTrait<>().startGroup("Feed").triggerPostLoad().multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of generated particles: first value is diameter, second value is cummulative fraction"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),AttrTrait<Attr::triggerPostLoad>(),"Material of particles"))
		((Real,feedRate,1.,,"Feed rate"))
		((shared_ptr<FrictMat>,wallMaterial,,,"Material of walls (if not given, material for particles is used)"))
		((string,loadSpheresFrom,"",,"Load initial packing from file (format x,y,z,radius)"))

		// Estimates
		((Real,ccaDt   ,,AttrTrait<>().readonly().timeUnit().startGroup("Estimates"),"Δt"))
		((long,ccaSteps,,AttrTrait<>().readonly(),"number of steps"))
		((Real,totMass ,,AttrTrait<>().readonly().massUnit(),"mass amount"))

		// Outputs
		((string,reportFmt,"/tmp/{tid}.xhtml",AttrTrait<>().startGroup("Outputs"),"Report output format (Scene.tags can be used)."))
		//((string,saveFmt,"/tmp/{tid}-{stage}.bin.gz",,"Savefile format; keys are :ref:`Scene.tags` and additionally ``{stage}`` will be replaced by 'init', 'steady' and 'done'."))
		//((int,backupSaveTime,1800,,"How often to save backup of the simulation (0 or negative to disable)"))
		//((Real,vtkFreq,4,AttrTrait<>(),"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."))
		//((string,vtkPrefix,"/tmp/{tid}-",,"Prefix for saving VtkExport data; formatted with ``format()`` providing :ref:`Scene.tags` as keys."))

		// Tunables
		((Real,gravity,10.,AttrTrait<>().accelUnit().startGroup("Tunables"),"Gravity acceleration magnitude"))
		((Real,damping,.4,,"Non-viscous damping coefficient"))
		((Real,pWaveSafety,.7,AttrTrait<Attr::triggerPostLoad>(),"Safety factor for critical timestep"))
		((Real,rateSmooth,.2,,"Smoothing factor for plotting rates in factory and deleters"))
		((int,factStep,200,,"Interval at which particle factory and deleters are run"))

		((vector<Particle::id_t>,hole1ids,,AttrTrait<>().noGui(),"Ids of hole 1 particles (for later removal)"))
		((vector<Particle::id_t>,hole2ids,,AttrTrait<>().noGui(),"Ids of hole 2 particles (for later removal)"))

#if 0
		// Tunables
		((int,factStepPeriod,800,AttrTrait<>().startGroup("Tunables"),"Run factory (and deleters) every *factStepPeriod* steps."))
		((string,variant,"plain",AttrTrait<>().choice({"plain","customer1","customer2"}),"Geometry of the feed and possibly other specific details"))
		((Vector2r,quivAmp,Vector2r(.0,.0),AttrTrait<>().lenUnit().prefUnit("mm"),"Cylinder quiver amplitudes (horizontal and vertical), relative to cylinder radius"))
		((Vector3r,quivHPeriod,Vector3r(3000,5000,3),,"Horizontal quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Vector3r,quivVPeriod,Vector3r(5000,11000,5),,"Vertical quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Real,steadyFlowFrac,1.,,"Start steady (measured) phase when efflux (fall over, apertures, out-of-domain) reaches this fraction of influx (feed); only used when *time* is given"))
		((Real,residueFlowFrac,.02,,"Stop simulation once delete rate drops below this fraction of the original feed rate (after the requested :ref:`mass` has been generated); only used when *mass* is given."))
		((Real,normPlastCoeff,1e3,,"Dimensionless normal plasticity coefficient :ref:`Law_L6Geom_FrictPhys_Pellet`; non-positive value disables normal plasticity."))
		((Vector2r,thinningCoeffs,Vector2r(0,.8),,"Parameters for plastic thinning: first component is :ref:`Law2_L6Geom_PelletPhys_Pellet.thinningFactor`, the other is :ref:`Law2_L6Geom_PelletPhys_Pellet.rMinFrac`."))
		((vector<int>,buckets,,,"Collect particles from several apertures together; each numer specifies how much apertures is taken; invalid values (past the number of cylinders) are simply ignored"))
		((vector<Real>,efficiencyGapFrac,vector<Real>({.9,1.,1.1,1.2,1.3}),,"Diameters relative to :ref:`gap` for which the sieving efficiency is determined."))
#endif

		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e5;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(BinSeg);

