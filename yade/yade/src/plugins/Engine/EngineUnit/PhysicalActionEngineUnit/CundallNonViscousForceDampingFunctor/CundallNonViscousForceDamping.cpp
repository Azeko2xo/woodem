/***************************************************************************
 *   Copyright (C) 2004 by Janek Kozicki                                   *
 *   cosurgi@berlios.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CundallNonViscousForceDamping.hpp"
#include "ParticleParameters.hpp"
#include "ActionParameterForce.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CundallNonViscousForceDamping::CundallNonViscousForceDamping() : damping(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CundallNonViscousForceDamping::registerAttributes()
{
	REGISTER_ATTRIBUTE(damping);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// this is Cundall non-viscous local damping, applied to force (ActionParameterForce)

void CundallNonViscousForceDamping::go(    const shared_ptr<ActionParameter>& a
						, const shared_ptr<PhysicalParameters>& b
						, const Body*)
{
	ActionParameterForce * af = static_cast<ActionParameterForce*>(a.get());
	ParticleParameters * p = static_cast<ParticleParameters*>(b.get());
	
	Vector3r& f  = af->force;
	register int sign;
	for(int j=0;j<3;j++)
	{
		if (p->velocity[j] == 0)
			sign = 0;
		else if (p->velocity[j] > 0)
			sign = 1;
		else
			sign = -1;
			
		f[j] -= damping * std::abs(f[j]) * sign;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

