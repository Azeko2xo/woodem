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
#include<yade/lib/serialization/ObjectIO.hpp>
#include<yade/extra/boost_python_len.hpp>

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

	void mapLabeledEntitiesToVariables(){
	};

	long iter(){ return OMEGA.getScene()->step;}
	int subStep(){ return OMEGA.getScene()->subStep; }
	bool subStepping_get(){ return OMEGA.getScene()->subStepping; }
	void subStepping_set(bool val){ OMEGA.getScene()->subStepping=val; }

	double time(){return OMEGA.getScene()->time;}
	double realTime(){ return OMEGA.getRealTime(); }
	//double dt_get(){return OMEGA.getScene()->dt;}
	//void dt_set(double dt){
	//	Scene* scene=OMEGA.getScene().get();
	//	scene->dt=dt;
	//	}
	long stopAtIter_get(){return OMEGA.getScene()->stopAtStep; }
	void stopAtIter_set(long s){OMEGA.getScene()->stopAtStep=s; }


	bool timingEnabled_get(){return TimingInfo::enabled;}
	void timingEnabled_set(bool enabled){TimingInfo::enabled=enabled;}

	void run(long int numIter=-1,bool doWait=false){
		Scene* scene=OMEGA.getScene().get();
		if(numIter>0) scene->stopAtStep=scene->step+numIter;
		OMEGA.run();
		// timespec t1,t2; t1.tv_sec=0; t1.tv_nsec=40000000; /* 40 ms */
		// while(!OMEGA.isRunning()) nanosleep(&t1,&t2); // wait till we start, so that calling wait() immediately afterwards doesn't return immediately
		LOG_DEBUG("RUN"<<((scene->stopAtStep-scene->step)>0?string(" ("+lexical_cast<string>(scene->stopAtStep-scene->step)+" to go)"):string(""))<<"!");
		if(doWait) wait();
	}
	void pause(){Py_BEGIN_ALLOW_THREADS; OMEGA.pause(); Py_END_ALLOW_THREADS; LOG_DEBUG("PAUSE!");}
	void step() { if(OMEGA.isRunning()) throw runtime_error("Called O.step() while simulation is running."); OMEGA.getScene()->moveToNextTimeStep(); /* LOG_DEBUG("STEP!"); run(1); wait(); */ }
	void wait(){
		if(OMEGA.isRunning()){LOG_DEBUG("WAIT!");} else return;
		timespec t1,t2; t1.tv_sec=0; t1.tv_nsec=40000000; /* 40 ms */ Py_BEGIN_ALLOW_THREADS; while(OMEGA.isRunning()) nanosleep(&t1,&t2); Py_END_ALLOW_THREADS;
		if(!OMEGA.simulationLoop.workerThrew) return;
		LOG_ERROR("Simulation error encountered."); OMEGA.simulationLoop.workerThrew=false; throw OMEGA.simulationLoop.workerException;
	}
	bool isRunning(){ return OMEGA.isRunning(); }
	py::object get_filename(){ string f(OMEGA.getScene()->lastSave); if(f.size()>0) return py::object(f); return py::object();}
	void load(std::string fileName,bool quiet=false) {
		Py_BEGIN_ALLOW_THREADS; OMEGA.stop(); Py_END_ALLOW_THREADS; 
		OMEGA.loadSimulation(fileName,quiet);
		mapLabeledEntitiesToVariables();
	}
	void reload(bool quiet=false){
		load(OMEGA.getScene()->lastSave,quiet);
	}
	void saveTmp(string mark="", bool quiet=false){
		assertScene();
		OMEGA.getScene()->saveTmp(mark,quiet);
	}
	void loadTmp(string mark="", bool quiet=false){ load(":memory:"+mark,quiet);}
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

	void reset(){Py_BEGIN_ALLOW_THREADS; OMEGA.reset(); Py_END_ALLOW_THREADS; }
	// void resetThisScene(){Py_BEGIN_ALLOW_THREADS; OMEGA.stop(); Py_END_ALLOW_THREADS; OMEGA.resetScene(); }
	void resetTime(){ OMEGA.getScene()->step=0; OMEGA.getScene()->time=0; }
	void switchScene(){ std::swap(OMEGA.scene,OMEGA.sceneAnother); }
	shared_ptr<Scene> scene_get(){ return OMEGA.getScene(); }
	void scene_set(const shared_ptr<Scene>& s){ OMEGA.setScene(s); }

	void save(std::string fileName,bool quiet=false){
		assertScene();
		OMEGA.getScene()->boostSave(fileName);
	}
	
	vector<shared_ptr<Engine> > engines_get(void){assertScene(); Scene* scene=OMEGA.getScene().get(); return scene->_nextEngines.empty()?scene->engines:scene->_nextEngines;}
	void engines_set(const vector<shared_ptr<Engine> >& egs){
		assertScene(); Scene* scene=OMEGA.getScene().get();
		if(scene->subStep<0) scene->engines=egs; // not inside the engine loop right now, ok to update directly
		else scene->_nextEngines=egs; // inside the engine loop, update _nextEngines; O.engines picks that up automatically, and Scene::moveToNextTimestep will put them in place of engines at the start of the next loop
		mapLabeledEntitiesToVariables();
	}
	// raw access to engines/_nextEngines, for debugging
	vector<shared_ptr<Engine> > currEngines_get(){ return OMEGA.getScene()->engines; }
	vector<shared_ptr<Engine> > nextEngines_get(){ return OMEGA.getScene()->_nextEngines; }

	//shared_ptr<Field> field_get(){ return OMEGA.getScene()->field; }
	//void field_set(shared_ptr<Field> f){ OMEGA.getScene()->field=f; }
	
	py::list listChildClassesNonrecursive(const string& base){
		py::list ret;
		for(map<string,DynlibDescriptor>::const_iterator di=Omega::instance().getDynlibsDescriptor().begin();di!=Omega::instance().getDynlibsDescriptor().end();++di) if (Omega::instance().isInheritingFrom((*di).first,base)) ret.append(di->first);
		return ret;
	}

	bool isChildClassOf(const string& child, const string& base){
		return (Omega::instance().isInheritingFrom_recursive(child,base));
	}

	py::list plugins_get(){
		const map<string,DynlibDescriptor>& plugins=Omega::instance().getDynlibsDescriptor();
		std::pair<string,DynlibDescriptor> p; py::list ret;
		FOREACH(p, plugins) ret.append(p.first);
		return ret;
	}

	// pyTags tags_get(void){assertScene(); return pyTags(OMEGA.getScene());}

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
	
	// shared_ptr<Cell> cell_get(){ if(OMEGA.getScene()->isPeriodic) return OMEGA.getScene()->cell; return shared_ptr<Cell>(); }

	//shared_ptr<EnergyTracker> energy_get(){ return OMEGA.getScene()->energy; }
	//bool trackEnergy_get(void){ return OMEGA.getScene()->trackEnergy; }
