 
#ifndef __TERRAIN2AABBFACTORY_H__
#define __TERRAIN2AABBFACTORY_H__

#include "BoundingVolumeFactory.hpp"

class Terrain2AABBFactory : public BoundingVolumeFactory
{	
	// construction
	public : Terrain2AABBFactory ();
	public : ~Terrain2AABBFactory ();
	
	public : void processAttributes();
	public : void registerAttributes();
	
	public : shared_ptr<BoundingVolume> buildBoundingVolume(const shared_ptr<CollisionModel> cm, const Se3& se3);
	REGISTER_CLASS_NAME(Terrain2AABBFactory);
};

REGISTER_CLASS(Terrain2AABBFactory,false);

#endif // __TERRAIN2AABBFACTORY_H__
