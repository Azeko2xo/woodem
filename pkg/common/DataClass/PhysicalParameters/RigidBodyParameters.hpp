/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/ParticleParameters.hpp>
#include<yade/lib-base/yadeWm3.hpp>

class RigidBodyParameters : public ParticleParameters
{	
	public :
		// parameters
		Vector3r	 inertia
		// state
				,angularAcceleration
				,angularVelocity;
	
		RigidBodyParameters ();
		virtual ~RigidBodyParameters ();

/// Serialization										///
	REGISTER_ATTRIBUTES(ParticleParameters,(inertia)(angularVelocity));
	REGISTER_CLASS_NAME(RigidBodyParameters);
	REGISTER_BASE_CLASS_NAME(ParticleParameters);
	
/// Indexable											///
	REGISTER_CLASS_INDEX(RigidBodyParameters,ParticleParameters);
};

REGISTER_SERIALIZABLE(RigidBodyParameters);


