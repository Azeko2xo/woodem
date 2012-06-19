#include<yade/core/Omega.hpp>
#include<yade/core/Scene.hpp>
#include<yade/lib/base/Math.hpp>
#include<yade/lib/multimethods/FunctorWrapper.hpp>
#include<yade/lib/multimethods/Indexable.hpp>
#include<cstdlib>
#include<boost/filesystem/operations.hpp>
#include<boost/filesystem/convenience.hpp>
#include<boost/filesystem/exception.hpp>
#include<boost/algorithm/string.hpp>
#include<boost/thread/mutex.hpp>
#include<boost/version.hpp>
#include<boost/python.hpp>

#include<yade/lib/object/ObjectIO.hpp>


#include<cxxabi.h>

class RenderMutexLock: public boost::mutex::scoped_lock{
	public:
	RenderMutexLock(): boost::mutex::scoped_lock(Omega::instance().renderMutex){/* cerr<<"Lock renderMutex"<<endl; */}
	~RenderMutexLock(){/* cerr<<"Unlock renderMutex"<<endl;*/ }
};

CREATE_LOGGER(Omega);
SINGLETON_SELF(Omega);


Omega::Omega(){
	sceneAnother=shared_ptr<Scene>(new Scene);
	scene=shared_ptr<Scene>(new Scene);
	startupLocalTime=boost::posix_time::microsec_clock::local_time();
	char dirTemplate[]="/tmp/yade-XXXXXX"; tmpFileDir=mkdtemp(dirTemplate); tmpFileCounter=0;
	defaultClDev=Vector2i(-1,-1);
}

void Omega::cleanupTemps(){ boost::filesystem::path tmpPath(tmpFileDir); boost::filesystem::remove_all(tmpPath); }

const map<string,DynlibDescriptor>& Omega::getDynlibsDescriptor(){return dynlibs;}

Real Omega::getRealTime(){ return (boost::posix_time::microsec_clock::local_time()-startupLocalTime).total_milliseconds()/1e3; }
boost::posix_time::time_duration Omega::getRealTime_duration(){return boost::posix_time::microsec_clock::local_time()-startupLocalTime;}


std::string Omega::tmpFilename(){
	assert(!tmpFileDir.empty());
	boost::mutex::scoped_lock lock(tmpFileCounterMutex);
	return tmpFileDir+"/tmp-"+lexical_cast<string>(tmpFileCounter++);
}

const shared_ptr<Scene>& Omega::getScene(){return scene;}
void Omega::setScene(const shared_ptr<Scene>& s){ if(!s) throw std::runtime_error("Scene must not be None."); scene=s; }

/* inheritance (?!!) */
bool Omega::isInheritingFrom(const string& className, const string& baseClassName){
	return (dynlibs[className].baseClasses.find(baseClassName)!=dynlibs[className].baseClasses.end());
}
bool Omega::isInheritingFrom_recursive(const string& className, const string& baseClassName){
	if (dynlibs[className].baseClasses.find(baseClassName)!=dynlibs[className].baseClasses.end()) return true;
	FOREACH(const string& parent,dynlibs[className].baseClasses){
		if(isInheritingFrom_recursive(parent,baseClassName)) return true;
	}
	return false;
}

/* named temporary store */
void Omega::saveTmp(shared_ptr<Object> obj, const string& name, bool quiet){
	if(memSavedSimulations.count(name)>0 && !quiet) LOG_INFO("Overwriting in-memory saved simulation "<<name);
	std::ostringstream oss;
	yade::ObjectIO::save<shared_ptr<Object>,boost::archive::binary_oarchive>(oss,"yade__Serializable",obj);
	memSavedSimulations[name]=oss.str();
}
shared_ptr<Object> Omega::loadTmp(const string& name){
	if(memSavedSimulations.count(name)==0) throw std::runtime_error("No memory-saved simulation "+name);
	std::istringstream iss(memSavedSimulations[name]);
	auto obj=make_shared<Object>();
	yade::ObjectIO::load<shared_ptr<Object>,boost::archive::binary_iarchive>(iss,"yade__Serializable",obj);
	return obj;
}

/* PLUGINS */

void Omega::loadPlugins(vector<string> pluginFiles){
	FOREACH(const string& plugin, pluginFiles){
		LOG_DEBUG("Loading plugin "<<plugin);
		try {
			ClassFactory::instance().load(plugin);
		} catch (std::runtime_error& e){
			const std::string err=e.what();
			if(err.find(": undefined symbol: ")!=std::string::npos){
				size_t pos=err.rfind(":");	assert(pos!=std::string::npos); std::string sym(err,pos+2); //2 removes ": " from the beginning
				int status=0; char* demangled_sym=abi::__cxa_demangle(sym.c_str(),0,0,&status);
				LOG_FATAL(plugin<<": undefined symbol `"<<demangled_sym<<"'"); LOG_FATAL(plugin<<": "<<err); LOG_FATAL("Bailing out.");
			}
			else { LOG_FATAL(plugin<<": "<<err<<" ."); /* leave space to not to confuse c++filt */ LOG_FATAL("Bailing out."); }
			throw;
		}
	}
	list<std::pair<string,string> >& plugins(ClassFactory::instance().modulePluginClasses);
	plugins.sort(); plugins.unique();
	initializePlugins(vector<std::pair<std::string,std::string> >(plugins.begin(),plugins.end()));
}

