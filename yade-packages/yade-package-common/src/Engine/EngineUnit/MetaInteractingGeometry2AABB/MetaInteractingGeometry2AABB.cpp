/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
 
#include "MetaInteractingGeometry2AABB.hpp"


#include "MetaInteractingGeometry.hpp"
#include "AABB.hpp"


void MetaInteractingGeometry2AABB::go(	  const shared_ptr<InteractingGeometry>&
						, shared_ptr<BoundingVolume>& bv
						, const Se3r&
						, const Body* body )
{
	AABB* aabb = static_cast<AABB*>(bv.get());
	
	Vector3r max(-Mathr::MAX_REAL,-Mathr::MAX_REAL,-Mathr::MAX_REAL);
	Vector3r min( Mathr::MAX_REAL, Mathr::MAX_REAL, Mathr::MAX_REAL);
	
	const MetaBody * ncb = dynamic_cast<const MetaBody*>(body);
	const shared_ptr<BodyContainer>& bodies = ncb->bodies;
	
	BodyContainer::iterator bi    = bodies->begin();
	BodyContainer::iterator biEnd = bodies->end();
	for( ; bi!=biEnd ; ++bi )
	{
		shared_ptr<Body> b = *bi;
		if(b->boundingVolume)
		{
	 		max = max.maxVector(b->boundingVolume->max);
 			min = min.minVector(b->boundingVolume->min);
		} 
		else
		{
	 		max = max.maxVector(b->physicalParameters->se3.position);
 			min = min.minVector(b->physicalParameters->se3.position);
		}
	}
	
	aabb->center = (max+min)*0.5;
	aabb->halfSize = (max-min)*0.5;
	
	aabb->min = min;
	aabb->max = max;
}
	
