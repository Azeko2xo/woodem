/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include"PhysicalActionApplier.hpp"
#include<yade/core/Scene.hpp>

void PhysicalActionApplier::action(Scene* ncb){
	ncb->forces.sync();
	FOREACH(const shared_ptr<Body>& b, *ncb->bodies){
		operator()(b->physicalParameters,b.get(),ncb);
	}
}
YADE_PLUGIN((PhysicalActionApplier));

YADE_REQUIRE_FEATURE(PHYSPAR);

