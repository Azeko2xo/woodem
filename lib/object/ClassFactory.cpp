#include<yade/lib/object/ClassFactory.hpp>
#include<boost/algorithm/string/regex.hpp>
#include<dlfcn.h>
CREATE_LOGGER(ClassFactory);
SINGLETON_SELF(ClassFactory);

bool ClassFactory::registerFactorable(const std::string& name, CreateSharedFnPtr createShared){
	return map.insert(factorableCreatorsMap::value_type(name,createShared)).second;
}

shared_ptr<Object> ClassFactory::createShared(const std::string& name){
	factorableCreatorsMap::const_iterator i=map.find(name);
	if(i==map.end()) throw std::runtime_error(("ClassFactory: Class "+name+" not known.").c_str());
	return (i->second)();
}

void ClassFactory::load(const string& lib){
	if (lib.empty()) throw std::runtime_error(__FILE__ ": got empty library name to load.");
	dlopen(lib.c_str(),RTLD_GLOBAL | RTLD_NOW);
 	char* error=dlerror();
	if(error) throw std::runtime_error((__FILE__ ": error loading plugin "+lib+" (dlopen): "+error).c_str());
}

void ClassFactory::registerPluginClasses(const char* module, const char* fileAndClasses[]){
	assert(fileAndClasses[0]!=NULL); // must be file name
	for(int i=1; fileAndClasses[i]!=NULL; i++){
		#ifdef YADE_DEBUG
			if(getenv("YADE_DEBUG")) cerr<<__FILE__<<":"<<__LINE__<<": Plugin "<<fileAndClasses[0]<<", class "<<module<<"."<<fileAndClasses[i]<<endl;	
		#endif
		modulePluginClasses.push_back(std::pair<std::string,std::string>(std::string(module),std::string(fileAndClasses[i])));
	}
}

