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

#include<yade/lib/base/Math.hpp>
#include<yade/lib/base/Singleton.hpp>
#include<yade/lib/base/Types.hpp>
#include<yade/lib/base/Logging.hpp>
#include<yade/lib/pyutil/except.hpp>

#include<yade/core/Timing.hpp>

#ifndef FOREACH
# define FOREACH BOOST_FOREACH
#endif

class Scene;
namespace yade { class Object; };
using namespace yade;

namespace py=boost::python;

class Omega: public Singleton<Omega>{
	map<string,set<string>> classBases; // FIXME: should store that in ClassFactory ?
	public:

	// pointer to factory function
	typedef std::function<shared_ptr<yade::Object>(void)> FactoryFunc;
	// map class name to function returning instance upcast to shared_ptr<Object>
	std::map<std::string,FactoryFunc> classnameFactoryMap;

	// add entry to classnameFactoryMap
	bool registerClassFactory(const std::string& name, FactoryFunc factory);

	// return instance for given class name
	shared_ptr<yade::Object> factorClass(const std::string& name);

	list<std::pair<string,string>> modulePluginClasses;

	// called automatically, when a plugin is dlopen'd
	void registerPluginClasses(const char* module, const char* fileAndClasses[]);
	// dlopen given plugin
	void loadPlugin(const string& pluginFiles);
	// called from python, with list of plugins to be loaded
	void loadPlugins(const vector<string>& pluginFiles);
	// called from loadPlugins
	// register class from plugin in python
	// store class hierarchy for quick lookup (might be removed in the future)
	void initializePlugins(const vector<std::pair<std::string, std::string> >& dynlibsList); 
	
	shared_ptr<Scene> scene;
	shared_ptr<Scene> sceneAnother; // used for temporarily running different simulation, in Omega().switchscene()

	boost::posix_time::ptime startupLocalTime;

	map<string,string> memSavedSimulations;

	// to avoid accessing simulation when it is being loaded (should avoid crashes with the UI)
	boost::mutex loadingSimulationMutex;
	boost::mutex tmpFileCounterMutex;
	long tmpFileCounter;
	std::string tmpFileDir;

	public:
		void cleanupTemps();
		const map<string,std::set<string>>& getClassBases();

		bool isInheritingFrom(const string& className, const string& baseClassName );
		bool isInheritingFrom_recursive(const string& className, const string& baseClassName );

		string gdbCrashBatch;

		// do not change by hand
		/* Mutex for:
		* 1. GLViewer::paintGL (deffered lock: if fails, no GL painting is done)
		* 2. other threads that wish to manipulate GL
		* 3. Omega when substantial changes to the scene are being made (bodies being deleted, simulation loaded etc) so that GL doesn't access those and crash */
		boost::try_mutex renderMutex;


