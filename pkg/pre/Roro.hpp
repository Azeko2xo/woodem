#include<yade/core/Preprocessor.hpp>
#include<yade/core/Scene.hpp>

#include<yade/pkg/dem/FrictMat.hpp>

struct Roro: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("yade.pre.Roro_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	void postLoad(Roro&){
		cylRelLen=cylLenSim/cylLenReal;
		totMass=time*massFlowRate;
		if(!psd.empty() && material){
			ccaDt=pWaveSafety*.5*psd[0][0]/sqrt(material->young/material->density);
			ccaSteps=time/ccaDt;
		} else {
			ccaDt=NaN; ccaSteps=-1;
		}
	}
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",
		((int, cylNum,6,AttrTrait<>().range(Vector2i(4,15)).startGroup("Basic"),"Number of cylinders"))
		((Real,cylLenReal,8,AttrTrait<>().lenUnit().triggerPostLoad(),"Real length of cylinders"))
		((Real,cylLenSim,.5,AttrTrait<>().lenUnit().triggerPostLoad(),"Simulated length of cylinders"))
		((Real,cylRelLen,,AttrTrait<>().readonly(),"Relative length of simulated cylinders"))
		((Real,massFlowRate,/*for real length*/2500,AttrTrait<>().massFlowRateUnit().prefUnit("Mt/y"),"Incoming mass flow rate (considering real length)"))
		((bool,conveyor,true,,"Feed particles using infinite conveyor rather than particles falling from above; see conveyorHt."))
		((Real,conveyorHt,.2,AttrTrait<>().lenUnit(),"Height of particle layor on the conveyor (only meaningful if *conveyor* is True)"))
		((Real,time,2,AttrTrait<>().timeUnit(),"Time of the simulation (after reaching steady state)"))
		((Real,flowVel,1.,AttrTrait<>().velUnit().cxxType("fooBar"),"Velocity of generated particles; this value is ignored if *conveyor* is True."))
		((Real,cylDiameter,.2,AttrTrait<>().lenUnit().prefUnit("mm"),"Diameter of cylinders"))
		((Real,inclination,30*Mathr::PI/180.,AttrTrait<>().angleUnit().prefUnit("deg"),"Inclination cylinders"))

		// Advanced
		((Real,gap,.038,AttrTrait<>().lenUnit().prefUnit("mm").startGroup("Advanced"),"Gap between cylinders"))
		((Real,angVel,10.,AttrTrait<>().angVelUnit().prefUnit("rot/min"),"Angular velocity of cylinders [rot/sec]"))
		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(0.02,.0),Vector2r(.03,.2),Vector2r(.04,.3),Vector2r(.05,.7)})/*set in the ctor*/,AttrTrait<>().triggerPostLoad().multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of generated particles: first value is diameter, second value is cummulative fraction"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),,"Material of particles"))

		// Estimates
		((Real,ccaDt   ,,AttrTrait<>().readonly().timeUnit().startGroup("Estimates"),"Δt"))
		((long,ccaSteps,,AttrTrait<>().readonly(),"number of steps"))
		((Real,totMass ,,AttrTrait<>().readonly().massUnit(),"mass amount"))

		// Tunables
		((Vector2r,quivAmp,Vector2r(.05,.03),AttrTrait<>().lenUnit().prefUnit("mm").startGroup("Tunables"),"Cylinder quiver amplitudes (horizontal and vertical), relative to cylinder radius"))
		((Vector3r,quivHPeriod,Vector3r(3000,5000,3),,"Horizontal quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Vector3r,quivVPeriod,Vector3r(5000,11000,5),,"Vertical quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Real,gravity,100.,AttrTrait<>().accelUnit(),"Gravity acceleration magnitude"))
		((Real,steadyFlowFrac,.9,,"Start steady (measured) phase when efflux (fall over, apertures, out-of-domain) reaches this fraction of influx (feed)"))
		((int,factStepPeriod,800,,"Run factory (and deleters) every *factStepPeriod* steps."))
		((Real,pWaveSafety,.7,AttrTrait<Attr::triggerPostLoad>(),"Safety factor for critical timestep"))
		((Real,rateSmooth,.4,,"Smoothing factor for plotting rates in factory and deleters"))
		((Real,vtkFreq,.25,,"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."))
		((string,vtkPrefix,"/tmp/",,"Prefix for saving VtkExport data, files will be called vtkPrefix+O.tags['id']. Don't forget trailing slash if vtkPrefix is a directory."))

		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e7;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(Roro);
