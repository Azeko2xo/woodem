/*************************************************************************
 Copyright (C) 2008 by Bruno Chareyre		                         *
*  bruno.chareyre@hmg.inpg.fr      					 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#pragma once

#include<yade/core/StandAloneEngine.hpp>
#include<Wm3Vector3.h>
#ifdef YADE_OPENMP
	#include<omp.h>
#endif

/*! An engine that can replace the usual series of engines used for integrating the laws of motion.

This engine is faster because it uses less loops and less dispatching 

The result is almost the same as with :
-NewtonsForceLaw
-NewtonsMomentumLaw
-LeapFrogPositionIntegrator
-LeapFrogOrientationIntegrator
-CundallNonViscousForceDamping
-CundallNonViscousMomentumDamping

...but the implementation of damping is slightly different compared to CundallNonViscousForceDamping+CundallNonViscousMomentumDamping. Here, damping is dependent on predicted (undamped) velocity at t+dt/2, while the other engines use velocity at time t.
 
Requirements :
-All dynamic bodies must have physical parameters of type (or inheriting from) BodyMacroParameters
-Physical actions must include forces and moments
 
NOTE: Cundall damping affected dynamic simulation! See examples/dynamic_simulation_tests
 
 */
class VelocityBins;

class NewtonsDampedLaw : public StandAloneEngine{
	inline void cundallDamp(const Real& dt, const Vector3r& f, const Vector3r& velocity, Vector3r& acceleration, const Vector3r& m, const Vector3r& angularVelocity, Vector3r& angularAcceleration);
	void handleClumpMember(MetaBody* ncb, const body_id_t memberId, State* clumpState);
	bool haveBins;
	public:
		///damping coefficient for Cundall's non viscous damping
		Real damping;
		/// store square of max. velocity, for informative purposes; computed again at every step
		Real maxVelocitySq;
		#ifdef YADE_OPENMP
			vector<Real> threadMaxVelocitySq;
		#endif
		/// velocity bins (not used if not created)
		shared_ptr<VelocityBins> velocityBins;
		virtual void action(MetaBody *);		
		NewtonsDampedLaw(): damping(0.2), maxVelocitySq(-1){
			#ifdef YADE_OPENMP
				threadMaxVelocitySq.resize(omp_get_max_threads());
			#endif
		}

	REGISTER_ATTRIBUTES(StandAloneEngine,(damping)(maxVelocitySq));
	REGISTER_CLASS_AND_BASE(NewtonsDampedLaw,StandAloneEngine);
};
REGISTER_SERIALIZABLE(NewtonsDampedLaw);

