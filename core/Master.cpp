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

#ifdef __MINGW64__
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include<process.h>
	#include<Windows.h>
	int getpid(){ return _getpid(); }
#else
	#include<unistd.h> // for getpid
#endif

namespace fs=boost::filesystem;

class RenderMutexLock: public boost::mutex::scoped_lock{
	public:
	RenderMutexLock(): boost::mutex::scoped_lock(Master::instance().renderMutex){/* cerr<<"Lock renderMutex"<<endl; */}
	~RenderMutexLock(){/* cerr<<"Unlock renderMutex"<<endl;*/ }
};

CREATE_LOGGER(Master);

#ifdef __MINGW64__
/* make sure that both sets of env vars define variable *name* the same way */
void crtWin32EnvCheck(const char* name){
	char win[1024]; int len=GetEnvironmentVariable(name,win,1024);
	char* crt=getenv(name);
	bool winDef=len>0;
	bool crtDef=(crt!=NULL);
	if(winDef!=crtDef){
		cerr<<"WARN: env var "<<name<<" is "<<(winDef?"":"NOT")<<" defined in win32 but "<<(crtDef?"":"NOT")<<" defined in the CRT."<<endl;
		return;
	}
	// both defined, compare value
	if(winDef){
		string w(win), c(crt);
		if(w!=c) cerr<<"WARN: env var "<<name<<" is \""<<w<<"\" in win32 but \""<<c<<"\" in CRT."<<endl;
	}
}
#endif


Master::Master(){
	LOG_DEBUG_EARLY("Constructing woo::Master.");
	sceneAnother=shared_ptr<Scene>(new Scene);
	scene=shared_ptr<Scene>(new Scene);
	startupLocalTime=boost::posix_time::microsec_clock::local_time();
	#ifdef __MINGW64__
		// check that (some) env vars set from python will work under windows.
		crtWin32EnvCheck("WOO_TEMP");
		crtWin32EnvCheck("OMP_NUM_THREADS");
	#endif
	
	fs::path tmp; // assigned in each block
	if(getenv("WOO_TEMP")){
		// use WOO_TEMP_PREFIX if defined
		// this is used on Windows, where _cxxInternal.pyd must be copied to the temp directory
		// since it is hardlinked (not symlinked) from individual modules, hence must be on the same
		// therefore, the tempdir must be created before _cxxInternal is imported, in python
		// but picked up here
		tmpFileDir=getenv("WOO_TEMP");
		tmp=fs::path(tmpFileDir);
		if(!fs::exists(tmp)) throw std::runtime_error("Provided temp directory WOO_TEMP="+tmpFileDir+" does not exist.");
		LOG_DEBUG_EARLY("Using temp dir"<<getenv("WOO_TEMP"));
	} else {
		tmp=fs::unique_path(fs::temp_directory_path()/"woo-tmp-%%%%%%%%");
		tmpFileDir=tmp.string();
		LOG_DEBUG_EARLY("Creating temp dir "<<tmpFileDir);
		if(!fs::create_directory(tmp)) throw std::runtime_error("Creating temporary directory "+tmpFileDir+" failed.");
	}

	// write pid file
	// this checks, as side-effect, that we have write permissions for the temporary directory
	std::ofstream pidfile;
	pidfile.open((tmp/"pid").string().c_str());
	if(!pidfile.is_open()) throw std::runtime_error("Error opening pidfile "+(tmp/"pid").string()+" for writing.");
	pidfile<<getpid()<<endl;
	if(pidfile.bad()) throw std::runtime_error("Error writing to pidfile "+(tmp/"pid").string()+".");
	pidfile.close();

	tmpFileCounter=0;

	defaultClDev=Vector2i(-1,-1);
}


Master::~Master(){ }

const map<string,set<string>>& Master::getClassBases(){return classBases;}

Real Master::getRealTime(){ return (boost::posix_time::microsec_clock::local_time()-startupLocalTime).total_milliseconds()/1e3; }
boost::posix_time::time_duration Master::getRealTime_duration(){return boost::posix_time::microsec_clock::local_time()-startupLocalTime;}

string Master::getTmpFileDir(){ return tmpFileDir; }

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
		LOG_DEBUG_EARLY("Plugin "<<fileAndClasses[0]<<", class "<<module<<"."<<fileAndClasses[i]);	
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
					// PyErr_Print shows error and clears error indicator
					if(getenv("WOO_DEBUG")) PyErr_Print();
					// this only clears the the error indicator
					else PyErr_Clear(); 
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
	for(int i=0; i<11 && pythonables.size()>0; i++){
		if(i==10) throw std::runtime_error("Too many attempts to register python classes. Run again with WOO_DEBUG=1 to get better diagnostics.");
		LOG_DEBUG_EARLY_FRAGMENT(endl<<"[[[ Round "<<i<<" ]]]: ");
		std::list<string> done;
		for(std::list<StringObjectPair>::iterator I=pythonables.begin(); I!=pythonables.end(); ){
			const std::string& module=I->first;
			const shared_ptr<Object>& s=I->second;
			const std::string& klass=s->getClassName();
			try{
				LOG_DEBUG_EARLY_FRAGMENT("{{"<<klass<<"}}");
				py::scope _scope(pyModules[module]);
				s->pyRegisterClass();
				std::list<StringObjectPair>::iterator prev=I++;
				pythonables.erase(prev);
			} catch (...){
				LOG_DEBUG_EARLY("["<<klass<<"]");
				if(getenv("WOO_DEBUG")) PyErr_Print();
				else PyErr_Clear();
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
	// flush all streams (so that in case we crash at exit, unflushed buffers are not lost)
	fflush(NULL);
	// attempt exit
	exit(status);
}

shared_ptr<woo::Object> Master::pyGetScene(){ return getScene(); }
void Master::pySetScene(const shared_ptr<Object>& s){
	if(!s) woo::ValueError("Argument is None");
	if(!s->isA<Scene>()) woo::TypeError("Argument is not a Scene instance");
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

int Master::numThreads_get(){
	#ifdef WOO_OPENMP
		return omp_get_max_threads();
	#else
		return 1;
	#endif
}

void Master::numThreads_set(int i){
	#ifdef WOO_OPENMP
		omp_set_num_threads(i);
		if(i!=omp_get_max_threads()) throw std::runtime_error("woo.core.Master: numThreads set to "+to_string(i)+" but is "+to_string(i)+" (are you setting numThreads inside parallel section?)");
	#else
		if(i==1) return;
		else LOG_WARN("woo.core.Master: compiled without OpenMP support, setting numThreads has no effect.");
	#endif
}
