#ifdef WIN32
#include <windows.h> // The Win32 versions of the GL header files require that you windows.h before gl.h/glu.h/glut.h, so that you get the #define types like WINGDIAPI and such
#endif

#include <GL/gl.h>
#include <GL/glut.h>
#include "Math.hpp"

#include "NonConnexBody.hpp"
#include "InteractionVecSet.hpp"
#include "BodyAssocVec.hpp"

// FIXME - who is to decide which class to use by default?
NonConnexBody::NonConnexBody() : Body() , bodies(new BodyAssocVec) , permanentInteractions(new InteractionVecSet)
{
}

NonConnexBody::~NonConnexBody()
{

}


void NonConnexBody::glDraw()
{

	glPushMatrix();

	float angle;
	Vector3r axis;
	se3.rotation.toAxisAngle(axis,angle);

	glTranslatef(se3.translation[0],se3.translation[1],se3.translation[2]);
	glRotated(angle*Mathr::RAD_TO_DEG,axis[0],axis[1],axis[2]);

	glDisable(GL_LIGHTING);

	for( bodies->gotoFirst() ; bodies->notAtEnd() ; bodies->gotoNext() )
		bodies->getCurrent()->glDraw();

	glPopMatrix();

}

void NonConnexBody::postProcessAttributes(bool)
{

}

void NonConnexBody::registerAttributes()
{
	Body::registerAttributes();

	REGISTER_ATTRIBUTE(permanentInteractions);
	REGISTER_ATTRIBUTE(bodies);
//	REGISTER_ATTRIBUTE(narrowCollider);
//	REGISTER_ATTRIBUTE(broadCollider);
//	REGISTER_ATTRIBUTE(kinematic);
}

void NonConnexBody::updateBoundingVolume(Se3r& se3)
{
	for( bodies->gotoFirst() ; bodies->notAtEnd() ; bodies->gotoNext() )
		bodies->getCurrent()->updateBoundingVolume(se3);
}

void NonConnexBody::updateCollisionGeometry(Se3r& )
{

}

void NonConnexBody::moveToNextTimeStep()
{

	vector<shared_ptr<Actor> >::iterator ai    = actors.begin();
	vector<shared_ptr<Actor> >::iterator aiEnd =  actors.end();
	for(;ai!=aiEnd;++ai)
		if ((*ai)->isActivated())
			(*ai)->action(this);

// FIND INTERACTIONS
	// serach for potential collision (maybe in to steps for hierarchical simulation)
//	if (broadCollider!=0)
//		broadCollider->broadCollisionTest(bodies,interactions);
	// this has to split the contact list into several constact list according to the physical type
	// (RigidBody,FEMBody ...) of colliding body
//	if (narrowCollider!=0)
//		narrowCollider->narrowCollisionPhase(bodies,interactions);
// MOTION
	// for each contact list we call the correct dynamic engine
	//if (dynamic!=0)
	//	dynamic->respondToCollisions(this,interactions); //effectiveDt == dynamic->...
	//if (kinematic!=0)
	//	kinematic->moveToNextTimeStep(bodies);

//	shared_ptr<Body> b;
//	for( b = bodies->getFirst() ; bodies->hasCurrent() ; b = bodies->getNext() )
//		b->moveToNextTimeStep();

	// FIXME : do we need to walk through the tree of objects in a different way than below  (recursively in NCB... ) ??
	for( bodies->gotoFirst() ; bodies->notAtEnd() ; bodies->gotoNext() )
		bodies->getCurrent()->moveToNextTimeStep();

}
