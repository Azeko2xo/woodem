/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "Collider.hpp"

Collider::Collider(){}

Collider::~Collider(){}
bool Collider::handleExistingInteraction(Interaction*){
	throw runtime_error("handleExistingInteraction should not be called");
#if 0
	/* logically, we have 4 possibilities
	 * 1.  real  new → ¬new, keep
	 * 2.  real ¬new → keep (same as 1.)
	 * 3. ¬real  new → keep (remains in potential state)
	 * 4. ¬real ¬new → delete
	 */
	
	// case 1.
	// Once the interaction has been fully created, it is not "new" anymore
	if( I->isReal &&  I->isNew){ I->isNew=false; return true; }
	// case 2.
	if( I->isReal && !I->isNew) return true; 
	// case 3.
	// Keep interaction in potential state (the collider _must_ delete it once it is not in potential state anymore, however)
	if(!I->isReal &&  I->isNew) return true;
	// case 4.
	// Remove interactions deleted by the constitutive law
	if(!I->isReal && !I->isNew) return false; // should be deleted

	assert(false); // unreachable
	return false;
#endif
}
bool Collider::mayCollide(const Body* b1, const Body* b2){
	return 
		// might be called with deleted bodies, i.e. NULL pointers
		(b1!=NULL && b2!=NULL) &&
		// only collide if at least one particle is standalone or they belong to different clumps
		(b1->isStandalone() || b2->isStandalone() || b1->clumpId!=b2->clumpId ) &&
		 // do not collide clumps, since they are just containers, never interact
		!b1->isClump() && !b2->isClump() &&
		// masks must have at least 1 bit in common
		(b1->groupMask & b2->groupMask)!=0 
	;

}

