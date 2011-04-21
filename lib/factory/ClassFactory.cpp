#include<yade/lib/factory/ClassFactory.hpp>
#include<boost/algorithm/string/regex.hpp>
#include<dlfcn.h>
CREATE_LOGGER(ClassFactory);
SINGLETON_SELF(ClassFactory);
class Factorable;
using namespace std;

bool ClassFactory::registerFactorable(const std::string& name, CreateSharedFactorableFnPtr createShared){
	return map.insert(FactorableCreatorsMap::value_type(name,createShared)).second;
}

shared_ptr<Factorable> ClassFactory::createShared(const std::string& name){
	FactorableCreatorsMap::const_iterator i=map.find(name);
	if(i==map.end()) throw std::runtime_error(("ClassFactory: Class "+name+" not known.").c_str());
	return (i->second)();
}

void ClassFactory::load(const string& lib){
	if (lib.empty()) throw std::runtime_error(__FILE__ ": got empty library name to load.");
	dlopen(lib.c_str(),RTLD_GLOBAL | RTLD_NOW);
 	char* error=dlerror();
	if(error) throw std::runtime_error((__FILE__ ": error loading plugin "+lib+" (dlopen): "+error).c_str());
}

void ClassFactory::registerPluginClasses(const char* fileAndClasses[]){
	assert(fileAndClasses[0]!=NULL); // must be file name
	// only filename given, no classes names explicitly
	if(fileAndClasses[1]==NULL){
		/* strip leading path (if any; using / as path separator) and strip one suffix (if any) to get the contained class name */
		string heldClass=boost::algorithm::replace_regex_copy(string(fileAndClasses[0]),boost::regex("^(.*/)?(.*?)(\\.[^.]*)?$"),string("\\2"));
		#ifdef YADE_DEBUG
			if(getenv("YADE_DEBUG")) cerr<<__FILE__<<":"<<__LINE__<<": Plugin "<<fileAndClasses[0]<<", class "<<heldClass<<" (deduced)"<<endl;
		#endif
		pluginClasses.push_back(heldClass); // last item with everything up to last / take off and .suffix strip
	}
	else {
		for(int i=1; fileAndClasses[i]!=NULL; i++){
			#ifdef YADE_DEBUG
				if(getenv("YADE_DEBUG")) cerr<<__FILE__<<":"<<__LINE__<<": Plugin "<<fileAndClasses[0]<<", class "<<fileAndClasses[i]<<endl;	
			#endif
			pluginClasses.push_back(fileAndClasses[i]);
		}
	}
}

