#pragma once

#include<boost/date_time/posix_time/posix_time.hpp>
#include<fstream>
#include<set>
#include<list>
#include<time.h>
#include<boost/thread/thread.hpp>
#include<iostream>
#include<boost/python.hpp>
#include<boost/foreach.hpp>
#include<boost/smart_ptr/scoped_ptr.hpp>
#include<boost/preprocessor/cat.hpp>
#include<boost/preprocessor/stringize.hpp>

#include<woo/lib/base/Math.hpp>
#include<woo/lib/base/Singleton.hpp>
#include<woo/lib/base/Types.hpp>
#include<woo/lib/base/Logging.hpp>
#include<woo/lib/pyutil/except.hpp>
#include<woo/lib/object/Object.hpp>

#include<woo/core/Timing.hpp>

#ifndef FOREACH
# define FOREACH BOOST_FOREACH
#endif

struct Scene;
//namespace woo { class Object; };
using namespace woo;

namespace py=boost::python;

class Master: public Singleton<Master>{
	map<string,set<string>> classBases;
	public:

	// pointer to factory function
	typedef std::function<shared_ptr<woo::Object>(void)> FactoryFunc;
	// map class name to function returning instance upcast to shared_ptr<Object>
	std::map<std::string,FactoryFunc> classnameFactoryMap;
	// add entry to classnameFactoryMap
	bool registerClassFactory(const std::string& name, FactoryFunc factory);
	// return instance for given class name
	shared_ptr<woo::Object> factorClass(const std::string& name);

	list<std::pair<string,string>> modulePluginClasses;

	// called automatically, when a plugin is dlopen'd
	void registerPluginClasses(const char* module, const char* fileAndClasses[]);
	// register class from plugin in python
	// store class hierarchy for quick lookup (might be removed in the future)
	void pyRegisterAllClasses(); 

	// compiled python modules
	list<string> compiledPyModules;
	py::list pyCompiledPyModules(void){ py::list ret; for(const auto& s: compiledPyModules) ret.append(s); return ret; }
	// full module name should be given: woo.*
	void registerCompiledPyModule(const char* mod){ compiledPyModules.push_back(mod); }
	#define WOO_PYTHON_MODULE(mod) namespace{ WOO__ATTRIBUTE__CONSTRUCTOR void BOOST_PP_CAT(_registerThisCompiledPyModule_,__COUNTER__) (void){ if(getenv("WOO_DEBUG")) cerr<<"Registering python module "<<BOOST_PP_STRINGIZE(mod)<<endl; Master::instance().registerCompiledPyModule("woo." #mod); } }

	
	shared_ptr<Scene> scene;
	shared_ptr<Scene> sceneAnother; // used for temporarily running different simulation, in Master().switchscene()

	boost::posix_time::ptime startupLocalTime;

	map<string,string> memSavedSimulations;

	// to avoid accessing simulation when it is being loaded (should avoid crashes with the UI)
	boost::mutex loadingSimulationMutex;
	boost::mutex tmpFileCounterMutex;
	long tmpFileCounter;
	bool termFlag;
	std::string tmpFileDir;

	public:
		bool terminateAll(){ return termFlag; }
		void cleanupTemps();
		const map<string,std::set<string>>& getClassBases();

		bool isInheritingFrom(const string& className, const string& baseClassName );
		bool isInheritingFrom_recursive(const string& className, const string& baseClassName );

		string gdbCrashBatch;

		// do not change by hand
		/* Mutex for:
		* 1. GLViewer::paintGL (deffered lock: if fails, no GL painting is done)
		* 2. other threads that wish to manipulate GL
		* 3. Master when substantial changes to the scene are being made (bodies being deleted, simulation loaded etc) so that GL doesn't access those and crash */
		boost::try_mutex renderMutex;


