#include<yade/core/Preprocessor.hpp>
#include<yade/core/Scene.hpp>

#include<yade/pkg/dem/FrictMat.hpp>

struct Roro: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("yade.pre.Roro_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",

		YADE_GROUP_SEPARATOR(basic)
		((int,cylNum,6,,"Number of cylinders"))
		((Vector2i,cylNum_range,Vector2i(4,15),Attr::noGui,"range for numCyl"))
		((Real,massFlowRate,100,AttrTrait().massFlowUnit(),"Mass flow rate of generated particles [kg/s]"))
		((Real,flowVel,1.,AttrTrait().velUnit().cxxType("fooBar"),"Velocity of generated particles"))
		((Real,cylDiameter,.2,AttrTrait().lenUnit(),"Diameter of cylinders"))
		((Real,cylLength,.5,AttrTrait().lenUnit(),"Length of cylinders"))

		YADE_GROUP_SEPARATOR(advanced)
		((Real,gap,.04,AttrTrait().lenUnit(),"Gap between cylinders"))
		((Real,inclination,30,AttrTrait().angleUnit(),"Inclination cylinders"))
		((Real,angVel,10.,AttrTrait().angVelUnit(),"Angular velocity of cylinders [rot/sec]"))
		((vector<Vector2r>,psd,/*set in the ctor*/,,"Particle size distribution of generated particles: first value is diameter [mm], second value is cummulative percentage [%]"))
		//((Real,adhesion,,,"Adhesion coefficient between particles"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),,"Material of particles"))
		, /*ctor*/
			psd.push_back(Vector2r(.02,.0));
			psd.push_back(Vector2r(.03,.2));
			psd.push_back(Vector2r(.04,.3));
			psd.push_back(Vector2r(.05,.7));
			material->density=3200;
			material->cast<FrictMat>().young=1e7;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(Roro);
