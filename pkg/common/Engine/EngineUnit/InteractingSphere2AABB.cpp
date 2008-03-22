/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
 
#include "InteractingSphere2AABB.hpp"
#include "InteractingSphere.hpp"
#include "AABB.hpp"

void InteractingSphere2AABB::go(const shared_ptr<InteractingGeometry>& cm, shared_ptr<BoundingVolume>& bv, const Se3r& se3, const Body*){
	InteractingSphere* sphere = static_cast<InteractingSphere*>(cm.get());
	AABB* aabb = static_cast<AABB*>(bv.get());
	aabb->center = se3.position;
	aabb->halfSize = Vector3r(sphere->radius,sphere->radius,sphere->radius);
	
	aabb->min = aabb->center-aabb->halfSize*aabbEnlargeFactor;
	aabb->max = aabb->center+aabb->halfSize*aabbEnlargeFactor;	
}
	
YADE_PLUGIN();
