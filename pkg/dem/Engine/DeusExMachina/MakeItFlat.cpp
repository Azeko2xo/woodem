/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include <yade/pkg-dem/MakeItFlat.hpp>
#include <yade/pkg-common/ParticleParameters.hpp>
#include <yade/pkg-common/Force.hpp>
#include <yade/core/MetaBody.hpp>

MakeItFlat::MakeItFlat() : actionParameterForce(new Force)
{
	direction=1;
	plane_position=0;
	reset_force=true;
}


MakeItFlat::~MakeItFlat()
{
}


void MakeItFlat::registerAttributes()
{
	REGISTER_ATTRIBUTE(direction);
	REGISTER_ATTRIBUTE(plane_position);
	REGISTER_ATTRIBUTE(reset_force);
}


void MakeItFlat::applyCondition(MetaBody* ncb)
{
	shared_ptr<BodyContainer>& bodies = ncb->bodies;

	BodyContainer::iterator bi    = bodies->begin();
	BodyContainer::iterator biEnd = bodies->end();
	for( ; bi!=biEnd ; ++bi )
	{
		shared_ptr<Body> b = *bi;
		/* skip bodies that are within a clump;
		 * even if they are marked isDynamic==false, forces applied to them are passed to the clump, which is dynamic;
		 * and since clump is a body with mass equal to the sum of masses of its components, it would have HydraulicForce applied twice.
		 *
		 * The choice is to skip (b->isClumpMember()) or (b->isClump()). We rather skip members,
		 * since that will apply smaller number of forces and (theoretically) improve numerical stability ;-) */
		if(b->isClumpMember()) continue;

		if(b->geometricalModel->getClassName()=="Sphere")
		{
			ParticleParameters* p = dynamic_cast<ParticleParameters*>(b->physicalParameters.get());
			if (p)
			{
				p->se3.position[direction]=plane_position;
				if(reset_force)
					#ifdef BEX_CONTAINER
						throw runtime_error("BexContainer doesn't work with MakeIfFlat resetting forces, since it would have to sync() frequently. Use PhysicalParameters::blockedDOFs instead, MakeItFlat will be removed.");
					#else
						static_cast<Force*>( ncb->physicalActions->find( b->getId() , actionParameterForce->getClassIndex() ).get() )->force[direction]=0;// 
					#endif
			}
		}
	}
}

YADE_PLUGIN();