		/* temporary storage */
		shared_ptr<woo::Object> loadTmp(const string& name);
		void saveTmp(shared_ptr<woo::Object> s, const string& name, bool quiet=false);
		void rmTmp(const string& name);
		py::list pyLsTmp();
		void pyTmpToFile(const string& mark, const string& filename);
		std::string pyTmpToString(const string& mark);
		/* colormap interface */
		py::list pyLsCmap();
		py::tuple pyGetCmap();
		void pySetCmap(py::object obj);
		/* other static things exposed through Master */
		bool timingEnabled_get(){ return TimingInfo::enabled;}
		void timingEnabled_set(bool enabled){ TimingInfo::enabled=enabled;}
		#ifdef WOO_OPENMP
			int numThreads_get(){ return omp_get_max_threads();}
		#else
			int numThreads_get(){return 1;}
		#endif
		void pyExitNoBacktrace(int status=0);
		void pyDisableGdb(){
			// disable for native Windows builds
			#ifndef __MINGW64__ 
				signal(SIGSEGV,SIG_DFL);
				signal(SIGABRT,SIG_DFL);
			#endif
		}
		py::list pyListChildClassesNonrecursive(const string& base);
		bool pyIsChildClassOf(const string& child, const string& base);


		// avoid using Scene in function signatures, since then Scene.hpp would have to be included
		// because boost::python needs full type, not just forwards
		// that would lead to circular dependencies
		shared_ptr<Object> pyGetScene();
		void pySetScene(const shared_ptr<Object>& s); 



		const shared_ptr<Scene>& getScene();
		void setScene(const shared_ptr<Scene>& s);
		//! Return unique temporary filename. May be deleted by the user; if not, will be deleted at shutdown.
		string tmpFilename();
		Real getRealTime();
		boost::posix_time::time_duration getRealTime_duration();

		// configuration directory used for logging config and possibly other things
		std::string confDir;
		// default OpenCL device passed from the command-line
		Vector2i defaultClDev;

	DECLARE_LOGGER;

	Master();
	~Master(){ cleanupTemps(); }

	FRIEND_SINGLETON(Master);

	#define _DEPREC_ERR(a) void err_##a(){ woo::AttributeError("O." #a " does not exist in tr2 anymore, use O.scene." #a); }
		_DEPREC_ERR(dt);
		_DEPREC_ERR(engines);
		_DEPREC_ERR(cell);
		_DEPREC_ERR(periodic);
		_DEPREC_ERR(trackEnergy);
		_DEPREC_ERR(energy);
		_DEPREC_ERR(tags);
	#undef _DEPREC_ERR

	static py::object pyGetInstance();
	void pyReset();
	py::list pyPlugins();


	static void pyRegisterClass(){
		py::class_<Master,boost::noncopyable>("Master",py::no_init)
			.add_property("realtime",&Master::getRealTime,"Return clock (human world) time the simulation has been running.")
			// tmp storage
			.def("loadTmpAny",&Master::loadTmp,(py::arg("name")=""),"Load any object from named temporary store.")
			.def("saveTmpAny",&Master::saveTmp,(py::arg("obj"),py::arg("name")="",py::arg("quiet")=false),"Save any object to named temporary store; *quiet* will supress warning if the name is already used.")
			.def("lsTmp",&Master::pyLsTmp,"Return list of all memory-saved simulations.")
			.def("rmTmp",&Master::rmTmp,py::arg("name"),"Remove memory-saved simulation.")
			.def("tmpToFile",&Master::pyTmpToFile,(py::arg("mark"),py::arg("fileName")),"Save XML of :ref:`saveTmp`'d simulation into *fileName*.")
			.def("tmpToString",&Master::pyTmpToString,(py::arg("mark")=""),"Return XML of :ref:`saveTmp<Master.saveTmp>`'d simulation as string.")

			.def("plugins",&Master::pyPlugins,"Return list of all plugins registered in the class factory.")
			#ifdef WOO_OPENCL
				.def_readwrite("defaultClDev",&Master::defaultClDev,"Default OpenCL platform/device couple (as ints), set internally from the command-line arg.")
			#endif
			.add_property("scene",&Master::pyGetScene,&Master::pySetScene)
			.def("reset",&Master::pyReset,"Set empty main scene")

			.add_property("cmaps",&Master::pyLsCmap,"List available colormaps (by name)")
			.add_property("cmap",&Master::pyGetCmap,&Master::pySetCmap,"Current colormap as (index,name) tuple; set by index or by name alone.")

			// throw on deprecated attributes
			.add_property("dt",&Master::err_dt)
			.add_property("engines",&Master::err_engines)
			.add_property("cell",&Master::err_cell)
			.add_property("periodic",&Master::err_periodic)
			.add_property("trackEnergy",&Master::err_trackEnergy)
			.add_property("energy",&Master::err_energy)
			.add_property("tags",&Master::err_tags)
	
			.def("childClassesNonrecursive",&Master::pyListChildClassesNonrecursive,"Return list of all classes deriving from given class, as registered in the class factory")
			.def("isChildClassOf",&Master::pyIsChildClassOf,"Tells whether the first class derives from the second one (both given as strings).")

			.add_property("timingEnabled",&Master::timingEnabled_get,&Master::timingEnabled_set,"Globally enable/disable timing services (see documentation of the :ref:`timing module<woo.timing>`).")
			.add_property("numThreads",&Master::numThreads_get,"Get maximum number of threads openMP can use.")
			.add_property("compiledPyModules",&Master::pyCompiledPyModules) // we might not use to-python converters, since _customConverters have not yet been imported

			.def("exitNoBacktrace",&Master::pyExitNoBacktrace,(py::arg("status")=0),"Disable SEGV handler and exit, optionally with given status number.")
			.def("disableGdb",&Master::pyDisableGdb,"Revert SEGV and ABRT handlers to system defaults.")
			.def("tmpFilename",&Master::tmpFilename,"Return unique name of file in temporary directory which will be deleted when woo exits.")
			.add_static_property("instance",py::make_function(&Master::instance,py::return_value_policy<py::reference_existing_object>()))
		;
	}
};


