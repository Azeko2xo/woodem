#include<woo/lib/pyutil/gil.hpp>
void pyRunString(const std::string& cmd){
	GilLock lock; PyRun_SimpleString(cmd.c_str());
};

