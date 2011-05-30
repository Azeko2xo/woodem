#pragma once

//#include<yade/core/Body.hpp>
#include<yade/core/Cell.hpp>
//#include<yade/core/BodyContainer.hpp>
#include<yade/core/Engine.hpp>
//#include<yade/core/Material.hpp>
#include<yade/core/DisplayParameters.hpp>
//#include<yade/core/ForceContainer.hpp>
//#include<yade/core/InteractionContainer.hpp>
#include<yade/core/EnergyTracker.hpp>
// #include<yade/core/Field.hpp>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255 
#endif


class Bound;
class Field;
#ifdef YADE_OPENGL
	class Renderer;
#endif

class Scene: public Serializable{
	public:
		// initialize tags (author, date, time)
		void fillDefaultTags();
		// advance by one iteration by running all engines
		void moveToNextTimeStep();
		#ifdef YADE_OPENGL
			shared_ptr<Renderer> renderer;
		#endif
		void postLoad(Scene&);

		// keep track of created labels; delete those which are no longer active and so on
		// std::set<std::string> pyLabels;

		std::vector<shared_ptr<Engine> > pyEnginesGet();
		void pyEnginesSet(const std::vector<shared_ptr<Engine> >&);
		
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

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Scene,Serializable,"Object comprising the whole simulation.",
		((Real,dt,1e-8,,"Current timestep for integration."))
		((long,step,0,Attr::readonly,"Current step number"))
		((bool,subStepping,false,,"Whether we currently advance by one engine in every step (rather than by single run through all engines)."))
		((int,subStep,-1,Attr::readonly,"Number of sub-step; not to be changed directly. -1 means to run loop prologue (cell integration), 0…n-1 runs respective engines (n is number of engines), n runs epilogue (increment step number and time."))
		((Real,time,0,Attr::readonly,"Simulation time (virtual time) [s]"))
		((long,stopAtStep,0,,"Iteration after which to stop the simulation."))

		((bool,isPeriodic,false,/*exposed as "periodic" in python */ Attr::hidden,"Whether periodic boundary conditions are active."))
		((bool,trackEnergy,false,,"Whether energies are being tracked."))

		((Vector3r,loHint,Vector3r(-1,-1,-1),,"Lower corner, for rendering purposes"))
		((Vector3r,hiHint,Vector3r(1,1,1),,"Upper corner, for rendering purposes"))

		((bool,runInternalConsistencyChecks,true,Attr::hidden,"Run internal consistency check, right before the very first simulation step."))

		((int,selection,-1,,"Id of particle selected by the user"))

		((StrStrMap,tags,,Attr::hidden,"Arbitrary key=value associations (tags like mp3 tags: author, date, version, description etc.)"))

		((vector<shared_ptr<Engine> >,engines,,(Attr::hidden|Attr::triggerPostLoad),"Engines sequence in the simulation."))
		((vector<shared_ptr<Engine> >,_nextEngines,,Attr::hidden,"Engines to be used from the next step on; is returned transparently by O.engines if in the middle of the loop (controlled by subStep>=0)."))
		((shared_ptr<EnergyTracker>,energy,new EnergyTracker,Attr::readonly,"Energy values, if energy tracking is enabled."))
		((vector<shared_ptr<Field> >,fields,,Attr::triggerPostLoad,"Defined simulation fields."))
		((shared_ptr<Cell>,cell,new Cell,Attr::hidden,"Information on periodicity; only should be used if Scene::isPeriodic."))
		((vector<shared_ptr<DisplayParameters> >,dispParams,,Attr::hidden,"'hash maps' of display parameters (since yade::serialization had no support for maps, emulate it via vector of strings in format key=value)"))
		((std::string,lastSave,,Attr::readonly,"Name under which the simulation was saved for the last time."))

		// ((shared_ptr<Bound>,bound,,Attr::hidden,"Bounding box of the scene (only used for rendering and initialized if needed)."))

		,
		/*ctor*/ fillDefaultTags();
		, /* py */
		.add_property("tags",&Scene::pyGetTags,"Arbitrary key=value associations (tags like mp3 tags: author, date, version, description etc.")
		.add_property("cell",&Scene::pyGetCell,"Periodic space configuration (is None for aperiodic scene); set :yref:`Scene.periodic` to enable/disable periodicity")
		.def_readwrite("periodic",&Scene::isPeriodic,"Set whether the scene is periodic or not")
		.add_property("engines",&Scene::pyEnginesGet,&Scene::pyEnginesSet,"Engine sequence in the simulation")
		.def_readonly("_currEngines",&Scene::engines,"Current engines, debugging only")
		.def_readonly("_nextEngines",&Scene::_nextEngines,"Current engines, debugging only")
		
		;
		
		// define nested class
		boost::python::scope foo(_classObj);
		boost::python::class_<Scene::pyTagsProxy>("TagsProxy",py::init<pyTagsProxy>()).def("__getitem__",&pyTagsProxy::getItem).def("__setitem__",&pyTagsProxy::setItem).def("__delitem__",&pyTagsProxy::delItem).def("has_key",&pyTagsProxy::has_key).def("__contains__",&pyTagsProxy::has_key).def("keys",&pyTagsProxy::keys);
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Scene);

