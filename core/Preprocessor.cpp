#include<yade/core/Preprocessor.hpp>
YADE_PLUGIN(core,(Preprocessor)
	#if 0
		(PyPreprocessor)
	#endif
);

shared_ptr<Scene> Preprocessor::operator()(){ throw std::logic_error("Preprocessor() called on the base class."); }
