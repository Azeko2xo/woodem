/*************************************************************************
*  Copyright (C) 2008 by Jérôme DURIEZ                                   *
*  duriez@geo.hmg.inpg.fr                                                *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef CONTACT_LAW_1_HPP
#define CONTACT_LAW_1_HPP

#include<yade/core/InteractionSolver.hpp>

#include <set>
#include <boost/tuple/tuple.hpp>

/*! \brief Contact law including cohesion, moment transfer and inelastic compression behaviour

This contact Law is inspired by CohesiveFrictionalContactLaw (inspired itselve directly from the work of Plassiard & Belheine, see the corresponding articles in "Annual Report 2006" in http://geo.hmg.inpg.fr/frederic/Discrete_Element_Group_FVD.html for example). It allows so to set moments, cohesion, tension limit and (that's the difference) inelastic unloadings in compression between bodies.
All that concerned brokenBodies (this flag and the erosionactivated one) and the useless "iter" has been suppressed.

The Relationsships corresponding are CL1Relationships, where the rigidities, the friction angles (with their tan()), and the orientations of the interactions are calculated. No more cohesion and tension limits are computed for all the interactions
To use it you should also use :
- CohesiveFrictionalBodyParameters for the bodies, with "isCohesive" = 1 (A verifier ce dernier point)
- CL1Relationships (=> which involves interactions of "ContactLaw1Interaction" type)

 */


class PhysicalAction;

class ContactLaw1 : public InteractionSolver
{
/// Attributes
	private :
		shared_ptr<PhysicalAction> actionForce;
		shared_ptr<PhysicalAction> actionMomentum;

	public :
		int sdecGroupMask;
		Real coeff_dech;	// = kn(unload) / kn(load)
		bool momentRotationLaw;
		bool momentAlwaysElastic;	// if true the value of the momentum (computed only if momentRotationLaw !!) is not limited by a plastic threshold
	
		ContactLaw1();
		void action(MetaBody*);

	protected :
		void registerAttributes();
	REGISTER_CLASS_NAME(ContactLaw1);
	REGISTER_BASE_CLASS_NAME(InteractionSolver);
};

REGISTER_SERIALIZABLE(ContactLaw1,false);

#endif // CONTACT_LAW_1_HPP

