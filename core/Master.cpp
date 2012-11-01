#include<woo/core/Master.hpp>
#include<woo/core/Scene.hpp>
#include<woo/lib/base/Math.hpp>
#include<woo/lib/multimethods/FunctorWrapper.hpp>
#include<woo/lib/multimethods/Indexable.hpp>
#include<cstdlib>
#include<boost/filesystem/operations.hpp>
#include<boost/filesystem/convenience.hpp>
#include<boost/filesystem/exception.hpp>
#include<boost/algorithm/string.hpp>
#include<boost/thread/mutex.hpp>
#include<boost/version.hpp>
#include<boost/python.hpp>


#include<woo/lib/object/ObjectIO.hpp>
#include<woo/core/Timing.hpp>

class RenderMutexLock: public boost::mutex::scoped_lock{
	public:
	RenderMutexLock(): boost::mutex::scoped_lock(Master::instance().renderMutex){/* cerr<<"Lock renderMutex"<<endl; */}
	~RenderMutexLock(){/* cerr<<"Unlock renderMutex"<<endl;*/ }
};

CREATE_LOGGER(Master);


Master::Master(){
	if(getenv("WOO_DEBUG")) cerr<<"Constructing woo::Master."<<endl;
	sceneAnother=shared_ptr<Scene>(new Scene);
	scene=shared_ptr<Scene>(new Scene);
	startupLocalTime=boost::posix_time::microsec_clock::local_time();

	auto tmp=boost::filesystem::unique_path(boost::filesystem::temp_directory_path()/"woo-%%%%%%%%");
	tmpFileDir=tmp.string();
	if(!boost::filesystem::create_directory(tmp)) throw std::runtime_error("Creating temporary directory "+tmpFileDir+" failed.");
	tmpFileCounter=0;

	defaultClDev=Vector2i(-1,-1);
}

void Master::cleanupTemps(){
	if(getenv("WOO_DEBUG")) cerr<<"Cleaning "<<tmpFileDir<<endl;
	boost::filesystem::path tmpPath(tmpFileDir); boost::filesystem::remove_all(tmpPath);
}

const map<string,set<string>>& Master::getClassBases(){return classBases;}

Real Master::getRealTime(){ return (boost::posix_time::microsec_clock::local_time()-startupLocalTime).total_milliseconds()/1e3; }
boost::posix_time::time_duration Master::getRealTime_duration(){return boost::posix_time::microsec_clock::local_time()-startupLocalTime;}


std::string Master::tmpFilename(){
	assert(!tmpFileDir.empty());
	boost::mutex::scoped_lock lock(tmpFileCounterMutex);
	return tmpFileDir+"/tmp-"+lexical_cast<string>(tmpFileCounter++);
}

const shared_ptr<Scene>& Master::getScene(){return scene;}
void Master::setScene(const shared_ptr<Scene>& s){ if(!s) throw std::runtime_error("Scene must not be None."); scene=s; }

/* inheritance (?!!) */
bool Master::isInheritingFrom(const string& className, const string& baseClassName){
	return (classBases[className].find(baseClassName)!=classBases[className].end());
}
bool Master::isInheritingFrom_recursive(const string& className, const string& baseClassName){
	if(classBases[className].find(baseClassName)!=classBases[className].end()) return true;
	FOREACH(const string& parent,classBases[className]){
		if(isInheritingFrom_recursive(parent,baseClassName)) return true;
	}
	return false;
}

/* named temporary store */
void Master::saveTmp(shared_ptr<Object> obj, const string& name, bool quiet){
	if(memSavedSimulations.count(name)>0 && !quiet){  LOG_INFO("Overwriting in-memory saved simulation "<<name); }
	std::ostringstream oss;
	woo::ObjectIO::save<shared_ptr<Object>,boost::archive::binary_oarchive>(oss,"woo__Serializable",obj);
	memSavedSimulations[name]=oss.str();
}
shared_ptr<Object> Master::loadTmp(const string& name){
	if(memSavedSimulations.count(name)==0) throw std::runtime_error("No memory-saved simulation "+name);
	std::istringstream iss(memSavedSimulations[name]);
	auto obj=make_shared<Object>();
	woo::ObjectIO::load<shared_ptr<Object>,boost::archive::binary_iarchive>(iss,"woo__Serializable",obj);
	return obj;
}

/* PLUGINS */
// registerFactorable
bool Master::registerClassFactory(const std::string& name, FactoryFunc factory){
	classnameFactoryMap[name]=factory;
	return true;
}