/*! Function returning class name (as string) for given index and topIndexable (top-level indexable, such as Shape, Material and so on)
This function exists solely for debugging, is quite slow: it has to traverse all classes and ask for inheritance information.
It should be used primarily to convert indices to names in Dispatcher::dictDispatchMatrix?D; since it relies on Master for RTTI,
this code could not be in Dispatcher itself.
s*/
template<class topIndexable>
std::string Dispatcher_indexToClassName(int idx){
	boost::scoped_ptr<topIndexable> top(new topIndexable);
	std::string topName=top->getClassName();
	for(auto& clss: Master::instance().getClassBases()){
		if(Master::instance().isInheritingFrom_recursive(clss.first,topName) || clss.first==topName){
			// create instance, to ask for index
			shared_ptr<topIndexable> inst=dynamic_pointer_cast<topIndexable>(Master::instance().factorClass(clss.first));
			assert(inst);
			if(inst->getClassIndex()<0 && inst->getClassName()!=top->getClassName()){
				throw logic_error("Class "+inst->getClassName()+" didn't use REGISTER_CLASS_INDEX("+inst->getClassName()+","+top->getClassName()+") and/or forgot to call createIndex() in the ctor. [[ Please fix that! ]]");
			}
			if(inst->getClassIndex()==idx) return clss.first;
		}
	}
	throw std::runtime_error("No class with index "+boost::lexical_cast<string>(idx)+" found (top-level indexable is "+topName+")");
}


//! Return class index of given indexable
template<typename TopIndexable>
int Indexable_getClassIndex(const shared_ptr<TopIndexable> i){return i->getClassIndex();}

//! Return sequence (hierarchy) of class indices of given indexable; optionally convert to names
template<typename TopIndexable>
py::list Indexable_getClassIndices(const shared_ptr<TopIndexable> i, bool convertToNames){
	int depth=1; py::list ret; int idx0=i->getClassIndex();
	if(convertToNames) ret.append(Dispatcher_indexToClassName<TopIndexable>(idx0));
	else ret.append(idx0);
	if(idx0<0) return ret; // don't continue and call getBaseClassIndex(), since we are at the top already
	while(true){
		int idx=i->getBaseClassIndex(depth++);
		if(convertToNames) ret.append(Dispatcher_indexToClassName<TopIndexable>(idx));
		else ret.append(idx);
		if(idx<0) return ret;
	}
}




