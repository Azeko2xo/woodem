#include<yade/core/Preprocessor.hpp>
#include<yade/core/Scene.hpp>

#include<yade/pkg/dem/FrictMat.hpp>

struct Roro: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("yade.pre.Roro_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",

		((int,cylNum,6,AttrTrait<>().range(Vector2i(4,15)).startGroup("Basic"),"Number of cylinders"))
		((Real,massFlowRate,100,AttrTrait<>().massFlowRateUnit().prefUnit("t/year"),"Mass flow rate of generated particles [kg/s]"))
		((Real,flowVel,1.,AttrTrait<>().velUnit().cxxType("fooBar"),"Velocity of generated particles"))
		((Real,cylDiameter,.2,AttrTrait<>().lenUnit(),"Diameter of cylinders"))
		((Real,cylLength,.5,AttrTrait<>().lenUnit(),"Length of cylinders"))
		((Real,inclination,30*Mathr::PI/180.,AttrTrait<>().angleUnit().prefUnit("deg"),"Inclination cylinders"))
		//((Real,rangeThing,.4,AttrTrait<>().range(Vector2r(0,2)),"something with range"))
		//((int,intChoice,2,AttrTrait<>().choice({0,1,2,3,4}),"something with int choice"))
		//((int,stringIntChoice,2,AttrTrait<>().choice({{0,"zero"},{1,"one"},{2,"two"}}),"something with int choice"))

		((Real,gap,.038,AttrTrait<>().lenUnit().startGroup("Advanced"),"Gap between cylinders"))
		((Real,angVel,10.,AttrTrait<>().angVelUnit(),"Angular velocity of cylinders [rot/sec]"))
		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(0.02,.0),Vector2r(.03,.2),Vector2r(.04,.3),Vector2r(.05,.7)})/*set in the ctor*/,AttrTrait<>().multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of generated particles: first value is diameter [mm], second value is cummulative percentage [%]"))
		//((Real,adhesion,,,"Adhesion coefficient between particles"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),,"Material of particles"))
		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e7;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(Roro);
