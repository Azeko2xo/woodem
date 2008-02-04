/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "GravityEngine.hpp"
#include "ParticleParameters.hpp"
#include "Force.hpp"
#include<yade/core/MetaBody.hpp>

GravityEngine::GravityEngine() : actionParameterForce(new Force), gravity(Vector3r::ZERO)
{
}


GravityEngine::~GravityEngine()
{
}


void GravityEngine::registerAttributes()
{
	REGISTER_ATTRIBUTE(gravity);
}


void GravityEngine::applyCondition(Body* body)
{
	MetaBody * ncb = YADE_CAST<MetaBody*>(body);
	shared_ptr<BodyContainer>& bodies = ncb->bodies;
	
	BodyContainer::iterator bi    = bodies->begin();
	BodyContainer::iterator biEnd = bodies->end();
	for( ; bi!=biEnd ; ++bi )
	{
		shared_ptr<Body> b = *bi;
		/* skip bodies that are within a clump;
		 * even if they are marked isDynamic==false, forces applied to them are passed to the clump, which is dynamic;
		 * and since clump is a body with mass equal to the sum of masses of its components, it would have gravity applied twice.
		 *
		 * The choice is to skip (b->isClumpMember()) or (b->isClump()). We rather skip members,
		 * since that will apply smaller number of forces and (theoretically) improve numerical stability ;-) */
		if(b->isClumpMember()) continue;

		ParticleParameters* p = dynamic_cast<ParticleParameters*>(b->physicalParameters.get());
		if (p)
			static_cast<Force*>( ncb->physicalActions->find( b->getId() , actionParameterForce->getClassIndex() ).get() )->force += gravity * p->mass;
        }
}

YADE_PLUGIN();
