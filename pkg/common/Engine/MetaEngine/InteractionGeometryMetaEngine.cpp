/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNUGeneral Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "InteractionGeometryMetaEngine.hpp"


#include<yade/core/MetaBody.hpp>
	
CREATE_LOGGER(InteractionGeometryMetaEngine);

/* Single dispatch for given pair of bodies, returning new interaction.
 *
 * The caller is responsible for inserting the interaction into some interaction container.
 *
 * The EngineUnit must not fail (return false).
 */

shared_ptr<Interaction> InteractionGeometryMetaEngine::explicitAction(const shared_ptr<Body>& b1, const shared_ptr<Body>& b2){
	//assert(b1->interactingGeometry && b2->interactingGeometry);
	//shared_ptr<Interaction> i(new Interaction(b1->getId(),b2->getId()));
	//i->isReal=true;
	//bool op=operator()(b1->interactingGeometry,b2->interactingGeometry,b1->physicalParameters->se3,b2->physicalParameters->se3,i);
	//if(!op) throw runtime_error("InteractionGeometryMetaEngine::explicitAction could not dispatch for given types ("+b1->interactingGeometry->getClassName()+","+b2->interactingGeometry->getClassName()+") or the dispatchee returned false.");
	//return i;

	// FIXME: not clear why it is not a good idea. If I really wnat interaction that I call this function, I also want it to say loudly that it failed.
	
	// Seems asserts and throws in code above is not good idea.
	// Below code do same (i.e. create interaction for specified bodies), but
	// without artifical exceptions. If creating interaction fails (for
	// example if bodies don't have an interactionGeometry), returned
	// interaction is non real, i.e. interaction->isReal==false. Sega.
	shared_ptr<Interaction> interaction(new Interaction(b1->getId(),b2->getId()));
	b1->interactingGeometry && b2->interactingGeometry && operator()( b1->interactingGeometry , b2->interactingGeometry , b1->physicalParameters->se3 , b2->physicalParameters->se3 , interaction );
	return interaction;
}

void InteractionGeometryMetaEngine::action(MetaBody* ncb)
{
	// Erase interaction that were requested for erase, but not processed by the collider, if any (and warn once about that, as it is suspicious)
	if(ncb->interactions->unconditionalErasePending()>0 && !alreadyWarnedNoCollider){
		LOG_WARN("Interactions pending erase found, no collider being used?");
		alreadyWarnedNoCollider=true;
	}

	shared_ptr<BodyContainer>& bodies = ncb->bodies;
	#ifdef YADE_OPENMP
		const long size=ncb->transientInteractions->size();
		#pragma omp parallel for
		for(long i=0; i<size; i++){
			const shared_ptr<Interaction>& I=(*ncb->transientInteractions)[i];
	#else
		FOREACH(const shared_ptr<Interaction>& I, *ncb->interactions){
	#endif
			const shared_ptr<Body>& b1=(*bodies)[I->getId1()];
			const shared_ptr<Body>& b2=(*bodies)[I->getId2()];
			bool wasReal=I->isReal();
			if (!b1->interactingGeometry || !b2->interactingGeometry) { assert(!wasReal); continue; } // some bodies do not have interactingGeometry
			bool geomCreated=operator()(b1->interactingGeometry, b2->interactingGeometry, b1->physicalParameters->se3, b2->physicalParameters->se3, I);
			// reset && erase interaction that existed but now has no geometry anymore
			if(wasReal && !geomCreated){ ncb->interactions->requestErase(I->getId1(),I->getId2()); }
	}
}

YADE_PLUGIN("InteractionGeometryMetaEngine");