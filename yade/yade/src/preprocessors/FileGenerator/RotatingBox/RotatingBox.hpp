
#ifndef __ROTATINGBOX_H__
#define __ROTATINGBOX_H__

#include "FileGenerator.hpp"

class RotatingBox : public FileGenerator
{
	private : Vector3r nbSpheres;
	private : Vector3r nbBoxes;
	private	: Real minSize;
	private	: Real maxSize;
	private	: Real disorder;
	private : Real dampingForce;
	private : Real dampingMomentum;
	
	// construction
	public : RotatingBox ();
	public : ~RotatingBox ();
	
	private : void createKinematicBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents,bool);
	private : void createSphere(shared_ptr<Body>& body, int i, int j, int k);
	private : void createBox(shared_ptr<Body>& body, int i, int j, int k);
	private : void createActors(shared_ptr<ComplexBody>& rootBody);
	private : void positionRootBody(shared_ptr<ComplexBody>& rootBody);

	public : virtual void registerAttributes();
	public : virtual string generate();

	REGISTER_CLASS_NAME(RotatingBox);
};

REGISTER_SERIALIZABLE(RotatingBox,false);

#endif // __ROTATINGBOX_H__