//	void trackEnergy_set(bool e){ OMEGA.getScene()->trackEnergy=e; }

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

	py::class_<pyOmega>("Omega")
		.add_property("realtime",&pyOmega::realTime,"Return clock (human world) time the simulation has been running.")
		.def("load",&pyOmega::load,(py::arg("file"),py::arg("quiet")=false),"Load simulation from file.")
		.def("reload",&pyOmega::reload,(py::arg("quiet")=false),"Reload current simulation")
		.def("save",&pyOmega::save,(py::arg("file"),py::arg("quiet")=false),"Save current simulation to file (should be .xml or .xml.bz2)")
		.def("loadTmp",&pyOmega::loadTmp,(py::arg("mark")="",py::arg("quiet")=false),"Load simulation previously stored in memory by saveTmp. *mark* optionally distinguishes multiple saved simulations")
		.def("saveTmp",&pyOmega::saveTmp,(py::arg("mark")="",py::arg("quiet")=false),"Save simulation to memory (disappears at shutdown), can be loaded later with loadTmp. *mark* optionally distinguishes different memory-saved simulations.")
		.def("lsTmp",&pyOmega::lsTmp,"Return list of all memory-saved simulations.")
		.def("tmpToFile",&pyOmega::tmpToFile,(py::arg("mark"),py::arg("fileName")),"Save XML of :yref:`saveTmp<Omega.saveTmp>`'d simulation into *fileName*.")
		.def("tmpToString",&pyOmega::tmpToString,(py::arg("mark")=""),"Return XML of :yref:`saveTmp<Omega.saveTmp>`'d simulation as string.")

		.def("run",&pyOmega::run,(py::arg("nSteps")=-1,py::arg("wait")=false),"Run the simulation. *nSteps* how many steps to run, then stop (if positive); *wait* will cause not returning to python until simulation will have stopped.")
		.def("pause",&pyOmega::pause,"Stop simulation execution. (May be called from within the loop, and it will stop after the current step).")
		.def("step",&pyOmega::step,"Advance the simulation by one step. Returns after the step will have finished.")
		.def("wait",&pyOmega::wait,"Don't return until the simulation will have been paused. (Returns immediately if not running).")
		.add_property("running",&pyOmega::isRunning,"Whether background thread is currently running a simulation.")
		.def("reset",&pyOmega::reset,"Reset simulations completely (including another scene!).")
		.def("switchScene",&pyOmega::switchScene,"Switch to alternative simulation (while keeping the old one). Calling the function again switches back to the first one. Note that most variables from the first simulation will still refer to the first simulation even after the switch\n(e.g. b=O.bodies[4]; O.switchScene(); [b still refers to the body in the first simulation here])")
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
//////////////////////////////////////////////////////////////
///////////// proxyless wrappers 
	// wrapped as AttrTrait in python
	yade::AttrTraitBase::pyRegisterClass();
	Serializable().pyRegisterClass();

	py::class_<TimingDeltas, shared_ptr<TimingDeltas>, boost::noncopyable >("TimingDeltas").add_property("data",&TimingDeltas::pyData,"Get timing data as list of tuples (label, execTime[nsec], execCount) (one tuple per checkpoint)").def("reset",&TimingDeltas::reset,"Reset timing information");



	py::scope().attr("O")=pyOmega();
}

