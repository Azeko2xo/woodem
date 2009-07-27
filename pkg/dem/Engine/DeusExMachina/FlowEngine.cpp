/*************************************************************************
*  Copyright (C) 2009 by Bruno Chareyre                                  *
*  bruno.chareyre@hmg.inpg.fr                                            *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "FlowEngine.hpp"
#include <yade/pkg-common/ParticleParameters.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/pkg-common/RigidBodyParameters.hpp>
#include<yade/lib-base/yadeWm3Extra.hpp>


FlowEngine::FlowEngine() : gravity(Vector3r::ZERO), isActivated(false)
{
dummyParameter = false;
}


FlowEngine::~FlowEngine()
{
}


void FlowEngine::applyCondition(MetaBody* ncb)
{

    if (isActivated)
    {
        shared_ptr<BodyContainer>& bodies = ncb->bodies;



	FOREACH(const shared_ptr<Body>& b, *ncb->bodies){
		// clump members are non-dynamic; they skip the rest of loop once their forces are properly taken into account, however
		//if (!b->isDynamic && !b->isClumpMember()) continue;
		
		RigidBodyParameters* rb = YADE_CAST<RigidBodyParameters*>(b->physicalParameters.get());
		///Access data (examples)
		const body_id_t& id=b->getId();
		Real vx = rb->velocity[0];
		Real rx = rb->angularVelocity[0];
		Real x = rb->se3.position[0];
	}

	///Compute flow and and forces here

	
	
	
	
	
		
	
	
	///End Compute flow and and forces
	
	
	
	
	int Nspheres=100;
	for (long i=1; i<Nspheres; ++i)
	{

		//file >> id >> fx >> fy >> fz >> mx >> my >> mz;

		//Vector3r f (fx,fy,fz);
		//Vector3r t (mx,my,mz);
                
		//b->bex.addForce(id,f);
		//ncb->bex.addTorque(id,t);

	}
	
    }
}

YADE_PLUGIN("FlowEngine");