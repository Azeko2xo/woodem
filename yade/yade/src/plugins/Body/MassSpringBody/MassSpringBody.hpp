#ifndef __MASSPSRINGBODY_H__
#define __MASSPSRINGBODY_H__

#include "ConnexBody.hpp"
#include "Vector3.hpp"
#include "Matrix3.hpp"

class NodeProperties : public Serializable
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
REGISTER_SERIALIZABLE(NodeProperties,true);

class MassSpringBody : public ConnexBody
{
	public : float stiffness;
	public : float damping;
 	public : vector<NodeProperties> properties;
	public : vector<float> initialLengths;
	public : vector<pair<int,Vector3> > externalForces;

	// construction
	public : MassSpringBody ();
	public : ~MassSpringBody ();

	public : void processAttributes();
	public : void registerAttributes();

	public : void updateBoundingVolume(Se3& se3);
	public : void updateCollisionModel(Se3& se3);

	REGISTER_CLASS_NAME(MassSpringBody);
};

REGISTER_SERIALIZABLE(MassSpringBody,false);

#endif // __MASSPSRINGBODY_H__
