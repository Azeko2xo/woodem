#ifndef __SDECDISCRETEELEMENT_H__
#define __SDECDISCRETEELEMENT_H__

#include "RigidBody.hpp"

class SDECDiscreteElement : public RigidBody
{
	public : float kn;
	public : float ks;

	// construction
	public : SDECDiscreteElement ();
	public : ~SDECDiscreteElement ();

	public : void processAttributes();
	public : void registerAttributes();

	//public : void updateBoundingVolume(Se3& se3);
	//public : void updateCollisionModel(Se3& se3);

	//public : void moveToNextTimeStep();

	REGISTER_CLASS_NAME(SDECDiscreteElement);
	//REGISTER_CLASS_INDEX(SDECDiscreteElement);
};

REGISTER_SERIALIZABLE(SDECDiscreteElement,false);

//REGISTER_CLASS_TO_MULTI_METHODS_MANAGER(SDECDiscreteElement);

#endif // __SDECDISCRETEELEMENT_H__

