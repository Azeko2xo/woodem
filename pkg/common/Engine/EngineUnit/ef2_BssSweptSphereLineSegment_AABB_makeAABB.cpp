/*************************************************************************
*  Copyright (C) 2008 by Vincent Richefeu                                *
*  vincent.richefeu@hmg.inpg.fr                                          *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
 
#include "ef2_BssSweptSphereLineSegment_AABB_makeAABB.hpp"
#include<yade/pkg-common/BssSweptSphereLineSegment.hpp>
#include<yade/pkg-common/AABB.hpp>

void ef2_BssSweptSphereLineSegment_AABB_makeAABB::go(	const shared_ptr<InteractingGeometry>& cm,
				shared_ptr<BoundingVolume>& bv,
				const Se3r& se3,
				const Body*	)
{
        BssSweptSphereLineSegment* SSLS = static_cast<BssSweptSphereLineSegment*>(cm.get());
        AABB* aabb = static_cast<AABB*>(bv.get());
        
        aabb->center = SSLS->position + 0.5 * SSLS->length * SSLS->orientation; // here orientation is not a quaternion but a unit vector
        aabb->halfSize = (0.5 * SSLS->length) * SSLS->orientation + Vector3r(SSLS->radius,SSLS->radius,SSLS->radius); 

        aabb->min = aabb->center - aabb->halfSize;
        aabb->max = aabb->center + aabb->halfSize; 
}
	
YADE_PLUGIN("ef2_BssSweptSphereLineSegment_AABB_makeAABB");
