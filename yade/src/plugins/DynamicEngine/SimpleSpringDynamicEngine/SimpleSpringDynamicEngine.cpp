#include "SimpleSpringDynamicEngine.hpp"
#include "RigidBody.hpp"
#include "ClosestFeatures.hpp"
#include "Omega.hpp"


SimpleSpringDynamicEngine::SimpleSpringDynamicEngine () : DynamicEngine()
{
	first = true;
}

SimpleSpringDynamicEngine::~SimpleSpringDynamicEngine ()
{

}

void SimpleSpringDynamicEngine::processAttributes()
{

}

void SimpleSpringDynamicEngine::registerAttributes()
{
}


void SimpleSpringDynamicEngine::respondToCollisions(std::vector<shared_ptr<Body> >& bodies, const std::list<shared_ptr<Contact> >& contacts,float dt)
{
	float stiffness = 100;
	float damping = 1.02;
	Vector3 gravity = Omega::instance().gravity;
	if (first)
	{
		forces.resize(bodies.size());
		couples.resize(bodies.size());
		prevVelocities.resize(bodies.size());
	}
	
	std::vector<Vector3>::iterator fi = forces.begin();
	std::vector<Vector3>::iterator fiEnd = forces.end();
	std::vector<Vector3>::iterator ci = couples.begin();
	std::vector<shared_ptr<Body> >::iterator bi = bodies.begin();
	for( ; fi!=fiEnd; ++fi, ++ci, ++bi)
	{
		shared_ptr<RigidBody>  rb = shared_dynamic_cast<RigidBody>(*bi);
		(*fi) = (gravity - rb->velocity*damping)*rb->mass;
		(*ci) = -(rb->angularVelocity*damping).multTerm(rb->inertia);
	}
	
	std::list<shared_ptr<Contact> >::const_iterator cti = contacts.begin();
	std::list<shared_ptr<Contact> >::const_iterator ctiEnd = contacts.end();
	for( ; cti!=ctiEnd ; ++cti)
	{
		std::vector<std::pair<Vector3,Vector3> >::iterator cpi = (shared_dynamic_cast<ClosestFeatures>((*cti)->contactModel))->closestsPoints.begin();
		std::vector<std::pair<Vector3,Vector3> >::iterator cpiEnd = (shared_dynamic_cast<ClosestFeatures>((*cti)->contactModel))->closestsPoints.end();
		for( ; cpi!=cpiEnd ; ++cpi)
		{	
			Vector3 f = (*cpi).first-(*cpi).second;

			f *= stiffness/(float)((shared_dynamic_cast<ClosestFeatures>((*cti)->contactModel))->closestsPoints.size());
			
			Vector3 p = 0.5*((*cpi).first+(*cpi).second);
			
			shared_ptr<RigidBody> r1 = shared_dynamic_cast<RigidBody>(bodies[(*cti)->id1]);
			shared_ptr<RigidBody> r2 = shared_dynamic_cast<RigidBody>(bodies[(*cti)->id2]);
			
			Vector3 o1p = p - r1->se3.translation;
			Vector3 o2p = p - r2->se3.translation;
						
			forces[(*cti)->id1] -= f;
			forces[(*cti)->id2] += f;
		
			couples[(*cti)->id1] -= o1p.cross(f);
			couples[(*cti)->id2] += o2p.cross(f);
		}
	}

		
	for(unsigned int i=0; i < bodies.size(); i++)
        {
		shared_ptr<RigidBody> rb = shared_dynamic_cast<RigidBody>(bodies[i]);

		if (rb->isDynamic)
		{
			Vector3 acc = forces[i]*rb->invMass;

			if (!first)
				rb->velocity = prevVelocities[i]+0.5*dt*acc;

			prevVelocities[i] = rb->velocity+0.5*dt*acc;
			rb->se3.translation += prevVelocities[i]*dt;

			rb->updateBoundingVolume(rb->se3);
		}
        }
	first = false;

// 	for(unsigned int i=0; i < bodies.size(); i++)
//         {
// 		shared_ptr<RigidBody> rb = shared_dynamic_cast<RigidBody>(bodies[i]);
// 				
// 		if (rb->isDynamic)
// 		{
// 			Vector3 acc = forces[i]*rb->invMass;
// 			Vector3 mom = couples[i].multTerm(rb->invInertia);
// 		
// 			rb->velocity += dt*acc;
// 			rb->se3.translation += dt*rb->velocity;
// 
// 			rb->angularVelocity += dt*mom;			
// 			Vector3 axis = rb->angularVelocity;
// 			float angle = axis.unitize();
// 			Quaternion q;
// 			q.fromAngleAxis(angle*dt,axis);
// 			rb->se3.rotation = rb->se3.rotation*q;
// 			rb->se3.rotation.normalize();			
// 
// 			rb->updateBoundingVolume(rb->se3);
// 		}
//         }
}

