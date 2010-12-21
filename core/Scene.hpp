/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/Body.hpp>
#include<yade/core/Cell.hpp>
#include<yade/core/BodyContainer.hpp>
#include<yade/core/Engine.hpp>
#include<yade/core/Material.hpp>
#include<yade/core/DisplayParameters.hpp>
#include<yade/core/ForceContainer.hpp>
#include<yade/core/InteractionContainer.hpp>
#include<yade/core/EnergyTracker.hpp>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255 
#endif


class Bound;

class Scene: public Serializable{
	public:
		//! Adds material to Scene::materials. It also sets id of the material accordingly and returns it.
		int addMaterial(shared_ptr<Material> m){ materials.push_back(m); m->id=(int)materials.size()-1; return m->id; }
		//! Checks that type of Body::state satisfies Material::stateTypeOk. Throws runtime_error if not. (Is called from BoundDispatcher the first time it runs)
		void checkStateTypes();
		//! update our bound; used directly instead of a BoundFunctor, since we don't derive from Body anymore
		void updateBound();

		// neither serialized, nor accessible from python (at least not directly)
		ForceContainer forces;

		// initialize tags (author, date, time)
		void fillDefaultTags();
		// advance by one iteration by running all engines
		void moveToNextTimeStep();

		/* Functions operating on TimeStepper; they all throw exception if there is more than 1 */
		// return whether a TimeStepper is present
		bool timeStepperPresent();
		// true if TimeStepper is present and active, false otherwise
		bool timeStepperActive();
		// (de)activate TimeStepper; returns whether the operation was successful (i.e. whether a TimeStepper was found)
		bool timeStepperActivate(bool activate);

		shared_ptr<Engine> engineByName(const string& s);

		void postLoad(Scene&);

	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Scene,Serializable,"Object comprising the whole simulation.",
		((Real,dt,1e-8,,"Current timestep for integration."))
		((long,iter,0,Attr::readonly,"Current iteration (computational step) number"))
		((bool,subStepping,false,,"Whether we currently advance by one engine in every step (rather than by single run through all engines)."))
		((int,subStep,-1,Attr::readonly,"Number of sub-step; not to be changed directly. -1 means to run loop prologue (cell integration), 0…n-1 runs respective engines (n is number of engines), n runs epilogue (increment step number and time."))
		((Real,time,0,Attr::readonly,"Simulation time (virtual time) [s]"))
		((long,stopAtIter,0,,"Iteration after which to stop the simulation."))
		#if 0
			// not yet implemented
			((Real,stopAtTime,0,,"Time at which to stop the simulation"))
			((Real,stopAtRealTime,0,,"Time at which to stop the simulation"))
		#endif
		((bool,isPeriodic,false,Attr::readonly,"Whether periodic boundary conditions are active."))
		((bool,trackEnergy,false,Attr::readonly,"Whether energies are being traced."))
		((bool,runInternalConsistencyChecks,true,Attr::hidden,"Run internal consistency check, right before the very first simulation step."))
		((Body::id_t,selectedBody,-1,,"Id of body that is selected by the user"))

		((list<string>,tags,,,"Arbitrary key=value associations (tags like mp3 tags: author, date, version, description etc.)"))
		((vector<shared_ptr<Engine> >,engines,,Attr::hidden,"Engines sequence in the simulation."))
		((vector<shared_ptr<Engine> >,_nextEngines,,Attr::hidden,"Engines to be used from the next step on; is returned transparently by O.engines if in the middle of the loop (controlled by subStep>=0)."))
		// NOTE: bodies must come before interactions, since InteractionContainer is initialized with a reference to BodyContainer::body
		((shared_ptr<BodyContainer>,bodies,new BodyContainer,Attr::hidden,"Bodies contained in the scene."))
		((shared_ptr<InteractionContainer>,interactions,new InteractionContainer,Attr::hidden,"All interactions between bodies."))
		((shared_ptr<EnergyTracker>,energy,new EnergyTracker,Attr::hidden,"Energy values, if energy tracking is enabled."))
		((vector<shared_ptr<Material> >,materials,,Attr::hidden,"Container of shared materials. Add elements using Scene::addMaterial, not directly. Do NOT remove elements from here unless you know what you are doing!"))
		((shared_ptr<Bound>,bound,,Attr::hidden,"Bounding box of the scene (only used for rendering and initialized if needed)."))

		((shared_ptr<Cell>,cell,new Cell,Attr::hidden,"Information on periodicity; only should be used if Scene::isPeriodic."))
		((vector<shared_ptr<Serializable> >,miscParams,,Attr::hidden,"Store for arbitrary Serializable objects; will set static parameters during deserialization (primarily for GLDraw functors which otherwise have no attribute access)"))
		((vector<shared_ptr<DisplayParameters> >,dispParams,,Attr::hidden,"'hash maps' of display parameters (since yade::serialization had no support for maps, emulate it via vector of strings in format key=value)"))

		,
		/*ctor*/
			fillDefaultTags();
			interactions->postLoad__calledFromScene(bodies);
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Scene);

