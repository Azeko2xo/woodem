#include<yade/core/Preprocessor.hpp>
#include<yade/core/Scene.hpp>
struct Roro: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("yade.pre.Roro_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	YADE_CLASS_BASE_DOC_ATTRS(Roro,Preprocessor,"Preprocessor for the Rollenrost simulation.",
		((int,numCyl,8,,"Number of cylinders"))
		((Vector2i,numCyl_range,Vector2i(1,12),Attr::noGui,"range for numCyl"))
		((Real,sphRad,.2,,"Mean particle radius"))
	);
};
REGISTER_SERIALIZABLE(Roro);
