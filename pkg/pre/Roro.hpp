#pragma once 

#include<woo/core/Preprocessor.hpp>
#include<woo/core/Scene.hpp>

#include<woo/pkg/dem/FrictMat.hpp>

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
		// compute gap if cylinder coordinates are given directly
		if(cylXzd.size()>1){
			gap=Inf;
			for(size_t i=0;i<cylXzd.size()-1;i++) gap=min(gap,(Vector2r(cylXzd[i][0],cylXzd[i][1])-Vector2r(cylXzd[i+1][0],cylXzd[i+1][1])).norm()-cylXzd[i][2]/2.-cylXzd[i][2]/2.);
		}
	}
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",
		((Real,cylLenReal,8,AttrTrait<>().lenUnit().triggerPostLoad().startGroup("General"),"Real length of cylinders"))
		((Real,cylLenSim,.5,AttrTrait<>().lenUnit().triggerPostLoad(),"Simulated length of cylinders"))
		((Real,cylRelLen,,AttrTrait<>().readonly(),"Relative length of simulated cylinders"))
		((Real,massFlowRate,/*for real length*/2500,AttrTrait<>().massFlowRateUnit().prefUnit("Mt/y"),"Incoming mass flow rate (considering real length)"))
		((bool,conveyor,true,AttrTrait<>().noGui(),"Feed particles using infinite conveyor rather than particles falling from above; see conveyorHt."))
		((Real,conveyorHt,.2,AttrTrait<>().lenUnit(),"Height of particle layor on the conveyor (only meaningful if *conveyor* is True)"))
		((Real,time,2,AttrTrait<>().timeUnit(),"Time of the simulation (after reaching steady state)"))
		((Real,flowVel,1.,AttrTrait<>().velUnit().cxxType("fooBar"),"Velocity of generated particles; this value is ignored if *conveyor* is True."))

		// Cylinders
		((vector<Vector3r>,cylXzd,,AttrTrait<>().startGroup("Cylinders").triggerPostLoad().lenUnit().prefUnit("mm"),"Coordinates and diameters of cylinders. If empty, *cylNum*, *cylDiameter*, *inclination* and *gap* are used to compute coordinates automatically"))
		((Real,angVel,10.,AttrTrait<>().angVelUnit().prefUnit("rot/min"),"Angular velocity of cylinders [rot/sec]"))
		((int, cylNum,6,AttrTrait<>()/*.range(Vector2i(4,30))*/,"Number of cylinders (only used when cylXzd is empty)"))
		((Real,cylDiameter,.2,AttrTrait<>().lenUnit().prefUnit("mm"),"Diameter of cylinders (only used when cylXzd is empty)"))
		((Real,inclination,30*Mathr::PI/180.,AttrTrait<>().angleUnit().prefUnit("deg"),"Inclination of cylinder plane (only used when cylXzd is empty)"))
		((Real,gap,.038,AttrTrait<>().lenUnit().prefUnit("mm"),"Gap between cylinders (computed automatically if cylXzd is given)"))

		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(0.02,.0),Vector2r(.03,.2),Vector2r(.04,.3),Vector2r(.05,.7)})/*set in the ctor*/,AttrTrait<>().startGroup("Pellets").triggerPostLoad().multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of generated particles: first value is diameter, second value is cummulative fraction"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),,"Material of particles"))

		// Estimates
		((Real,ccaDt   ,,AttrTrait<>().readonly().timeUnit().startGroup("Estimates"),"Δt"))
		((long,ccaSteps,,AttrTrait<>().readonly(),"number of steps"))
		((Real,totMass ,,AttrTrait<>().readonly().massUnit(),"mass amount"))

		// Outputs
		((string,reportFmt,"/tmp/{tid}.xhtml",AttrTrait<>().startGroup("Outputs"),"Report output format (Scene.tags can be used)."))
		((string,feedCacheDir,".",,"Directory where to store pre-generated feed packings"))
		((string,saveFmt,"/tmp/{tid}-{stage}.bin.gz",,"Savefile format; keys are :ref:`Scene.tags` and additionally ``{stage}`` will be replaced by 'init', 'steady' and 'done'."))
		((int,vtkFreq,4,AttrTrait<>(),"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."))
		((string,vtkPrefix,"/tmp/{tid}-",,"Prefix for saving VtkExport data; formatted with ``format()`` providing :ref:`Scene.tags` as keys."))

		// Tunables
		((int,factStepPeriod,800,AttrTrait<>().startGroup("Tunables"),"Run factory (and deleters) every *factStepPeriod* steps."))
		((Real,pWaveSafety,.7,AttrTrait<Attr::triggerPostLoad>(),"Safety factor for critical timestep"))
		((Real,gravity,100.,AttrTrait<>().accelUnit(),"Gravity acceleration magnitude"))
		((Vector2r,quivAmp,Vector2r(.05,.03),AttrTrait<>().lenUnit().prefUnit("mm"),"Cylinder quiver amplitudes (horizontal and vertical), relative to cylinder radius"))
		((Vector3r,quivHPeriod,Vector3r(3000,5000,3),,"Horizontal quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Vector3r,quivVPeriod,Vector3r(5000,11000,5),,"Vertical quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Real,steadyFlowFrac,1.,,"Start steady (measured) phase when efflux (fall over, apertures, out-of-domain) reaches this fraction of influx (feed)"))
		((Real,rateSmooth,.2,,"Smoothing factor for plotting rates in factory and deleters"))


		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e7;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(Roro);
