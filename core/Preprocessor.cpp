#include<woo/core/Preprocessor.hpp>
#include<woo/core/Master.hpp>
#include<woo/core/Scene.hpp>
#include<boost/algorithm/string.hpp>

WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_core_Preprocessor_CLASS_BASE_DOC_ATTRS_PY);
WOO_PLUGIN(core,(Preprocessor));
shared_ptr<Scene> Preprocessor::operator()(){ throw std::logic_error("Preprocessor() called on the base class."); }
