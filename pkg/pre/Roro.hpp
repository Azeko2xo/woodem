#pragma once 

#include<woo/core/Preprocessor.hpp>
#include<woo/core/Scene.hpp>

#include<woo/pkg/dem/Pellet.hpp>

struct Roro: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("woo.pre.Roro_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	void postLoad(Roro&){
		cylRelLen=cylLenSim/cylLenReal;
		totMass=time*massFlowRate;
		// compute estimates from psd and material
		if(!psd.empty() && material){
			ccaDt=pWaveSafety*.5*psd[0][0]/sqrt(material->young/material->density);
			ccaSteps=time/ccaDt;
		} else {
			ccaDt=NaN; ccaSteps=-1;
		}
		#if 0
			// compute gap if cylinder coordinates are given directly
			if(cylXzd.size()>1){
				gap=Inf;
				for(size_t i=0;i<cylXzd.size()-1;i++) gap=min(gap,(Vector2r(cylXzd[i][0],cylXzd[i][1])-Vector2r(cylXzd[i+1][0],cylXzd[i+1][1])).norm()-cylXzd[i][2]/2.-cylXzd[i][2]/2.);
			}
		#endif
	}
	WOO_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",
		((Real,cylLenReal,2,AttrTrait<>().lenUnit().triggerPostLoad().startGroup("General"),"Real length of cylinders"))
		((Real,cylLenSim,.1,AttrTrait<>().lenUnit().triggerPostLoad(),"Simulated length of cylinders"))
		((Real,cylRelLen,,AttrTrait<>().readonly(),"Relative length of simulated cylinders"))
		((Real,massFlowRate,/*for real length*/100,AttrTrait<>().massFlowRateUnit().prefUnit("t/h"),"Incoming mass flow rate (considering real length)"))
		((Real,conveyorHt,.05,AttrTrait<>().lenUnit(),"Height of particle layor on the conveyor"))
		((Real,time,2,AttrTrait<>().timeUnit(),"Time of the simulation (after reaching steady state); if non-positive, don't wait for steady state, use *mass* instead."))
		((Real,mass,0,AttrTrait<>().massUnit(),"Total feed mass; if non-positive, don't limit generated mass, simulate *time* of steady state instead"))

		// Cylinders
		((vector<Vector3r>,cylXzd,,AttrTrait<>().startGroup("Cylinders").triggerPostLoad().lenUnit().prefUnit("mm"),"Coordinates and diameters of cylinders. If empty, *cylNum*, *cylDiameter*, *inclination* and *gap* are used to compute coordinates automatically"))
		((Real,angVel,2*Mathr::PI,AttrTrait<>().angVelUnit().prefUnit("rot/min"),"Angular velocity of cylinders [rot/sec]"))
		((int, cylNum,6,AttrTrait<>()/*.range(Vector2i(4,30))*/,"Number of cylinders (only used when cylXzd is empty)"))
		((Real,cylDiameter,.085,AttrTrait<>().lenUnit().altUnits({{"in",1/0.0254}}).prefUnit("mm"),"Diameter of cylinders (only used when cylXzd is empty)"))
		((Real,inclination,15*Mathr::PI/180.,AttrTrait<>().angleUnit().prefUnit("deg"),"Inclination of cylinder plane (only used when cylXzd is empty)"))
		((Real,gap,.01,AttrTrait<>().lenUnit().prefUnit("mm"),"Gap between cylinders (computed automatically if cylXzd is given)"))
		((vector<Real>,gaps,,AttrTrait<>().lenUnit().prefUnit("mm"),"Variable gaps between cylinders; if given, the value of *gap* and *cylNum* is not used for creating cylinder coordinates"))

		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(0.005,.0),Vector2r(.01,.2),Vector2r(.02,1.)})/*set in the ctor*/,AttrTrait<>().startGroup("Pellets").triggerPostLoad().multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of generated particles: first value is diameter, second value is cummulative fraction"))
		((shared_ptr<PelletMat>,material,make_shared<PelletMat>(),,"Material of particles"))
		((shared_ptr<PelletMat>,cylMaterial,,,"Material of cylinders (if not given, material for particles is used for cylinders)"))
		((shared_ptr<PelletMat>,plateMaterial,,,"Material of plates (if not given, material for cylinders is used for cylinders)"))

		// Estimates
		((Real,ccaDt   ,,AttrTrait<>().readonly().timeUnit().startGroup("Estimates"),"Δt"))
		((long,ccaSteps,,AttrTrait<>().readonly(),"number of steps"))
		((Real,totMass ,,AttrTrait<>().readonly().massUnit(),"mass amount"))

		// Outputs
		((string,reportFmt,"/tmp/{tid}.xhtml",AttrTrait<>().startGroup("Outputs"),"Report output format (Scene.tags can be used)."))
		((string,feedCacheDir,".",,"Directory where to store pre-generated feed packings"))
		((string,saveFmt,"/tmp/{tid}-{stage}.bin.gz",,"Savefile format; keys are :ref:`Scene.tags` and additionally ``{stage}`` will be replaced by 'init', 'steady' and 'done'."))
		((int,backupSaveTime,1800,,"How often to save backup of the simulation (0 or negative to disable)"))
		((Real,vtkFreq,4,AttrTrait<>(),"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."))
		((string,vtkPrefix,"/tmp/{tid}-",,"Prefix for saving VtkExport data; formatted with ``format()`` providing :ref:`Scene.tags` as keys."))
		((vector<string>,reportHooks,,AttrTrait<>().noGui(),"Python expressions returning a 3-tuple with 1. raw HTML to be included in the report, 2. list of (figureName,matplotlibFigure) to be included in figures, 3. dictionary to be added to the 'custom' dict saved in the database."))

		// Tunables
		((int,factStepPeriod,800,AttrTrait<>().startGroup("Tunables"),"Run factory (and deleters) every *factStepPeriod* steps."))
		((Real,pWaveSafety,.7,AttrTrait<Attr::triggerPostLoad>(),"Safety factor for critical timestep"))
		((string,variant,"plain",AttrTrait<>().choice({"plain","customer1","customer2"}),"Geometry of the feed and possibly other specific details"))
		((Real,gravity,10.,AttrTrait<>().accelUnit(),"Gravity acceleration magnitude"))
		((Vector2r,quivAmp,Vector2r(.0,.0),AttrTrait<>().lenUnit().prefUnit("mm"),"Cylinder quiver amplitudes (horizontal and vertical), relative to cylinder radius"))
		((Vector3r,quivHPeriod,Vector3r(3000,5000,3),,"Horizontal quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Vector3r,quivVPeriod,Vector3r(5000,11000,5),,"Vertical quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Real,steadyFlowFrac,1.,,"Start steady (measured) phase when efflux (fall over, apertures, out-of-domain) reaches this fraction of influx (feed); only used when *time* is given"))
		((Real,residueFlowFrac,.02,,"Stop simulation once delete rate drops below this fraction of the original feed rate (after the requested :ref:`mass` has been generated); only used when *mass* is given."))
		((Real,rateSmooth,.2,,"Smoothing factor for plotting rates in factory and deleters"))
		((Vector2r,thinningCoeffs,Vector2r(0,.8),,"Parameters for plastic thinning: first component is :ref:`Law2_L6Geom_PelletPhys_Pellet.thinningFactor`, the other is :ref:`Law2_L6Geom_PelletPhys_Pellet.rMinFrac`."))
		((vector<int>,buckets,,,"Collect particles from several apertures together; each numer specifies how much apertures is taken; invalid values (past the number of cylinders) are simply ignored"))
		((vector<Real>,efficiencyGapFrac,vector<Real>({.9,1.,1.1,1.2,1.3}),,"Diameters relative to :ref:`gap` for which the sieving efficiency is determined."))
		((Real,__nonexistent__,,AttrTrait<Attr::noSave>().hidden(),""))
		, /* deprec */
			((normPlastCoeff,__nonexistent__,"normPlastCoeff is inside PelletMat now, defining RoRo.normPlastCoeff has no effect."))
		, /* init */
		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e5;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
			material->cast<PelletMat>().normPlastCoeff=50;
			material->cast<PelletMat>().kaDivKn=0.;
		, /* py */
	);
};
REGISTER_SERIALIZABLE(Roro);