shared_ptr<Object> Master::factorClass(const std::string& name){
	auto I=classnameFactoryMap.find(name);
	if(I==classnameFactoryMap.end()) throw std::runtime_error("Master.factorClassByName: Class '"+name+"' not known.");
	return (I->second)();
}


void Master::registerPluginClasses(const char* module, const char* fileAndClasses[]){
	assert(fileAndClasses[0]!=NULL); // must be file name
	for(int i=1; fileAndClasses[i]!=NULL; i++){
		#ifdef WOO_DEBUG
			if(getenv("WOO_DEBUG")) cerr<<__FILE__<<":"<<__LINE__<<": Plugin "<<fileAndClasses[0]<<", class "<<module<<"."<<fileAndClasses[i]<<endl;	
		#endif
		modulePluginClasses.push_back({module,fileAndClasses[i]});
	}
}

void Master::pyRegisterAllClasses(){	
	// register support classes
	py::scope core(py::import("woo.core"));
		this->pyRegisterClass();
		woo::ClassTrait::pyRegisterClass();
		woo::AttrTraitBase::pyRegisterClass();
		woo::TimingDeltas::pyRegisterClass();
		Object().pyRegisterClass(); // virtual method, therefore cannot be static

	LOG_DEBUG("called with "<<modulePluginClasses.size()<<" module+class pairs.");
	std::map<std::string,py::object> pyModules;
	// http://boost.2283326.n4.nabble.com/C-sig-How-to-create-package-structure-in-single-extension-module-td2697292.html
	py::object wooScope=boost::python::import("woo");

	typedef std::pair<std::string,shared_ptr<Object> > StringObjectPair;
	typedef std::pair<std::string,std::string> StringPair;
	std::list<StringObjectPair> pythonables;
	FOREACH(StringPair moduleName, modulePluginClasses){
		string module(moduleName.first);
		string name(moduleName.second);
		shared_ptr<Object> obj;
		try {
			LOG_DEBUG("Factoring class "<<name);
			obj=factorClass(name);
			assert(obj);
			// needed for Master::childClasses
			for(int i=0;i<obj->getBaseClassNumber();i++) classBases[name].insert(obj->getBaseClassName(i));
			if(pyModules.find(module)==pyModules.end()){
				try{
					// module existing as file, use it
					pyModules[module]=py::import(("woo."+module).c_str());
				} catch (py::error_already_set& e){
					if(getenv("WOO_DEBUG")) PyErr_Print(); //boost::python::handle_exception();
					py::object newModule(py::handle<>(PyModule_New(("woo."+module).c_str())));
					newModule.attr("__file__")="<synthetic>";
					wooScope.attr(module.c_str())=newModule;
					//pyModules[module]=py::import(("woo."+module).c_str());
					pyModules[module]=newModule;
					// http://stackoverflow.com/questions/11063243/synethsized-submodule-from-a-import-b-ok-vs-import-a-b-error/11063494
					py::extract<py::dict>(py::getattr(py::import("sys"),"modules"))()[("woo."+module).c_str()]=newModule;
					LOG_INFO("Synthesized new module woo."<<module);
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
		if(getenv("WOO_DEBUG")) cerr<<endl<<"[[[ Round "<<i<<" ]]]: ";
		std::list<string> done;
		for(std::list<StringObjectPair>::iterator I=pythonables.begin(); I!=pythonables.end(); ){
			const std::string& module=I->first;
			const shared_ptr<Object>& s=I->second;
			const std::string& klass=s->getClassName();
			try{
				if(getenv("WOO_DEBUG")) cerr<<"{{"<<klass<<"}}";
				py::scope _scope(pyModules[module]);
				s->pyRegisterClass();
				std::list<StringObjectPair>::iterator prev=I++;
				pythonables.erase(prev);
			} catch (...){
				if(getenv("WOO_DEBUG")){ cerr<<"["<<klass<<"]"; PyErr_Print(); }
				boost::python::handle_exception();
				I++;
			}
		}
	}
}


py::list Master::pyListChildClassesNonrecursive(const string& base){
	py::list ret;
	for(auto& i: getClassBases()){ if(isInheritingFrom(i.first,base)) ret.append(i.first); }
	return ret;
}

bool Master::pyIsChildClassOf(const string& child, const string& base){
	return (isInheritingFrom_recursive(child,base));
}

void Master::rmTmp(const string& name){
	if(memSavedSimulations.count(name)==0) throw runtime_error("No memory-saved object named "+name);
	memSavedSimulations.erase(name);
}

py::list Master::pyLsTmp(){
	py::list ret;
	for(const auto& i: memSavedSimulations) ret.append(i.first);
	return ret;
}

void Master::pyTmpToFile(const string& mark, const string& filename){
	if(memSavedSimulations.count(mark)==0) throw runtime_error("No memory-saved simulation named "+mark);
	boost::iostreams::filtering_ostream out;
	if(boost::algorithm::ends_with(filename,".bz2")) out.push(boost::iostreams::bzip2_compressor());
	out.push(boost::iostreams::file_sink(filename));
	if(!out.good()) throw runtime_error("Error while opening file `"+filename+"' for writing.");
	LOG_INFO("Saving memory-stored '"<<mark<<"' to file "<<filename);
	out<<memSavedSimulations[mark];
}

std::string Master::pyTmpToString(const string& mark){
	if(memSavedSimulations.count(mark)==0) throw runtime_error("No memory-saved simulation named "+mark);
	return memSavedSimulations[mark];
}


py::list Master::pyLsCmap(){ py::list ret; for(const CompUtils::Colormap& cm: CompUtils::colormaps) ret.append(cm.name); return ret; }
py::tuple Master::pyGetCmap(){ return py::make_tuple(CompUtils::defaultCmap,CompUtils::colormaps[CompUtils::defaultCmap].name); } 
void Master::pySetCmap(py::object obj){
	py::extract<int> exInt(obj);
	py::extract<string> exStr(obj);
	py::extract<py::tuple> exTuple(obj);
	if(exInt.check()){
		int i=exInt();
		if(i<0 || i>=(int)CompUtils::colormaps.size()) woo::IndexError(boost::format("Colormap index out of range 0…%d")%(CompUtils::colormaps.size()));
		CompUtils::defaultCmap=i;
		return;
	}
	if(exStr.check()){
		int i=-1; string s(exStr());
		for(const CompUtils::Colormap& cm: CompUtils::colormaps){ i++; if(cm.name==s){ CompUtils::defaultCmap=i; return; } }
		woo::KeyError("No colormap named `"+s+"'.");
	}
	if(exTuple.check() && py::extract<int>(exTuple()[0]).check() && py::extract<string>(exTuple()[1]).check()){
		int i=py::extract<int>(exTuple()[0]); string s=py::extract<string>(exTuple()[1]);
		if(i<0 || i>=(int)CompUtils::colormaps.size()) woo::IndexError(boost::format("Colormap index out of range 0…%d")%(CompUtils::colormaps.size()));
		CompUtils::defaultCmap=i;
		if(CompUtils::colormaps[i].name!=s) LOG_WARN("Given colormap name ignored, does not match index");
		return;
	}
	woo::TypeError("cmap can be specified as int, str or (int,str)");
}

// only used below with POSIX signal handlers
#ifndef __MINGW64__
	void termHandlerNormal(int sig){cerr<<"Woo: normal exit."<<endl; raise(SIGTERM);}
	void termHandlerError(int sig){cerr<<"Woo: error exit."<<endl; raise(SIGTERM);}
#endif

void Master::pyExitNoBacktrace(int status){
	#ifndef __MINGW64__
		if(status==0) signal(SIGSEGV,termHandlerNormal); /* unset the handler that runs gdb and prints backtrace */
		else signal(SIGSEGV,termHandlerError);
	#endif
	// try to clean our mess
	//
	// cleanupTemps(); :: called from the destructor now
	//
	// flush all streams (so that in case we crash at exit, unflushed buffers are not lost)
	fflush(NULL);
	// attempt exit
	exit(status);
}

shared_ptr<woo::Object> Master::pyGetScene(){ return getScene(); }
void Master::pySetScene(const shared_ptr<Object>& s){
	if(!s) woo::ValueError("Argument is None");
	if(!dynamic_pointer_cast<Scene>(s)) woo::TypeError("Argument is not a Scene instance");
	setScene(static_pointer_cast<Scene>(s));
}


py::object Master::pyGetInstance(){
	//return py::object(py::ref(Master::getInstance()));
	return py::object();
}

void Master::pyReset(){
	getScene()->pyStop();
	setScene(make_shared<Scene>());
}

py::list Master::pyPlugins(){
	py::list ret;
	for(auto& i: getClassBases()) ret.append(i.first);
	return ret;
}
