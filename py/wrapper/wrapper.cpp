// 2007,2008 © Václav Šmilauer <eudoxos@arcig.cz> 

#include<yade/lib/base/Types.hpp>
#include<sstream>
#include<map>
#include<vector>
#include<unistd.h>
#include<list>
#include<signal.h>

#include<boost/python.hpp>
#include<boost/python/raw_function.hpp>
#include<boost/bind.hpp>
#include<boost/lambda/bind.hpp>
#include<boost/thread/thread.hpp>
#include<boost/filesystem/operations.hpp>
#include<boost/date_time/posix_time/posix_time.hpp>
#include<boost/any.hpp>
#include<boost/python.hpp>
#include<boost/foreach.hpp>
#include<boost/algorithm/string.hpp>
#include<boost/version.hpp>

#include<yade/lib/base/Logging.hpp>
#include<yade/lib/base/CompUtils.hpp>
#include<yade/lib/pyutil/gil.hpp>
#include<yade/lib/pyutil/raw_constructor.hpp>
#include<yade/lib/pyutil/doc_opts.hpp>
#include<yade/core/Omega.hpp>
#include<yade/core/EnergyTracker.hpp>
#include<yade/core/Scene.hpp>
#include<yade/core/Field.hpp>

#if BOOST_VERSION<104800
	// local copy
	#include<boost/math/nonfinite_num_facets.hpp>
#else
	#include<boost/math/special_functions/nonfinite_num_facets.hpp>
#endif
#include<locale>
#include<boost/archive/codecvt_null.hpp>
#include<yade/lib/object/ObjectIO.hpp>


#include<numpy/ndarrayobject.h>


#ifdef YADE_LOG4CXX
	log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("yade.python");
#endif

void termHandlerNormal(int sig){cerr<<"Yade: normal exit."<<endl; raise(SIGTERM);}
void termHandlerError(int sig){cerr<<"Yade: error exit."<<endl; raise(SIGTERM);}

class pyOmega{
	private:
		// can be safely removed now, since pyOmega makes an empty scene in the constructor, if there is none
		void assertScene(){if(!OMEGA.getScene()) throw std::runtime_error("No Scene instance?!"); }
		Omega& OMEGA;
	public:
	pyOmega(): OMEGA(Omega::instance()){
		shared_ptr<Scene> rb=OMEGA.getScene();
		assert(rb);
	};

	double realTime(){ return OMEGA.getRealTime(); }

	bool timingEnabled_get(){return TimingInfo::enabled;}
	void timingEnabled_set(bool enabled){TimingInfo::enabled=enabled;}

	void saveTmpAny(shared_ptr<Object> obj, const string& name, bool quiet){ OMEGA.saveTmp(obj,name,quiet); }
	shared_ptr<Object> loadTmpAny(const string& name){ return OMEGA.loadTmp(name); }

	py::list lsTmp(){ py::list ret; typedef pair<std::string,string> strstr; FOREACH(const strstr& sim,OMEGA.memSavedSimulations){ string mark=sim.first; boost::algorithm::replace_first(mark,":memory:",""); ret.append(mark); } return ret; }

	void tmpToFile(string mark, string filename){
		if(OMEGA.memSavedSimulations.count(":memory:"+mark)==0) throw runtime_error("No memory-saved simulation named "+mark);
		boost::iostreams::filtering_ostream out;
		if(boost::algorithm::ends_with(filename,".bz2")) out.push(boost::iostreams::bzip2_compressor());
		out.push(boost::iostreams::file_sink(filename));
		if(!out.good()) throw runtime_error("Error while opening file `"+filename+"' for writing.");
		LOG_INFO("Saving :memory:"<<mark<<" to "<<filename);
		out<<OMEGA.memSavedSimulations[":memory:"+mark];
	}
	string tmpToString(string mark){
		if(OMEGA.memSavedSimulations.count(":memory:"+mark)==0) throw runtime_error("No memory-saved simulation named "+mark);
		return OMEGA.memSavedSimulations[":memory:"+mark];
	}

	void resetTime(){ OMEGA.getScene()->step=0; OMEGA.getScene()->time=0; }
	//void switchScene(){ std::swap(OMEGA.scene,OMEGA.sceneAnother); }
	shared_ptr<Scene> scene_get(){ return OMEGA.getScene(); }
	void scene_set(const shared_ptr<Scene>& s){ OMEGA.setScene(s); }

