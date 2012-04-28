#include<yade/core/Preprocessor.hpp>
#include<yade/core/Scene.hpp>

#include<yade/pkg/dem/FrictMat.hpp>

struct Roro: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("yade.pre.Roro_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",
		((int,cylNum,6,,"Number of cylinders"))
		//((Vector2i,numCyl_range,Vector2i(1,12),Attr::noGui,"range for numCyl"))
		((Real,massFlowRate,100,,"Mass flow rate of generated particles [kg/s]"))
		((Real,flowVel,1.,,"Velocity of generated particles [m/s]"))
		((Real,cylDiameter,.2,,"Diameter of cylinders [m]"))
		((Real,cylLength,.5,,"Length of cylinders [m]"))
		((int,_groupSeparator_whatever,Attr::readonly,,"<some group name>"))
		((Real,gap,.04,,"Gap between cylinders [m]"))
		((Real,inclination,40,,"Inclination cylinders [deg]"))
		((Real,angVel,10.,,"Angular velocity of cylinders [rot/sec]"))
		((vector<Vector2r>,psd,/*set in the ctor*/,,"Particle size distribution of generated particles: first value is diameter [mm], second value is cummulative percentage [%]"))
		//((Real,adhesion,,,"Adhesion coefficient between particles"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),,"Material of particles"))
		, /*ctor*/
			psd.push_back(Vector2r(.01,.2));
			psd.push_back(Vector2r(.02,.4));
			psd.push_back(Vector2r(.04,.8));
			psd.push_back(Vector2r(.05,1.));
			material->density=2500;
			material->cast<FrictMat>().young=1e7;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(Roro);
