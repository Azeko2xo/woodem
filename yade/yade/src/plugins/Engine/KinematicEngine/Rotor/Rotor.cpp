#include "Rotor.hpp"
#include "RigidBody.hpp"
#include "ComplexBody.hpp"

void Rotor::registerAttributes()
{
	KinematicEngine::registerAttributes();
	REGISTER_ATTRIBUTE(angularVelocity);
	REGISTER_ATTRIBUTE(rotationAxis);
}

void Rotor::moveToNextTimeStep(Body * body)
{

	ComplexBody * ncb = dynamic_cast<ComplexBody*>(body);
	shared_ptr<BodyContainer> bodies = ncb->bodies;

	std::vector<int>::const_iterator ii = subscribedBodies.begin();
	std::vector<int>::const_iterator iiEnd = subscribedBodies.end();

	Real dt = Omega::instance().getTimeStep();
	time = dt;

	Quaternionr q;
	q.fromAxisAngle(rotationAxis,angularVelocity*dt);

	Vector3r ax;
	Real an;
	
	for(;ii!=iiEnd;++ii)
	{
		RigidBody * rb = static_cast<RigidBody*>((*bodies)[*ii]->physicalParameters.get());

		//rb->se3.translation	= q*rb->se3.translation;
		rb->se3.rotation	= q*rb->se3.rotation;

		rb->se3.rotation.normalize();
		rb->se3.rotation.toAxisAngle(ax,an);
		
		rb->angularVelocity	= rotationAxis*angularVelocity;
		rb->velocity		= Vector3r(0,0,0);
	}


}
