#pragma once

#include<yade/core/Cell.hpp>
#include<yade/core/Engine.hpp>
#include<yade/core/DisplayParameters.hpp>
#include<yade/core/EnergyTracker.hpp>
#include<yade/core/Preprocessor.hpp>

#ifdef YADE_OPENCL
	#define __CL_ENABLE_EXCEPTIONS
	#include<CL/cl.hpp>
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255 
#endif

class Bound;
class Field;
class ScalarRange;

#ifdef YADE_OPENGL
	class Renderer;
#endif

class Scene: public Object{
		boost::mutex runMutex;
		bool runningFlag;
		// this will be std::atomic<bool>
		// once libstdc++ headers are accepted by clang++
		bool stopFlag; 
		bool stopFlagSet(){ boost::mutex::scoped_lock l(runMutex); return stopFlag; }
		shared_ptr<std::exception> except;
	public:
		// interface for python,
		void pyRun(long steps=-1, bool wait=false);
		void pyStop();         
		void pyOne();         
		void pyWait();         
		bool running(); 
		void backgroundLoop();

		// initialize tags (author, date, time)
		void fillDefaultTags();
		// advance by one iteration by running all engines
		void doOneStep();

		#ifdef YADE_OPENGL
			shared_ptr<Renderer> renderer;
		#endif
		void postLoad(Scene&);

		#ifdef YADE_OPENCL
			shared_ptr<cl::Platform> platform;
			shared_ptr<cl::Device> device;
			shared_ptr<cl::Context> context;
			shared_ptr<cl::CommandQueue> queue;
		#endif

		// keep track of created labels; delete those which are no longer active and so on
		// std::set<std::string> pyLabels;

		std::vector<shared_ptr<Engine> > pyEnginesGet();
		void pyEnginesSet(const std::vector<shared_ptr<Engine> >&);
		shared_ptr<ScalarRange> getRange(const std::string& l) const;
		
		struct pyTagsProxy{
			Scene* scene;
			pyTagsProxy(Scene* _scene): scene(_scene){};
			std::string getItem(const std::string& key);
			void setItem(const std::string& key, const std::string& value);
			void delItem(const std::string& key);
			boost::python::list keys();
			bool has_key(const std::string& key);
		};
		pyTagsProxy pyGetTags(){ return pyTagsProxy(this); }
		shared_ptr<Cell> pyGetCell(){ return (isPeriodic?cell:shared_ptr<Cell>()); }

		typedef std::map<std::string,std::string> StrStrMap;

		void ensureCl(); // calls initCL or throws exception if compiled without OpenCL
		#ifdef YADE_OPENCL
			void initCl(); // initialize OpenCL using clDev
		#endif

		// override Object::boostSave, to set lastSave correctly
		void boostSave(const string& out);
		void saveTmp(const string& slot, bool quiet=true);

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Scene,Object,"Object comprising the whole simulation.",
		((Real,dt,1e-8,AttrTrait<>().timeUnit(),"Current timestep for integration."))
		((long,step,0,AttrTrait<Attr::readonly>(),"Current step number"))
		((bool,subStepping,false,,"Whether we currently advance by one engine in every step (rather than by single run through all engines)."))
		((int,subStep,-1,AttrTrait<Attr::readonly>(),"Number of sub-step; not to be changed directly. -1 means to run loop prologue (cell integration), 0…n-1 runs respective engines (n is number of engines), n runs epilogue (increment step number and time."))
		((Real,time,0,AttrTrait<>().readonly().timeUnit(),"Simulation time (virtual time) [s]"))
		((long,stopAtStep,0,,"Iteration after which to stop the simulation."))

		((bool,isPeriodic,false,/*exposed as "periodic" in python */AttrTrait<Attr::hidden>(),"Whether periodic boundary conditions are active."))
		((bool,trackEnergy,false,,"Whether energies are being tracked."))

		((Vector2i,clDev,Vector2i(-1,-1),AttrTrait<Attr::triggerPostLoad>(),"OpenCL device to be used; if (-1,-1) (default), no OpenCL device will be initialized until requested. Saved simulations should thus always use the same device when re-loaded."))
		#ifdef YADE_OPENCL
			((Vector2i,_clDev,Vector2i(-1,-1),AttrTrait<Attr::readonly|Attr::noSave>(),"OpenCL device which is really initialized (to detect whether clDev was changed manually to avoid spurious re-initializations from postLoad"))
		#endif

		((Vector3r,loHint,Vector3r(-1,-1,-1),,"Lower corner, for rendering purposes"))
		((Vector3r,hiHint,Vector3r(1,1,1),,"Upper corner, for rendering purposes"))