void Omega::initializePlugins(const vector<std::pair<string,string> >& pluginClasses){	
	LOG_DEBUG("called with "<<pluginClasses.size()<<" plugin classes.");
	// boost::python::object wrapperScope=boost::python::import("yade.wrapper");
	std::map<std::string,py::object> pyModules;
	pyModules["wrapper"]=py::import("yade.wrapper");

	// http://boost.2283326.n4.nabble.com/C-sig-How-to-create-package-structure-in-single-extension-module-td2697292.html
	py::object yadeScope=boost::python::import("yade");

	typedef std::pair<std::string,shared_ptr<Object> > StringObjectPair;
	typedef std::pair<std::string,std::string> StringPair;
	std::list<StringObjectPair> pythonables;
	FOREACH(StringPair moduleName, pluginClasses){
		string module(moduleName.first);
		string name(moduleName.second);
		shared_ptr<Object> obj;
		try {
			LOG_DEBUG("Factoring class "<<name);
			obj=ClassFactory::instance().createShared(name);
			assert(obj);
			// needed for Omega::childClasses
			for(int i=0;i<obj->getBaseClassNumber();i++) dynlibs[name].baseClasses.insert(obj->getBaseClassName(i));
			if(pyModules.find(module)==pyModules.end()){
				try{
					// module existing as file, use it
					pyModules[module]=py::import(("yade."+module).c_str());
				} catch (py::error_already_set& e){
					// import error, synthesize the module
					#if 1
						py::object newModule(py::handle<>(PyModule_New(("yade."+module).c_str())));
						newModule.attr("__file__")="<synthetic>";
						yadeScope.attr(module.c_str())=newModule;
						//pyModules[module]=py::import(("yade."+module).c_str());
						pyModules[module]=newModule;
						// http://stackoverflow.com/questions/11063243/synethsized-submodule-from-a-import-b-ok-vs-import-a-b-error/11063494
						py::extract<py::dict>(py::getattr(py::import("sys"),"modules"))()[("yade."+module).c_str()]=newModule;
						LOG_DEBUG("Synthesized new module yade."<<module);
					#else
						cerr<<"Error importing module yade."<<module<<" for class "<<name;
						boost::python::handle_exception();
						throw;
					#endif
				}
			}
			pythonables.push_back(StringObjectPair(module,obj));
		}
		catch (std::runtime_error& e){
			/* FIXME: this catches all errors! Some of them are not harmful, however:
			 * when a class is not factorable, it is OK to skip it; */	
			 cerr<<"Caught non-critical error for class "<<module<<"."<<name;
		}
	}

	// handle Object specially
	// py::scope _scope(pyModules["core"]);
	// Object().pyRegisterClass(pyModules["core"]);

	/* python classes must be registered such that base classes come before derived ones;
	for now, just loop until we succeed; proper solution will be to build graphs of classes
	and traverse it from the top. It will be done once all classes are pythonable. */
	for(int i=0; i<10 && pythonables.size()>0; i++){
		if(getenv("YADE_DEBUG")) cerr<<endl<<"[[[ Round "<<i<<" ]]]: ";
		std::list<string> done;
		for(std::list<StringObjectPair>::iterator I=pythonables.begin(); I!=pythonables.end(); ){
			const std::string& module=I->first;
			const shared_ptr<Object>& s=I->second;
			const std::string& klass=s->getClassName();
			try{
				if(getenv("YADE_DEBUG")) cerr<<"{{"<<klass<<"}}";
				py::scope _scope(pyModules[module]);
				s->pyRegisterClass();
				std::list<StringObjectPair>::iterator prev=I++;
				pythonables.erase(prev);
			} catch (...){
				if(getenv("YADE_DEBUG")){ cerr<<"["<<klass<<"]"; PyErr_Print(); }
				boost::python::handle_exception();
				I++;
			}
		}
	}
#if 0
	// import all known modules, this should solve crashes which happen at serialization when the module (yade.pre in particular) is not imported by hand first
	for(const auto& m: pyModules){
		if(getenv("YADE_DEBUG")){ cerr<<"import module yade."<<m.first<<endl; }
		py::import(("yade."+m.first).c_str());
	}
#endif
}


