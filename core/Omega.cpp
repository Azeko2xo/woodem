#include<yade/core/Omega.hpp>
#include<yade/core/Scene.hpp>
#include<yade/core/BgThread.hpp>
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

#include<yade/lib/serialization/ObjectIO.hpp>


#include<cxxabi.h>

class RenderMutexLock: public boost::mutex::scoped_lock{
	public:
	RenderMutexLock(): boost::mutex::scoped_lock(Omega::instance().renderMutex){/* cerr<<"Lock renderMutex"<<endl; */}
	~RenderMutexLock(){/* cerr<<"Unlock renderMutex"<<endl;*/ }
};

CREATE_LOGGER(Omega);
SINGLETON_SELF(Omega);


Omega::Omega(): simulationLoop(&simulationFlow){
	sceneAnother=shared_ptr<Scene>(new Scene);
	scene=shared_ptr<Scene>(new Scene);
	startupLocalTime=boost::posix_time::microsec_clock::local_time();
	char dirTemplate[]="/tmp/yade-XXXXXX"; tmpFileDir=mkdtemp(dirTemplate); tmpFileCounter=0;
	defaultClDev=Vector2i(-1,-1);
}

void Omega::reset(){ stop(); scene=shared_ptr<Scene>(new Scene); }
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

void Omega::stop(){ LOG_DEBUG(""); if(simulationLoop.looping())simulationLoop.stop(); }
void Omega::step(){ simulationLoop.spawnSingleAction(); }
void Omega::run(){ if (!simulationLoop.looping()) simulationLoop.start(); }
void Omega::pause(){ if(simulationLoop.looping()) simulationLoop.stop(); }
bool Omega::isRunning(){ return simulationLoop.looping(); }

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


/* IO */
void Omega::loadSimulation(const string& f, bool quiet){
	bool isMem=boost::algorithm::starts_with(f,":memory:");
	if(!isMem && !boost::filesystem::exists(f)) throw runtime_error("Simulation file to load doesn't exist: "+f);
	if(isMem && memSavedSimulations.count(f)==0) throw runtime_error("Cannot load nonexistent memory-saved simulation "+f);
	
	simulationLoop.workerThrew=false;  // this is invalidated

	if(!quiet) LOG_INFO("Loading file "+f);
	{
		stop(); // stop current simulation if running
		scene=shared_ptr<Scene>(new Scene);
		RenderMutexLock lock;
		if(isMem){
			std::istringstream iss(memSavedSimulations[f]);
			yade::ObjectIO::load<decltype(scene),boost::archive::binary_iarchive>(iss,"scene",scene);
		} else {
			yade::ObjectIO::load(f,"scene",scene);
		}
	}
	if(scene->getClassName()!="Scene") throw logic_error("Wrong file format (scene is not a Scene!?) in "+f);
	scene->lastSave=f;
	startupLocalTime=boost::posix_time::microsec_clock::local_time();
	if(!quiet) LOG_DEBUG("Simulation loaded");
}


void Omega::saveSimulation(const string& f, bool quiet){
	if(f.size()==0) throw runtime_error("f of file to save has zero length.");
	if(!quiet) LOG_INFO("Saving file "<<f);
	scene->lastSave=f;
	if(boost::algorithm::starts_with(f,":memory:")){
		if(memSavedSimulations.count(f)>0 && !quiet) LOG_INFO("Overwriting in-memory saved simulation "<<f);
		std::ostringstream oss;
		yade::ObjectIO::save<decltype(scene),boost::archive::binary_oarchive>(oss,"scene",scene);
		memSavedSimulations[f]=oss.str();
	}
	else {
		// handles automatically the XML/binary distinction as well as gz/bz2 compression
		yade::ObjectIO::save(f,"scene",scene);
	}
}

void Omega::saveTmp(const shared_ptr<Scene>& _scene, const string& _slot, bool quiet){
	if(!quiet) LOG_INFO("Saving into memory slot "<<_slot);
	string slot=":memory:"+_slot;
	_scene->lastSave=slot;
	if(memSavedSimulations.count(slot)>0 && !quiet) LOG_INFO("Overwriting in-memory saved simulation "<<_slot);
	std::ostringstream oss;
	yade::ObjectIO::save<decltype(_scene),boost::archive::binary_oarchive>(oss,"scene",_scene);
	memSavedSimulations[slot]=oss.str();
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

	typedef std::pair<std::string,shared_ptr<Serializable> > StringSerializablePair;
	typedef std::pair<std::string,std::string> StringPair;
	std::list<StringSerializablePair> pythonables;
	FOREACH(StringPair moduleName, pluginClasses){
		string module(moduleName.first);
		string name(moduleName.second);
		shared_ptr<Factorable> f;
		try {
			LOG_DEBUG("Factoring class "<<name);
			f=ClassFactory::instance().createShared(name);
			for(int i=0;i<f->getBaseClassNumber();i++) dynlibs[name].baseClasses.insert(f->getBaseClassName(i));
			shared_ptr<Serializable> ser(dynamic_pointer_cast<Serializable>(f));
			if(ser){
				if(pyModules.find(module)==pyModules.end()){
					#if 0
						// create new module here!
						py::handle<> newModule(PyModule_New(module.c_str()));
						yadeScope.attr(module.c_str())=newModule;
						//pyModules[module]=py::import(("yade."+module).c_str());
						pyModules[module]=py::object(newModule);
						pyModules[module].attr("__file__")="<synthetic>";
						cerr<<"Synthesized new module yade."<<module<<endl;
					#endif
					try{
						pyModules[module]=py::import(("yade."+module).c_str());
					} catch (...){
						cerr<<"Error importing module yade."<<module<<" for class "<<name;
						boost::python::handle_exception();
						throw;
					}
				}
				pythonables.push_back(StringSerializablePair(module,static_pointer_cast<Serializable>(f)));
			}
		}
		catch (std::runtime_error& e){
			/* FIXME: this catches all errors! Some of them are not harmful, however:
			 * when a class is not factorable, it is OK to skip it; */	
			 cerr<<"Caught non-critical error for class "<<module<<"."<<name;
		}
	}

	// handle Serializable specially
	// Serializable().pyRegisterClass(pyModules["core"]);

	/* python classes must be registered such that base classes come before derived ones;
	for now, just loop until we succeed; proper solution will be to build graphs of classes
	and traverse it from the top. It will be done once all classes are pythonable. */
	for(int i=0; i<10 && pythonables.size()>0; i++){
		if(getenv("YADE_DEBUG")) cerr<<endl<<"[[[ Round "<<i<<" ]]]: ";
		std::list<string> done;
		for(std::list<StringSerializablePair>::iterator I=pythonables.begin(); I!=pythonables.end(); ){
			const std::string& module=I->first;
			const shared_ptr<Serializable>& s=I->second;
			const std::string& klass=s->getClassName();
			try{
				if(getenv("YADE_DEBUG")) cerr<<"{{"<<klass<<"}}";
				s->pyRegisterClass(pyModules[module]);
				std::list<StringSerializablePair>::iterator prev=I++;
				pythonables.erase(prev);
			} catch (...){
				if(getenv("YADE_DEBUG")){ cerr<<"["<<klass<<"]"; PyErr_Print(); }
				boost::python::handle_exception();
				I++;
			}
		}
	}

}


