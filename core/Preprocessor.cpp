#include<yade/core/Preprocessor.hpp>
YADE_PLUGIN(core,(Preprocessor));

shared_ptr<Scene> Preprocessor::operator()(){ throw std::logic_error("Preprocessor() called on the base class."); }
