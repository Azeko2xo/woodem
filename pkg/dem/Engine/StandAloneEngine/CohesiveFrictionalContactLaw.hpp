/*************************************************************************
*  Copyright (C) 2007 by Bruno Chareyre <bruno.chareyre@imag.fr>         *
*  Copyright (C) 2008 by Janek Kozicki <cosurgi@berlios.de>              *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/InteractionSolver.hpp>

#include <set>
#include <boost/tuple/tuple.hpp>


class CohesiveFrictionalContactLaw : public InteractionSolver
{
/// Attributes
	public :
		int sdecGroupMask;
		bool momentRotationLaw, erosionActivated, detectBrokenBodies,always_use_moment_law;
		bool shear_creep,twist_creep;
		Real creep_viscosity; /// probably should be moved to CohesiveFrictionalRelationships...
		long iter;/// used for checking if new iteration
	
		CohesiveFrictionalContactLaw();
		void action(MetaBody*);

	REGISTER_ATTRIBUTES(InteractionSolver,(sdecGroupMask)(momentRotationLaw)(erosionActivated)(detectBrokenBodies)(always_use_moment_law)(shear_creep)(twist_creep)(creep_viscosity));
	REGISTER_CLASS_NAME(CohesiveFrictionalContactLaw);
	REGISTER_BASE_CLASS_NAME(InteractionSolver);
};

REGISTER_SERIALIZABLE(CohesiveFrictionalContactLaw);

