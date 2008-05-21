/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/


#include "RotationEngine.hpp"
#include "RigidBodyParameters.hpp"
#include<yade/core/MetaBody.hpp>
#include<yade/lib-base/yadeWm3Extra.hpp>


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

void RotationEngine::postProcessAttributes(bool deserializing)
{
		if (!deserializing) return;
		rotationAxis.Normalize();
}

void RotationEngine::applyCondition(MetaBody *ncb)
{

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

YADE_PLUGIN();
