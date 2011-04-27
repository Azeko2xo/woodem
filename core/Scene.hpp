#pragma once

//#include<yade/core/Body.hpp>
#include<yade/core/Cell.hpp>
//#include<yade/core/BodyContainer.hpp>
#include<yade/core/Engine.hpp>
//#include<yade/core/Material.hpp>
//#include<yade/core/DisplayParameters.hpp>
//#include<yade/core/ForceContainer.hpp>
//#include<yade/core/InteractionContainer.hpp>
#include<yade/core/EnergyTracker.hpp>
#include<yade/core/Field.hpp>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255 
#endif


class Bound;
#ifdef YADE_OPENGL
	class OpenGLRenderer;
#endif

class Scene: public Serializable{
	public:
		//! Adds material to Scene::materials. It also sets id of the material accordingly and returns it.
		// int addMaterial(shared_ptr<Material> m){ materials.push_back(m); m->id=(int)materials.size()-1; return m->id; }
		//! Checks that type of Body::state satisfies Material::stateTypeOk. Throws runtime_error if not. (Is called from BoundDispatcher the first time it runs)
		// void checkStateTypes();
		//! update our bound; used directly instead of a BoundFunctor, since we don't derive from Body anymore
		// void updateBound();

		// neither serialized, nor accessible from python (at least not directly)
		// ForceContainer forces;

		// initialize tags (author, date, time)
		void fillDefaultTags();

		// advance by one iteration by running all engines
		void moveToNextTimeStep();

		// shared_ptr<Engine> engineByName(const string& s);

		#ifdef YADE_OPENGL
			shared_ptr<OpenGLRenderer> renderer;
		#endif

		void postLoad(Scene&);

		// bits for Scene::flags
		enum { LOCAL_COORDS=1, COMPRESSION_NEGATIVE=2 }; /* add powers of 2 as needed */


	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Scene,Serializable,"Object comprising the whole simulation.",
		((Real,dt,1e-8,,"Current timestep for integration."))
		((long,step,0,Attr::readonly,"Current step number"))
		((bool,subStepping,false,,"Whether we currently advance by one engine in every step (rather than by single run through all engines)."))
		((int,subStep,-1,Attr::readonly,"Number of sub-step; not to be changed directly. -1 means to run loop prologue (cell integration), 0…n-1 runs respective engines (n is number of engines), n runs epilogue (increment step number and time."))
		((Real,time,0,Attr::readonly,"Simulation time (virtual time) [s]"))
		((long,stopAtStep,0,,"Iteration after which to stop the simulation."))

		((bool,isPeriodic,false,Attr::readonly,"Whether periodic boundary conditions are active."))
		((bool,trackEnergy,false,Attr::readonly,"Whether energies are being traced."))

		((bool,runInternalConsistencyChecks,true,Attr::hidden,"Run internal consistency check, right before the very first simulation step."))

		// ((Body::id_t,selectedBody,-1,,"Id of body that is selected by the user"))

		((list<string>,tags,,,"Arbitrary key=value associations (tags like mp3 tags: author, date, version, description etc.)"))

		((vector<shared_ptr<Engine> >,engines,,Attr::hidden,"Engines sequence in the simulation."))
		((vector<shared_ptr<Engine> >,_nextEngines,,Attr::hidden,"Engines to be used from the next step on; is returned transparently by O.engines if in the middle of the loop (controlled by subStep>=0)."))
		((shared_ptr<EnergyTracker>,energy,new EnergyTracker,Attr::hidden,"Energy values, if energy tracking is enabled."))
		// ((shared_ptr<Bound>,bound,,Attr::hidden,"Bounding box of the scene (only used for rendering and initialized if needed)."))
		((shared_ptr<Field>,field,,,"Some field (experimentally only one, later there will be multiple fields available)"))
		((shared_ptr<Cell>,cell,new Cell,Attr::hidden,"Information on periodicity; only should be used if Scene::isPeriodic."))
		// ((vector<shared_ptr<Serializable> >,miscParams,,Attr::hidden,"Store for arbitrary Serializable objects; will set static parameters during deserialization (primarily for GLDraw functors which otherwise have no attribute access)"))
		// ((vector<shared_ptr<DisplayParameters> >,dispParams,,Attr::hidden,"'hash maps' of display parameters (since yade::serialization had no support for maps, emulate it via vector of strings in format key=value)"))

		,
		/*ctor*/
			fillDefaultTags();
			// interactions->postLoad__calledFromScene(bodies);
		,
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Scene);

