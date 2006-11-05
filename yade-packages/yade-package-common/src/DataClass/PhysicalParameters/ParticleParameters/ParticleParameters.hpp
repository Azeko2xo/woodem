/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef PARTICLEPARAMETERS_HPP
#define PARTICLEPARAMETERS_HPP

#include <yade/yade-core/PhysicalParameters.hpp>
#include <yade/yade-lib-wm3-math/Vector3.hpp>
#include <yade/yade-lib-wm3-math/Se3.hpp>

class ParticleParameters : public PhysicalParameters
{
	public :
		// parameters
		Real		 mass
				,invMass;

		// state
		Vector3r	 acceleration
				,velocity;

		ParticleParameters();
		virtual ~ParticleParameters();

/// Serializable
	protected :
		virtual void postProcessAttributes(bool deserializing);
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(ParticleParameters);
	REGISTER_BASE_CLASS_NAME(PhysicalParameters);

/// Indexable
	REGISTER_CLASS_INDEX(ParticleParameters,PhysicalParameters);

};

REGISTER_SERIALIZABLE(ParticleParameters,false);

#endif // PARTICLEPARAMETERS_HPP

