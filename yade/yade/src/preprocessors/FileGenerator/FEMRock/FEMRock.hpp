
#ifndef __FEMROCK_H__
#define __FEMROCK_H__

#include "Serializable.hpp"
#include "Vector3.hpp"
/*class NodeProperties : public Serializable
{
	public : float invMass;
	public : Vector3 velocity;

	public : NodeProperties() {};
	public : NodeProperties(float im) : invMass(im), velocity(Vector3(0,0,0)) {};
	public : void processAttributes() {};
	public : void registerAttributes()
	{
		REGISTER_ATTRIBUTE(invMass);
		REGISTER_ATTRIBUTE(velocity);
	};
	REGISTER_CLASS_NAME(NodeProperties);
};
REGISTER_CLASS(NodeProperties,false);*/

class FEMRock : public Serializable
{
	public : vector<Vector3> nodes;
	public : vector<vector<int> > tetrahedrons;

	// construction
	public : FEMRock ();
	public : ~FEMRock ();

	public : void processAttributes();
	public : void registerAttributes();

	public : void exec();

	REGISTER_CLASS_NAME(FEMRock);
};

REGISTER_SERIALIZABLE(FEMRock,false);

#endif // __FEMROCK_H__

