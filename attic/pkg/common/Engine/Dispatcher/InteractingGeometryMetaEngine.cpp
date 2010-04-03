/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "InteractingGeometryMetaEngine.hpp"
#include<yade/core/Scene.hpp>

YADE_REQUIRE_FEATURE(geometricalmodel);
void InteractingGeometryMetaEngine::action()
{
	const shared_ptr<BodyContainer>& bodies = ncb->bodies;
	
	const long numBodies=(long)bodies->size();
	// #pragma omp parallel for
	for(int id=0; id<numBodies; id++){
		const shared_ptr<Body>& b=(*bodies)[id];
		if(b->geometricalModel && b->shape) operator()(b->geometricalModel,b->shape,b->physicalParameters->se3,b.get());
	}
	if(ncb->geometricalModel && ncb->shape)
		operator()(ncb->geometricalModel,ncb->shape,ncb->physicalParameters->se3,ncb);
}

YADE_PLUGIN((InteractingGeometryMetaEngine));

YADE_REQUIRE_FEATURE(PHYSPAR);