		/* temporary storage */
		shared_ptr<yade::Object> loadTmp(const string& name);
		void saveTmp(shared_ptr<yade::Object> s, const string& name, bool quiet=false);
		py::list pyLsTmp();
		void pyTmpToFile(const string& mark, const string& filename);
		std::string pyTmpToString(const string& mark);
		/* colormap interface */
		py::list pyLsCmap();
		py::tuple pyGetCmap();
		void pySetCmap(py::object obj);
		/* other static things exposed through Omega */
		bool timingEnabled_get(){ return TimingInfo::enabled;}
		void timingEnabled_set(bool enabled){ TimingInfo::enabled=enabled;}
		#ifdef YADE_OPENMP
			int numThreads_get(){ return omp_get_max_threads();}
		#else
			int numThreads_get(){return 1;}
		#endif
		void pyExitNoBacktrace(int status=0);
		void pyDisableGdb(){
			signal(SIGSEGV,SIG_DFL);
			signal(SIGABRT,SIG_DFL);
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

	Omega();

	FRIEND_SINGLETON(Omega);
	friend class pyOmega;

	#define _DEPREC_ERR(a) void err_##a(){ yade::AttributeError("O." #a " does not exist in tr2 anymore, use O.scene." #a); }
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


	static void pyRegisterClass(){
		py::class_<Omega,boost::noncopyable>("Master",py::no_init)
			.add_property("realtime",&Omega::getRealTime,"Return clock (human world) time the simulation has been running.")
			// tmp storage
			.def("loadTmpAny",&Omega::loadTmp,(py::arg("name")=""),"Load any object from named temporary store.")
			.def("saveTmpAny",&Omega::saveTmp,(py::arg("obj"),py::arg("name")="",py::arg("quiet")=false),"Save any object to named temporary store; *quiet* will supress warning if the name is already used.")
			.def("lsTmp",&Omega::pyLsTmp,"Return list of all memory-saved simulations.")
			.def("tmpToFile",&Omega::pyTmpToFile,(py::arg("mark"),py::arg("fileName")),"Save XML of :ref:`saveTmp`'d simulation into *fileName*.")
			.def("tmpToString",&Omega::pyTmpToString,(py::arg("mark")=""),"Return XML of :yref:`saveTmp<Omega.saveTmp>`'d simulation as string.")

			// .def("plugins",&pyOmega::plugins_get,"Return list of all plugins registered in the class factory.")
			#ifdef YADE_OPENCL
				.def_readwrite("defaultClDev",&Omega::defaultClDev,"Default OpenCL platform/device couple (as ints), set internally from the command-line arg.")
			#endif
			//.def_readwrite("scene",&Omega::scene,"Main Scene (the one which is rendered in the 3d view, for instance).")
			.add_property("scene",&Omega::pyGetScene,&Omega::pySetScene)
			.def("reset",&Omega::pyReset,"Set empty main scene")

			.add_property("cmaps",&Omega::pyLsCmap,"List available colormaps (by name)")
			.add_property("cmap",&Omega::pyGetCmap,&Omega::pySetCmap,"Current colormap as (index,name) tuple; set by index or by name alone.")

			// throw on deprecated attributes
			.add_property("dt",&Omega::err_dt)
			.add_property("engines",&Omega::err_engines)
			.add_property("cell",&Omega::err_cell)
			.add_property("periodic",&Omega::err_periodic)
			.add_property("trackEnergy",&Omega::err_trackEnergy)
			.add_property("energy",&Omega::err_energy)
			.add_property("tags",&Omega::err_tags)
	
			.def("childClassesNonrecursive",&Omega::pyListChildClassesNonrecursive,"Return list of all classes deriving from given class, as registered in the class factory")
			.def("isChildClassOf",&Omega::pyIsChildClassOf,"Tells whether the first class derives from the second one (both given as strings).")

			.add_property("timingEnabled",&Omega::timingEnabled_get,&Omega::timingEnabled_set,"Globally enable/disable timing services (see documentation of the :yref:`timing module<yade.timing>`).")
			.add_property("numThreads",&Omega::numThreads_get,"Get maximum number of threads openMP can use.")

			.def("exitNoBacktrace",&Omega::pyExitNoBacktrace,(py::arg("status")=0),"Disable SEGV handler and exit, optionally with given status number.")
			.def("disableGdb",&Omega::pyDisableGdb,"Revert SEGV and ABRT handlers to system defaults.")
			.def("tmpFilename",&Omega::tmpFilename,"Return unique name of file in temporary directory which will be deleted when yade exits.")
			.add_static_property("instance",py::make_function(&Omega::instance,py::return_value_policy<py::reference_existing_object>()))
		;
	}
};


/*! Function returning class name (as string) for given index and topIndexable (top-level indexable, such as Shape, Material and so on)
This function exists solely for debugging, is quite slow: it has to traverse all classes and ask for inheritance information.
It should be used primarily to convert indices to names in Dispatcher::dictDispatchMatrix?D; since it relies on Omega for RTTI,
this code could not be in Dispatcher itself.
s*/
template<class topIndexable>
std::string Dispatcher_indexToClassName(int idx){
	boost::scoped_ptr<topIndexable> top(new topIndexable);
	std::string topName=top->getClassName();
	for(auto& clss: Omega::instance().getClassBases()){
		if(Omega::instance().isInheritingFrom_recursive(clss.first,topName) || clss.first==topName){
			// create instance, to ask for index
			shared_ptr<topIndexable> inst=dynamic_pointer_cast<topIndexable>(Omega::instance().factorClass(clss.first));
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




