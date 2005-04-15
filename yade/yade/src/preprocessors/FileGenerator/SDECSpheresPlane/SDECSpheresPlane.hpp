
#ifndef SDEC_SPHERES_PLANE_HPP
#define SDEC_SPHERES_PLANE_HPP

#include "FileGenerator.hpp"

class SDECSpheresPlane : public FileGenerator
{
	private : Vector3r nbSpheres;
	private : Real minRadius,density;
	private : Vector3r groundSize,gravity;
	private : Real maxRadius;
	private : Real dampingForce;
	private	: Real disorder;
	private : Real dampingMomentum;
	private : int timeStepUpdateInterval;
	private : bool rotationBlocked;
	private : Real sphereYoungModulus,spherePoissonRatio,sphereFrictionDeg;
	// construction
	public : SDECSpheresPlane ();
	public : ~SDECSpheresPlane ();


	private : void createBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents);
	private : void createSphere(shared_ptr<Body>& body, int i, int j, int k);
	private : void createActors(shared_ptr<ComplexBody>& rootBody);
	private : void positionRootBody(shared_ptr<ComplexBody>& rootBody);

	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();

	public : string generate();

	REGISTER_CLASS_NAME(SDECSpheresPlane);
};

REGISTER_SERIALIZABLE(SDECSpheresPlane,false);

#endif // SDEC_SPHERES_PLANE_HPP 