		((bool,runInternalConsistencyChecks,true,AttrTrait<Attr::hidden>(),"Run internal consistency check, right before the very first simulation step."))

		((int,selection,-1,,"Id of particle selected by the user"))

		((StrStrMap,tags,,AttrTrait<Attr::hidden>(),"Arbitrary key=value associations (tags like mp3 tags: author, date, version, description etc.)"))

		((vector<shared_ptr<Engine>>,engines,,AttrTrait<Attr::hidden|Attr::triggerPostLoad>(),"Engines sequence in the simulation."))
		((vector<shared_ptr<Engine>>,_nextEngines,,AttrTrait<Attr::hidden>(),"Engines to be used from the next step on; is returned transparently by O.engines if in the middle of the loop (controlled by subStep>=0)."))
		((shared_ptr<EnergyTracker>,energy,new EnergyTracker,AttrTrait<Attr::readonly>(),"Energy values, if energy tracking is enabled."))
		((vector<shared_ptr<Field>>,fields,,AttrTrait<Attr::triggerPostLoad>(),"Defined simulation fields."))
		((shared_ptr<Cell>,cell,new Cell,AttrTrait<Attr::hidden>(),"Information on periodicity; only should be used if Scene::isPeriodic."))
		((vector<shared_ptr<DisplayParameters>>,dispParams,,AttrTrait<Attr::hidden>(),"'hash maps' of display parameters (since yade::serialization had no support for maps, emulate it via vector of strings in format key=value)"))
		((std::string,lastSave,,AttrTrait<>().noGui(),"Name under which the simulation was saved for the last time; used for reloading the simulation. Updated automatically, don't change."))

		#if YADE_OPENGL
			((vector<shared_ptr<ScalarRange>>,ranges,,,"Scalar ranges to be rendered on the display as colormaps"))
		#endif
		((vector<shared_ptr<Object>>,any,,,"Storage for arbitrary Objects; meant for storing and loading static objects like Gl1_* functors to restore their parameters when scene is loaded."))
		((shared_ptr<Preprocessor>,pre,,AttrTrait<>().noGui(),"Preprocessor used for generating this simulation; to be only used in user scripts to query preprocessing parameters, not in c++ code."))

		// ((shared_ptr<Bound>,bound,,AttrTrait<Attr::hidden>(),"Bounding box of the scene (only used for rendering and initialized if needed)."))

		,
		/*ctor*/ fillDefaultTags(); runningFlag=false;
		, /* py */
		.add_property("tags",&Scene::pyGetTags,"Arbitrary key=value associations (tags like mp3 tags: author, date, version, description etc.")
		.add_property("cell",&Scene::pyGetCell,"Periodic space configuration (is None for aperiodic scene); set :yref:`Scene.periodic` to enable/disable periodicity")
		.def_readwrite("periodic",&Scene::isPeriodic,"Set whether the scene is periodic or not")
		.add_property("engines",&Scene::pyEnginesGet,&Scene::pyEnginesSet,"Engine sequence in the simulation")
		.add_property("_currEngines",py::make_getter(&Scene::engines,py::return_value_policy<py::return_by_value>()),"Current engines, debugging only")
		.add_property("_nextEngines",py::make_getter(&Scene::_nextEngines,py::return_value_policy<py::return_by_value>()),"Next engines, debugging only")
		#ifdef YADE_OPENGL
			.def("getRange",&Scene::getRange,"Retrieve a *ScalarRange* object by its label")
		#endif
		#ifdef YADE_OPENCL
			.def("ensureCl",&Scene::ensureCl,"[for debugging] Initialize the OpenCL subsystem (this is done by engines using OpenCL, but trying to do so in advance might catch errors earlier)")
		#endif
		.def("saveTmp",&Scene::saveTmp,(py::arg("slot")="",py::arg("quiet")=false),"Save into a temporary slot inside Omega (loadable with O.loadTmp)")

		.def("run",&Scene::pyRun,(py::args("steps")=-1,py::args("wait")=false))
		.def("stop",&Scene::pyStop)
		.def("one",&Scene::pyOne)
		.def("wait",&Scene::pyWait)
		.add_property("running",&Scene::running)
		;
		// define nested class
		boost::python::scope foo(_classObj);
		boost::python::class_<Scene::pyTagsProxy>("TagsProxy",py::init<pyTagsProxy>()).def("__getitem__",&pyTagsProxy::getItem).def("__setitem__",&pyTagsProxy::setItem).def("__delitem__",&pyTagsProxy::delItem).def("has_key",&pyTagsProxy::has_key).def("__contains__",&pyTagsProxy::has_key).def("keys",&pyTagsProxy::keys)
		);
	DECLARE_LOGGER;
};

REGISTER_SERIALIZABLE(Scene);
