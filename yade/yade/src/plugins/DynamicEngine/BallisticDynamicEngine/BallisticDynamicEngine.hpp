#ifndef __BALLISTICDYNAMICENGINE_H__
#define __BALLISTICDYNAMICENGINE_H__

#include "DynamicEngine.hpp"

class BallisticDynamicEngine : public DynamicEngine
{
	private : Vector3 prevVelocity;
	private : bool first;
	
	// construction
	public : BallisticDynamicEngine ();
	public : ~BallisticDynamicEngine ();
	
	public : void processAttributes();
	public : void registerAttributes();
	
	//public : void respondToCollisions(std::vector<shared_ptr<Body> >& bodies, const std::list<shared_ptr<Interaction> >& interactions,float dt);
	public : void respondToCollisions(Body* body, const std::list<shared_ptr<Interaction> >& interactions);
	REGISTER_CLASS_NAME(BallisticDynamicEngine);
};

REGISTER_CLASS(BallisticDynamicEngine,false);

#endif // __BALLISTICDYNAMICENGINE_H__
