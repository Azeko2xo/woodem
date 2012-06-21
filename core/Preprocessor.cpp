#include<woo/core/Preprocessor.hpp>
WOO_PLUGIN(core,(Preprocessor));

shared_ptr<Scene> Preprocessor::operator()(){ throw std::logic_error("Preprocessor() called on the base class."); }
