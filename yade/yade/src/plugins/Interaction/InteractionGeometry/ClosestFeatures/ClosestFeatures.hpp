#ifndef __CLOSESTSFEATURES_H__
#define __CLOSESTSFEATURES_H__

#include <vector>

#include "GeometryOfInteraction.hpp"
#include "Vector3.hpp"

class ClosestFeatures : public GeometryOfInteraction
{
	public : std::vector<std::pair<Vector3r,Vector3r> > closestsPoints;
	public : std::vector<int> verticesId;
	// construction
	public : ClosestFeatures ();
	public : ~ClosestFeatures ();

	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();

	REGISTER_CLASS_NAME(ClosestFeatures);
};

REGISTER_SERIALIZABLE(ClosestFeatures,false);

#endif // __CLOSESTSFEATURES_H__
