/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "GeometricalModelForceColorizer.hpp"
#include<yade/pkg-common/RigidBodyParameters.hpp>
#include<yade/core/Omega.hpp>
#include<yade/core/Scene.hpp>
#include <boost/lexical_cast.hpp>
#include<yade/lib-base/yadeWm3Extra.hpp>
//#include <Wm3Vector3r.hpp>

YADE_REQUIRE_FEATURE(geometricalmodel);

GeometricalModelForceColorizer::GeometricalModelForceColorizer () : GlobalEngine()
{
}


bool GeometricalModelForceColorizer::isActivated(Scene*)
{
	return true;
//	return ((Omega::instance().getCurrentIteration() % interval == 0) && (ofile));
}


void GeometricalModelForceColorizer::action()
{
	// FIXME the same in GLDrawLatticeBeamState.cpp
	ncb->forces.sync();

	BodyContainer* bodies = ncb->bodies.get();
	
	Real min=10000,max=-10000;

	BodyContainer::iterator bi    = bodies->begin();
	BodyContainer::iterator biEnd = bodies->end();
	for( ; bi!=biEnd ; ++bi ) // first loop to calculate min/max
	{
		Body* body = (*bi).get();
		if(body->isDynamic)
		{
			unsigned int i = body -> getId();
			Vector3r force=ncb->forces.getForce(i);
			min = std::min( force[0] , std::min( force[1] , std::min( force[2], min ) ) );
			max = std::max( force[0] , std::max( force[1] , std::max( force[2], max ) ) );
		}

	}
	
	bi    = bodies->begin();
	biEnd = bodies->end();
	for( ; bi!=biEnd ; ++bi ) // second loop to assign correct color
	{
		Body* body = (*bi).get();
		if(body->isDynamic)
		{
			GeometricalModel* gm = body->geometricalModel.get();
			unsigned int i = body -> getId();
			Vector3r force=ncb->forces.getForce(i);

			gm->diffuseColor[0] = (force[2]-min)/(max-min);
			gm->diffuseColor[1] = (force[2]-min)/(max-min);
			gm->diffuseColor[2] = (force[2]-min)/(max-min);

		/*
			//Real val = std::sqrt(force[0]*force[0]+force[1]*force[1]+force[2]*force[2]);
			Real val = force[2];
			//Real val = force.squaredLength();
			//min=0;
			gm->diffuseColor[0] = (val-min)/(max-min);
			gm->diffuseColor[1] = (val-min)/(max-min);
			gm->diffuseColor[2] = (val-min)/(max-min);
		*/

		}

	}
		
}

YADE_PLUGIN((GeometricalModelForceColorizer));

YADE_REQUIRE_FEATURE(PHYSPAR);

