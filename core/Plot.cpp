#include<woo/core/Master.hpp>
#include<woo/core/Plot.hpp>

WOO_PLUGIN(core,(Plot));
WOO_CLASS_IMPLEMENTATION(woo_core_Plot_CLASS_DESCRIPTOR);

shared_ptr<Scene> Plot::getScene_py(){ return scene.lock(); }


