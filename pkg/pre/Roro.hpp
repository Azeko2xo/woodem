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
		((Real,massFlowRate,1600,AttrTrait<>().massFlowRateUnit().prefUnit("t/year"),"Incoming mass flow rate (considering real length)"))
		((Real,time,2,AttrTrait<>().timeUnit(),"Time of the simulation (after reaching steady state)"))


		((Real,flowVel,1.,AttrTrait<>().velUnit().cxxType("fooBar"),"Velocity of generated particles"))
		((Real,cylDiameter,.2,AttrTrait<>().lenUnit(),"Diameter of cylinders"))
		((Real,inclination,30*Mathr::PI/180.,AttrTrait<>().angleUnit().prefUnit("deg"),"Inclination cylinders"))
		((Real,gap,.038,AttrTrait<>().lenUnit().startGroup("Advanced"),"Gap between cylinders"))
		((Real,angVel,10.,AttrTrait<>().angVelUnit(),"Angular velocity of cylinders [rot/sec]"))
		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(0.02,.0),Vector2r(.03,.2),Vector2r(.04,.3),Vector2r(.05,.7)})/*set in the ctor*/,AttrTrait<>().triggerPostLoad().multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of generated particles: first value is diameter [mm], second value is cummulative percentage [%]"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),,"Material of particles"))


		((Real,ccaDt   ,,AttrTrait<>().readonly().timeUnit().startGroup("Estimates"),"Δt"))
		((long,ccaSteps,,AttrTrait<>().readonly(),"number of steps"))
		((Real,totMass ,,AttrTrait<>().readonly().massUnit(),"mass amount"))

		((Vector2r,quivAmp,Vector2r(.05,.03),AttrTrait<>().startGroup("Tunables"),"Cylinder quiver amplitudes (horizontal and vertical), relative to cylinder radius"))
		((Vector3r,quivHPeriod,Vector3r(3000,5000,3),,"Horizontal quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Vector3r,quivVPeriod,Vector3r(5000,11000,5),,"Vertical quiver period (relative to Δt); assigned quasi-randomly from the given range, with z-component giving modulo divisor"))
		((Real,gravity,100.,AttrTrait<>().accelUnit(),"Gravity acceleration magnitude"))
		((int,steadyOver,500,,"Start steady (measured) phase when this number of particles falls over the last cylinder"))
		((int,factStepPeriod,200,,"Run factory (and deleters) every *factStepPeriod* steps."))
		((Real,pWaveSafety,.7,Attr::triggerPostLoad,"Safety factor for critical timestep"))
		((Real,rateSmooth,.1,,"Smoothing factor for plotting rates in factory and deleters"))

		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e7;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(Roro);
