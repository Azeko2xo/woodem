#ifndef __SIMPLESPRINGDYNAMICENGINE_H__
#define __SIMPLESPRINGDYNAMICENGINE_H__

#include "DynamicEngine.hpp"

class SimpleSpringDynamicEngine : public DynamicEngine
{
	private : std::vector<Vector3r> prevVelocities;
	private : std::vector<Vector3r> forces;
	private : std::vector<Vector3r> couples;
	private : bool first;

	// construction
	public : SimpleSpringDynamicEngine ();
	public : ~SimpleSpringDynamicEngine ();

	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();

	//public : void respondToCollisions(std::vector<shared_ptr<Body> >& bodies, const std::list<shared_ptr<Interaction> >& interactions,float dt);
	public : void respondToCollisions(Body* body);
	REGISTER_CLASS_NAME(SimpleSpringDynamicEngine);
};

REGISTER_SERIALIZABLE(SimpleSpringDynamicEngine,false);

#endif // __SIMPLESPRINGDYNAMICENGINE_H__
