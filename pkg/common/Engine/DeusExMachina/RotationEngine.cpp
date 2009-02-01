/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/


#include"RotationEngine.hpp"
#include<yade/pkg-common/RigidBodyParameters.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/lib-base/yadeWm3Extra.hpp>
#include<yade/extra/Shop.hpp>

#include<yade/pkg-common/LinearInterpolate.hpp>

YADE_PLUGIN("RotationEngine","SpiralEngine","InterpolatingSpiralEngine");


void InterpolatingSpiralEngine::applyCondition(MetaBody* rb){
	Real virtTime=wrap ? Shop::periodicWrap(rb->simulationTime,*times.begin(),*times.rbegin()) : rb->simulationTime;
	angularVelocity=linearInterpolate<Real>(virtTime,times,angularVelocities,pos);
	linearVelocity=angularVelocity*slope;
	SpiralEngine::applyCondition(rb);
}

void SpiralEngine::applyCondition(MetaBody* rb){
	Real dt=Omega::instance().getTimeStep();
	axis.Normalize();
	Quaternionr q;
	q.FromAxisAngle(axis,angularVelocity*dt);
	shared_ptr<BodyContainer> bodies = rb->bodies;
	FOREACH(body_id_t id,subscribedBodies){
		assert(id<bodies->size());
		Body* b=Body::byId(id,rb).get();
		ParticleParameters* rbp=YADE_CAST<RigidBodyParameters*>(b->physicalParameters.get());
		assert(rbp);
		// translation
		rbp->se3.position+=dt*linearVelocity*axis;
		// rotation
		rbp->se3.position=q*(rbp->se3.position-axisPt)+axisPt;
		rbp->se3.orientation=q*rbp->se3.orientation;
		rbp->se3.orientation.Normalize(); // to make sure
	}
}

RotationEngine::RotationEngine()
{
	rotateAroundZero = false;
	zeroPoint = Vector3r(0,0,0);
}


void RotationEngine::registerAttributes()
{
	DeusExMachina::registerAttributes();
	REGISTER_ATTRIBUTE(angularVelocity);
	REGISTER_ATTRIBUTE(rotationAxis);
	REGISTER_ATTRIBUTE(rotateAroundZero);
	REGISTER_ATTRIBUTE(zeroPoint);
}

void RotationEngine::applyCondition(MetaBody *ncb)
{
    rotationAxis.Normalize();

	shared_ptr<BodyContainer> bodies = ncb->bodies;

	std::vector<int>::const_iterator ii = subscribedBodies.begin();
	std::vector<int>::const_iterator iiEnd = subscribedBodies.end();

	Real dt = Omega::instance().getTimeStep();
	// time = dt;

	Quaternionr q;
	q.FromAxisAngle(rotationAxis,angularVelocity*dt);

	Vector3r ax;
	Real an;
	
	for(;ii!=iiEnd;++ii)
	{
		RigidBodyParameters * rb = static_cast<RigidBodyParameters*>((*bodies)[*ii]->physicalParameters.get());

		if(rotateAroundZero)
			rb->se3.position	= q*(rb->se3.position-zeroPoint)+zeroPoint; // for RotatingBox
			
		rb->se3.orientation	= q*rb->se3.orientation;

		rb->se3.orientation.Normalize();
		rb->se3.orientation.ToAxisAngle(ax,an);
		
		rb->angularVelocity	= rotationAxis*angularVelocity;
		rb->velocity		= Vector3r(0,0,0);
	}


}

