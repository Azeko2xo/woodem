/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "RigidBodyParameters.hpp"

RigidBodyParameters::RigidBodyParameters() : ParticleParameters()
{
	createIndex();
	acceleration = Vector3r(0,0,0);
	angularAcceleration = Vector3r(0,0,0);
}

RigidBodyParameters::~RigidBodyParameters()
{
}

void RigidBodyParameters::postProcessAttributes(bool deserializing)
{
	ParticleParameters::postProcessAttributes(deserializing);
	
	if(deserializing)
	{
		
		for(int i=0;i<3;i++)
		{
			if (inertia[i]==0)
				invInertia[i] = 0;
			else
				invInertia[i] = 1.0/inertia[i];
		}
	}
}

void RigidBodyParameters::registerAttributes()
{
	ParticleParameters::registerAttributes();
	REGISTER_ATTRIBUTE(inertia);
	REGISTER_ATTRIBUTE(angularVelocity);
}