	py::list listChildClassesNonrecursive(const string& base){
		py::list ret;
		for(auto& i: Omega::instance().getClassBases()){ if(Omega::instance().isInheritingFrom(i.first,base)) ret.append(i.first); }
		return ret;
	}

	bool isChildClassOf(const string& child, const string& base){
		return (Omega::instance().isInheritingFrom_recursive(child,base));
	}

	py::list plugins_get(){
		py::list ret;
		for(auto& i: Omega::instance().getClassBases()) ret.append(i.first);
		return ret;
	}

	#ifdef YADE_OPENMP
		int numThreads_get(){ return omp_get_max_threads();}
		// void numThreads_set(int n){ int bcn=OMEGA.getScene()->forces.getNumAllocatedThreads(); if(bcn<n) LOG_WARN("ForceContainer has only "<<bcn<<" threads allocated. Changing thread number to on "<<bcn<<" instead of "<<n<<" requested."); omp_set_num_threads(min(n,bcn)); LOG_WARN("BUG: Omega().numThreads=n doesn't work as expected (number of threads is not changed globally). Set env var OMP_NUM_THREADS instead."); }
	#else
		int numThreads_get(){return 1;}
		// void numThreads_set(int n){ LOG_WARN("Yade was compiled without openMP support, changing number of threads will have no effect."); }
	#endif

	#ifdef YADE_OPENCL
		Vector2i defaultClDev_get(){ return OMEGA.defaultClDev; }
		void defaultClDev_set(const Vector2i dev){ OMEGA.defaultClDev=dev; }
	#endif
	
	void disableGdb(){
		signal(SIGSEGV,SIG_DFL);
		signal(SIGABRT,SIG_DFL);
	}
	void exitNoBacktrace(int status=0){
		if(status==0) signal(SIGSEGV,termHandlerNormal); /* unset the handler that runs gdb and prints backtrace */
		else signal(SIGSEGV,termHandlerError);
		// try to clean our mess
		Omega::instance().cleanupTemps();
		// flush all streams (so that in case we crash at exit, unflushed buffers are not lost)
		fflush(NULL);
		// attempt exit
		exit(status);
	}
	std::string tmpFilename(){ return OMEGA.tmpFilename(); }

	vector<string> lsCmap(){ vector<string> ret; for(const CompUtils::Colormap& cm: CompUtils::colormaps) ret.push_back(cm.name); return ret; }
	py::tuple getCmap(){ return py::make_tuple(CompUtils::defaultCmap,CompUtils::colormaps[CompUtils::defaultCmap].name); }
	void setCmap(py::object obj){
		py::extract<int> exInt(obj);
		py::extract<string> exStr(obj);
		py::extract<py::tuple> exTuple(obj);
		if(exInt.check()){
			int i=exInt();
			if(i<0 || i>=(int)CompUtils::colormaps.size()) yade::IndexError(boost::format("Colormap index out of range 0…%d")%(CompUtils::colormaps.size()));
			CompUtils::defaultCmap=i;
			return;
		}
		if(exStr.check()){
			int i=-1; string s(exStr());
			for(const CompUtils::Colormap& cm: CompUtils::colormaps){ i++; if(cm.name==s){ CompUtils::defaultCmap=i; return; } }
			yade::KeyError("No colormap named `"+s+"'.");
		}
		if(exTuple.check() && py::extract<int>(exTuple()[0]).check() && py::extract<string>(exTuple()[1]).check()){
			int i=py::extract<int>(exTuple()[0]); string s=py::extract<string>(exTuple()[1]);
			if(i<0 || i>=(int)CompUtils::colormaps.size()) yade::IndexError(boost::format("Colormap index out of range 0…%d")%(CompUtils::colormaps.size()));
			CompUtils::defaultCmap=i;
			if(CompUtils::colormaps[i].name!=s) LOG_WARN("Given colormap name ignored, does not match index");
			return;
		}
		yade::TypeError("cmap can be specified as int, str or (int,str)");
	}

	#define _DEPREC_ERR(a) void err_##a(){ yade::AttributeError("O." #a " does not exist in tr2 anymore, use O.scene." #a); }
	_DEPREC_ERR(dt);
	_DEPREC_ERR(engines);
	_DEPREC_ERR(cell);
	_DEPREC_ERR(periodic);
	_DEPREC_ERR(trackEnergy);
	_DEPREC_ERR(energy);
	_DEPREC_ERR(tags);
};

