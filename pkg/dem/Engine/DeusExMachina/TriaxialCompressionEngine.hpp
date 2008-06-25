/*************************************************************************
*  Copyright (C) 2006 by Bruno Chareyre                                  *
*  bruno.chareyre@hmg.inpg.fr                                            *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/


#ifndef TRIAXIALCOMPRESSIONENGINE_HPP
#define TRIAXIALCOMPRESSIONENGINE_HPP

#include<yade/core/DeusExMachina.hpp>
#include <Wm3Vector3.h>
#include<yade/lib-base/yadeWm3.hpp>
#include "TriaxialStressController.hpp"
#include <string>


/*! \brief Isotropic compression + uniaxial compression test

	detailed description...
 */

class PhysicalAction;

/** \brief Class for controlling optional initial isotropic compaction and subsequent triaxial stress test with hydrostatic confinement.
 *
 * The engine is a state machine with the following states (and automatic transitions):
 *
 * 1. STATE_ISO_COMPACTION: isotropic compaction (compression) until
 *    the prescribed mean pressue sigmaIsoCompaction is reached and the packing is stable.
 *    The compaction happens either by straining the walls (!internalCompaction)
 *    or by growing size of grains (internalCompaction)
 * 2. STATE_ISO_UNLOADING: isotropic unloading from the previously reached state, until
 *    the mean pressure sigmaLateralConfinement is reached (and stabilizes)
 * 3. STATE_TRIAX_LOADING: confined uniaxial compression:
 *		constant sigmaLateralConfinement is kept at lateral walls (left, right, front, back), while
 * 	top and bottom walls load the packing in their axis (by straining), until the value of epsilonMax
 * 	(deformation along the loading axis) is reached. At this point, the simulation is stopped.
 * 4. STATE_TRIAX_LIMBO: currently unused, since simulation is hard-stopped in the previous state.
 *
 */
class TriaxialCompressionEngine : public TriaxialStressController
{
	private :
		shared_ptr<PhysicalAction> actionForce;
		std::string Phase1End;//used to name output files based on current state
				
	public :
		TriaxialCompressionEngine();
		virtual ~TriaxialCompressionEngine();
		
		// FIXME: current serializer doesn't handle named enum types, this is workaround.
		#define stateNum int
		// should be "enum stateNum {...}" once this is fixed
		enum {STATE_UNINITIALIZED, STATE_ISO_COMPACTION, STATE_ISO_UNLOADING, STATE_TRIAX_LOADING, STATE_LIMBO};
		stateNum currentState;
		void doStateTransition(MetaBody *body, stateNum nextState);
		#define _STATE_CASE(ST) case ST: return #ST
		string stateName(stateNum st){switch(st){ _STATE_CASE(STATE_UNINITIALIZED);_STATE_CASE(STATE_ISO_COMPACTION);_STATE_CASE(STATE_ISO_UNLOADING);_STATE_CASE(STATE_TRIAX_LOADING);_STATE_CASE(STATE_LIMBO); default: return "<unknown state>"; } }
		#undef _STATE_CASE
		
		//! Strain velocity (./s)
		Real strainRate;
		Real currentStrainRate;
		//! Max ratio of resultant forces on mean contact force
		Real UnbalancedForce;
		//! Value of UnbalancedForce for which the system is considered stable
		Real StabilityCriterion;
		//! Previous value of inherited sigma_iso (used to detect manual changes of the confining pressure)
		Real previousSigmaIso;
		//! Desired isotropic pressure during the compaction phase
		Real sigmaIsoCompaction;
		//! Desired isotropic pressure during the confined uniaxial test; may be different from sigmaIsoCompaction
		Real sigmaLateralConfinement;
		//! Value of axial deformation for which the simulation must stop
		Real epsilonMax;
		//! Current value of axial deformation during confined loading (is reference to strain[1])
		Real& uniaxialEpsilonCurr;
		//! Value of friction to use for the compression test
		Real frictionAngleDegree;
		//! Previous state (used to detect manual changes of the state in .xml)
		stateNum previousState;
		//Vector3r strain;
		Vector3r translationAxis;
		//! is isotropicInternalCompactionFinished?
		bool Phase1, saveSimulation;
		//! is this the beginning of the simulation, after reading the scene?
		bool firstRun;
		int FinalIterationPhase1, Iteration, testEquilibriumInterval;
		
		std::string Key;//A code that is appended to file names to help distinguish between different simulations
		// //! Is uniaxial compression currently activated?
		// bool compressionActivated;
		//! Auto-switch between isotropic and uniaxial compression?
		bool autoCompressionActivation;
				
		virtual void applyCondition(MetaBody * ncb);
		void updateParameters(MetaBody * ncb);
		
		///Change physical properties of interactions and/or bodies in the middle of a simulation (change only friction for the moment, complete this function to set cohesion and others before compression test)
		void setContactProperties(MetaBody * body, Real frictionAngle);


		DECLARE_LOGGER;
		

	protected :
		virtual void postProcessAttributes(bool);
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(TriaxialCompressionEngine);
	REGISTER_BASE_CLASS_NAME(TriaxialStressController);
};

REGISTER_SERIALIZABLE(TriaxialCompressionEngine,false);

#endif //  TRIAXIALCOMPRESSIONENGINE_HPP

