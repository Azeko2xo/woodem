#include "Rotor.hpp"
#include "RigidBody.hpp"
#include "NonConnexBody.hpp"

Rotor::Rotor () : KinematicEngine() // encapsuler dans implicitfunction user redefini uniquement dp = || interpolateur ...
{
}

Rotor::~Rotor ()
{

}

void Rotor::afterDeserialization()
{

}

void Rotor::registerAttributes()
{
	KinematicEngine::registerAttributes();
	REGISTER_ATTRIBUTE(angularVelocity);
	REGISTER_ATTRIBUTE(rotationAxis);
}

void Rotor::moveToNextTimeStep(Body * body)
{

	NonConnexBody * ncb = dynamic_cast<NonConnexBody*>(body);
	vector<shared_ptr<Body> >& bodies = ncb->bodies;

	std::vector<int>::const_iterator ii = subscribedBodies.begin();
	std::vector<int>::const_iterator iiEnd = subscribedBodies.end();

	float dt = Omega::instance().dt;
	time = dt;

	Quaternionr q;
	q.fromAxisAngle(rotationAxis,angularVelocity*dt);

	Vector3r ax;
	float an;

	for(;ii!=iiEnd;++ii)
	{
		shared_ptr<Body>  b = bodies[(*ii)];

		//b->se3.translation += dp;

		b->se3.translation	= q*b->se3.translation;
		b->se3.rotation		= q*b->se3.rotation;

		b->se3.rotation.normalize();
		b->se3.rotation.toAxisAngle(ax,an);

		b->angularVelocity	= rotationAxis*angularVelocity;
		b->velocity		= Vector3r(0,0,0);

		// FIXME : this shouldn't be there
		b->updateBoundingVolume(b->se3);
	}


}