BOOST_PYTHON_MODULE(wrapper)
{
	py::scope().attr("__doc__")="Wrapper for c++ internals of yade.";

	YADE_SET_DOCSTRING_OPTS;
# if 0
	py::class_<pyOmega>("Omega")
		.add_property("realtime",&pyOmega::realTime,"Return clock (human world) time the simulation has been running.")

		.def("loadTmpAny",&pyOmega::loadTmpAny,(py::arg("name")=""),"Load any object from named temporary store.")
		.def("saveTmpAny",&pyOmega::saveTmpAny,(py::arg("obj"),py::arg("name")="",py::arg("quiet")=false),"Save any object to named temporary store; *quiet* will supress warning if the name is already used.")
		.def("lsTmp",&pyOmega::lsTmp,"Return list of all memory-saved simulations.")
		.def("tmpToFile",&pyOmega::tmpToFile,(py::arg("mark"),py::arg("fileName")),"Save XML of :yref:`saveTmp<Omega.saveTmp>`'d simulation into *fileName*.")
		.def("tmpToString",&pyOmega::tmpToString,(py::arg("mark")=""),"Return XML of :yref:`saveTmp<Omega.saveTmp>`'d simulation as string.")

		//.def("switchScene",&pyOmega::switchScene,"Switch to alternative simulation (while keeping the old one). Calling the function again switches back to the first one. Note that most variables from the first simulation will still refer to the first simulation even after the switch\n(e.g. b=O.bodies[4]; O.switchScene(); [b still refers to the body in the first simulation here])")
		.def("resetTime",&pyOmega::resetTime,"Reset simulation time: step number, virtual and real time. (Doesn't touch anything else, including timings).")
		.def("plugins",&pyOmega::plugins_get,"Return list of all plugins registered in the class factory.")
		#ifdef YADE_OPENCL
			.add_property("defaultClDev",&pyOmega::defaultClDev_get,&pyOmega::defaultClDev_set,"Default OpenCL platform/device couple (as ints), set internally from the command-line arg.")
		#endif
		.add_property("scene",&pyOmega::scene_get,&pyOmega::scene_set,"Return the current :yref:`scene <Scene>` object. Only set this object carefully!")

		.add_property("cmaps",&pyOmega::lsCmap,"List available colormaps (by name)")
		.add_property("cmap",&pyOmega::getCmap,&pyOmega::setCmap,"Current colormap as (index,name) tuple; set by index or by name alone.")

		// throw on deprecated attributes
		.add_property("dt",&pyOmega::err_dt)
		.add_property("engines",&pyOmega::err_engines)
		.add_property("cell",&pyOmega::err_cell)
		.add_property("periodic",&pyOmega::err_periodic)
		.add_property("trackEnergy",&pyOmega::err_trackEnergy)
		.add_property("energy",&pyOmega::err_energy)
		.add_property("tags",&pyOmega::err_tags)
	
		.def("childClassesNonrecursive",&pyOmega::listChildClassesNonrecursive,"Return list of all classes deriving from given class, as registered in the class factory")
		.def("isChildClassOf",&pyOmega::isChildClassOf,"Tells whether the first class derives from the second one (both given as strings).")
		.add_property("timingEnabled",&pyOmega::timingEnabled_get,&pyOmega::timingEnabled_set,"Globally enable/disable timing services (see documentation of the :yref:`timing module<yade.timing>`).")
		.add_property("numThreads",&pyOmega::numThreads_get /* ,&pyOmega::numThreads_set*/ ,"Get maximum number of threads openMP can use.")

		.def("exitNoBacktrace",&pyOmega::exitNoBacktrace,(py::arg("status")=0),"Disable SEGV handler and exit, optionally with given status number.")
		.def("disableGdb",&pyOmega::disableGdb,"Revert SEGV and ABRT handlers to system defaults.")
		.def("tmpFilename",&pyOmega::tmpFilename,"Return unique name of file in temporary directory which will be deleted when yade exits.")
	;

	// http://numpy.scipy.org/numpydoc/numpy-13.html mentions this must be done in module init, otherwise we will crash
	import_array();

	//py::scope().attr("O")=pyOmega();
#endif
}

