#include<woo/core/Preprocessor.hpp>
#include<woo/core/Master.hpp>
#include<woo/core/Scene.hpp>
#include<boost/algorithm/string.hpp>
WOO_PLUGIN(core,(Preprocessor));

shared_ptr<Scene> Preprocessor::operator()(){ throw std::logic_error("Preprocessor() called on the base class."); }

void Preprocessor::fixWindowsPath(string& s){
	#ifdef _WIN32
		if(s=="/tmp") s=string("c:/temp");
		if(boost::starts_with(s,"/tmp/")) boost::replace_first(s,"/tmp/","c:/temp/");
	#endif
}
