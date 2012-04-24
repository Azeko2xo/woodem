#include<yade/core/Preprocessor.hpp>
#include<yade/core/Scene.hpp>

#include<yade/pkg/dem/FrictMat.hpp>

struct Roro: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("yade.pre.Roro_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",
		((int,cylNum,8,,"Number of cylinders"))
		//((Vector2i,numCyl_range,Vector2i(1,12),Attr::noGui,"range for numCyl"))
		((Real,cylDiameter,.2,,"Diameter of cylinders [m]"))
		((Real,cylLength,.2,,"Length of cylinders [m]"))
		((Real,gap,.05,,"Gap between cylinders [m]"))
		((Real,inclination,0,,"Inclination cylinders [deg]"))
		((Real,angVel,1.,,"Angular velocity of cylinders [rot/sec]"))
		((vector<Vector2r>,psd,/*set in the ctor*/,,"Particle size distribution of generated particles: first value is diameter [mm], second value is cummulative percentage [%]"))
		//((Real,friction,,,"Friction coefficient between particles"))
		//((Real,adhesion,,,"Adhesion coefficient between particles"))
		//((Real,density,3600,,"Density of particles [kg/m³]"))
		//((Real,young,1e6,,"Young's modulus of particles"))
		//((Real,ktDivKn,.2,,"Normal to tangential stiffness ratio"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),,"Material of particles"))
		//((Real,sphRad,.2,,"Mean particle radius"))
		, /*ctor*/
			psd.push_back(Vector2r(.2,.5));
			psd.push_back(Vector2r(.3,1.));
	);
};
REGISTER_SERIALIZABLE(Roro);
