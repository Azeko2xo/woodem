#include<woo/core/Master.hpp>
#include<woo/core/Plot.hpp>

WOO_PLUGIN(core,(Plot));
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_core_Plot__CLASS_BASE_DOC_ATRRS_PY);

shared_ptr<Scene> Plot::getScene_py(){ return scene.lock(); }


