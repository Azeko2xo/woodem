#include "SDECImport.hpp"

#include "Box.hpp"
#include "AABB.hpp"
#include "Sphere.hpp"
#include "ComplexBody.hpp"
#include "SAPCollider.hpp"
#include "PersistentSAPCollider.hpp"
#include <fstream>
#include "IOManager.hpp"
#include "Interaction.hpp"
#include "BoundingVolumeDispatcher.hpp"
#include "InteractionDescriptionSet2AABBFunctor.hpp"
#include "InteractionDescriptionSet.hpp"

#include "SDECDynamicEngine.hpp"
#include "SDECMacroMicroElasticRelationships.hpp"
#include "SDECParameters.hpp"
#include "SDECLinkGeometry.hpp"
#include "SDECLinkPhysics.hpp"
#include "SDECTimeStepper.hpp"

#include "ActionDispatcher.hpp"
#include "ActionDispatcher.hpp"
#include "CundallNonViscousForceDamping.hpp"
#include "CundallNonViscousMomentumDamping.hpp"

#include "InteractionGeometryDispatcher.hpp"
#include "InteractionPhysicsDispatcher.hpp"
#include "SimpleBody.hpp"
#include "InteractionBox.hpp"
#include "InteractionSphere.hpp"
#include "TimeIntegratorDispatcher.hpp"

#include "ActionReset.hpp"

#include "AveragePositionRecorder.hpp"
#include "ForceRecorder.hpp"

#include <boost/filesystem/convenience.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost;
using namespace std;

SDECImport::SDECImport () : FileGenerator()
{
	lowerCorner 		= Vector3r(1000,1000,1000);
	upperCorner 		= Vector3r(-1000,-1000,-1000);
	thickness 		= 0.01;
	importFilename 		= "../data/small.sdec.xyz";
	wall_top 		= false;
	wall_bottom 		= true;
	wall_1			= true;
	wall_2			= true;
	wall_3			= true;
	wall_4			= true;
	wall_top_wire 		= true;
	wall_bottom_wire	= true;
	wall_1_wire		= true;
	wall_2_wire		= true;
	wall_3_wire		= true;
	wall_4_wire		= true;
	spheresColor		= Vector3f(0.8,0.3,0.3);
	spheresRandomColor	= false;
	recordBottomForce	= true;
	forceRecordFile		= "../data/force";
	recordAveragePositions	= true;
	positionRecordFile	= "../data/position";
	recordIntervalIter	= 100;

	bigBallRadius		= 0.15;
	bigBallDensity		= 7000;
	bigBallDropTimeSeconds	= 450;
	
	dampingForce = 0.7;
	dampingMomentum = 0.7;
	timeStepUpdateInterval = 300;
	
	sphereYoungModulus  = 15000000.0;
	spherePoissonRatio  = 0.2;
	sphereFrictionDeg   = 18.0;
	density			= 2600;
	
	boxYoungModulus   = 15000000.0;
	boxPoissonRatio  = 0.2;
	boxFrictionDeg   = -20.0;
}

SDECImport::~SDECImport ()
{

}


void SDECImport::registerAttributes()
{
//	REGISTER_ATTRIBUTE(lowerCorner);
//	REGISTER_ATTRIBUTE(upperCorner);
//	REGISTER_ATTRIBUTE(thickness);
	REGISTER_ATTRIBUTE(importFilename);

	REGISTER_ATTRIBUTE(sphereYoungModulus);
	REGISTER_ATTRIBUTE(spherePoissonRatio);
	REGISTER_ATTRIBUTE(sphereFrictionDeg);

	REGISTER_ATTRIBUTE(boxYoungModulus);
	REGISTER_ATTRIBUTE(boxPoissonRatio);
	REGISTER_ATTRIBUTE(boxFrictionDeg);

	REGISTER_ATTRIBUTE(density);
	REGISTER_ATTRIBUTE(dampingForce);
	REGISTER_ATTRIBUTE(dampingMomentum);
	REGISTER_ATTRIBUTE(timeStepUpdateInterval);
//	REGISTER_ATTRIBUTE(wall_top);
//	REGISTER_ATTRIBUTE(wall_bottom);
//	REGISTER_ATTRIBUTE(wall_1);
//	REGISTER_ATTRIBUTE(wall_2);
//	REGISTER_ATTRIBUTE(wall_3);
//	REGISTER_ATTRIBUTE(wall_4);
//	REGISTER_ATTRIBUTE(wall_top_wire);
//	REGISTER_ATTRIBUTE(wall_bottom_wire);
//	REGISTER_ATTRIBUTE(wall_1_wire);
//	REGISTER_ATTRIBUTE(wall_2_wire);
//	REGISTER_ATTRIBUTE(wall_3_wire);
//	REGISTER_ATTRIBUTE(wall_4_wire);
//	REGISTER_ATTRIBUTE(spheresColor);
//	REGISTER_ATTRIBUTE(spheresRandomColor);
	REGISTER_ATTRIBUTE(recordBottomForce);
	REGISTER_ATTRIBUTE(forceRecordFile);
	REGISTER_ATTRIBUTE(recordAveragePositions);
	REGISTER_ATTRIBUTE(positionRecordFile);
	REGISTER_ATTRIBUTE(recordIntervalIter);

	REGISTER_ATTRIBUTE(bigBallRadius);
	REGISTER_ATTRIBUTE(bigBallDensity);
	REGISTER_ATTRIBUTE(bigBallDropTimeSeconds);

}

string SDECImport::generate()
{
	rootBody = shared_ptr<ComplexBody>(new ComplexBody);
	createActors(rootBody);
	positionRootBody(rootBody);

	shared_ptr<Body> body;
	if(importFilename.size() != 0 && filesystem::exists(importFilename) )
	{
		ifstream loadFile(importFilename.c_str());
		long int i=0;
		Real f,g,x,y,z,radius;
		while( ! loadFile.eof() )
		{
			++i;
			loadFile >> x;
			loadFile >> y;
			loadFile >> z;
			Vector3r translation = Vector3r(x,z,y);
			loadFile >> radius;
		
			loadFile >> f;
			loadFile >> g;
			if(f != 1) // skip loading of SDEC walls
				continue;

			if( i % 100 == 0 ) // FIXME - should display a progress BAR !!
				cout << "loaded: " << i << endl;
			
			lowerCorner[0] = min(translation[0]-radius , lowerCorner[0]);
			lowerCorner[1] = min(translation[1]-radius , lowerCorner[1]);
			lowerCorner[2] = min(translation[2]-radius , lowerCorner[2]);
			upperCorner[0] = max(translation[0]+radius , upperCorner[0]);
			upperCorner[1] = max(translation[1]+radius , upperCorner[1]);
			upperCorner[2] = max(translation[2]+radius , upperCorner[2]);
			
			createSphere(body,translation,radius);			
			rootBody->bodies->insert(body);
		}
	}

// create bigBall
	Vector3r translation = (upperCorner+lowerCorner)*0.5 + Vector3r(0,upperCorner[1]+bigBallRadius+4,0);
	createSphere(body,translation,bigBallRadius,true);	
	body->isDynamic = false;
	rootBody->bodies->insert(body);
	int bigId = body->getId();

// bottom box
 	Vector3r center		= Vector3r(
 						(lowerCorner[0]+upperCorner[0])/2,
 						lowerCorner[1]-thickness/2.0,
 						(lowerCorner[2]+upperCorner[2])/2);
 	Vector3r halfSize	= Vector3r(
 						fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
						thickness/2.0,
 						fabs(lowerCorner[2]-upperCorner[2])/2+thickness);

	createBox(body,center,halfSize,wall_bottom_wire);
 	if(wall_bottom)
		rootBody->bodies->insert(body);
	forcerec->id = body->getId();
	forcerec->bigBallId = bigId;
	forcerec->bigBallReleaseTime = bigBallDropTimeSeconds;

// top box
 	center			= Vector3r(
 						(lowerCorner[0]+upperCorner[0])/2,
 						upperCorner[1]+thickness/2.0,
 						(lowerCorner[2]+upperCorner[2])/2);
 	halfSize		= Vector3r(
 						fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
 						thickness/2.0,
 						fabs(lowerCorner[2]-upperCorner[2])/2+thickness);

	createBox(body,center,halfSize,wall_top_wire);
 	if(wall_top)
		rootBody->bodies->insert(body);
// box 1

 	center			= Vector3r(
 						lowerCorner[0]-thickness/2.0,
 						(lowerCorner[1]+upperCorner[1])/2,
 						(lowerCorner[2]+upperCorner[2])/2);
	halfSize		= Vector3r(
						thickness/2.0,
 						fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
 						fabs(lowerCorner[2]-upperCorner[2])/2+thickness);
	createBox(body,center,halfSize,wall_1_wire);
 	if(wall_1)
		rootBody->bodies->insert(body);
// box 2
 	center			= Vector3r(
 						upperCorner[0]+thickness/2.0,
 						(lowerCorner[1]+upperCorner[1])/2,
						(lowerCorner[2]+upperCorner[2])/2);
 	halfSize		= Vector3r(
 						thickness/2.0,
 						fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
 						fabs(lowerCorner[2]-upperCorner[2])/2+thickness);
 	
	createBox(body,center,halfSize,wall_2_wire);
 	if(wall_2)
		rootBody->bodies->insert(body);
// box 3
 	center			= Vector3r(
 						(lowerCorner[0]+upperCorner[0])/2,
 						(lowerCorner[1]+upperCorner[1])/2,
 						lowerCorner[2]-thickness/2.0);
 	halfSize		= Vector3r(
 						fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
 						fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
 						thickness/2.0);
	createBox(body,center,halfSize,wall_3_wire);
 	if(wall_3)
 		rootBody->bodies->insert(body);

// box 4
 	center			= Vector3r(
 						(lowerCorner[0]+upperCorner[0])/2,
 						(lowerCorner[1]+upperCorner[1])/2,
 						upperCorner[2]+thickness/2.0);
 	halfSize		= Vector3r(
 						fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
 						fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
 						thickness/2.0);
	createBox(body,center,halfSize,wall_3_wire);
 	if(wall_4)
 		rootBody->bodies->insert(body);

 	return "Generated a sample inside box of dimensions: (" 
 		+ lexical_cast<string>(lowerCorner[0]) + "," 
 		+ lexical_cast<string>(lowerCorner[1]) + "," 
 		+ lexical_cast<string>(lowerCorner[2]) + ") and (" 
 		+ lexical_cast<string>(upperCorner[0]) + "," 
 		+ lexical_cast<string>(upperCorner[1]) + "," 
 		+ lexical_cast<string>(upperCorner[2]) + ").";

}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void SDECImport::createSphere(shared_ptr<Body>& body, Vector3r translation, Real radius, bool big )
{
	body = shared_ptr<Body>(new SimpleBody(0,10));
	shared_ptr<SDECParameters> physics(new SDECParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Sphere> gSphere(new Sphere);
	shared_ptr<InteractionSphere> iSphere(new InteractionSphere);
	
	Quaternionr q;
	q.fromAxisAngle( Vector3r(0,0,1),0);
	
	body->isDynamic			= true;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= 4.0/3.0*Mathr::PI*radius*radius*(big ? bigBallDensity : density);
	physics->inertia		= Vector3r( 	2.0/5.0*physics->mass*radius*radius,
							2.0/5.0*physics->mass*radius*radius,
							2.0/5.0*physics->mass*radius*radius);
	physics->se3			= Se3r(translation,q);
	physics->young			= sphereYoungModulus;
	physics->poisson		= spherePoissonRatio;
	physics->frictionAngle		= sphereFrictionDeg * Mathr::PI/180.0;

	aabb->diffuseColor		= Vector3r(0,1,0);


	gSphere->radius			= radius;
	gSphere->diffuseColor		= spheresColor;
	gSphere->wire			= false;
	gSphere->visible		= true;
	gSphere->shadowCaster		= true;
	
	iSphere->radius			= radius;
	iSphere->diffuseColor		= Vector3f(Mathf::unitRandom(),Mathf::unitRandom(),Mathf::unitRandom());

	body->interactionGeometry	= iSphere;
	body->geometricalModel		= gSphere;
	body->boundingVolume		= aabb;
	body->physicalParameters	= physics;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void SDECImport::createBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents, bool wire)
{
	body = shared_ptr<Body>(new SimpleBody(0,10));
	shared_ptr<SDECParameters> physics(new SDECParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Box> gBox(new Box);
	shared_ptr<InteractionBox> iBox(new InteractionBox);
	
	Quaternionr q;
	q.fromAxisAngle( Vector3r(0,0,1),0);

	body->isDynamic			= false;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= extents[0]*extents[1]*extents[2]*density*2; 
	physics->inertia		= Vector3r(
							  physics->mass*(extents[1]*extents[1]+extents[2]*extents[2])/3
							, physics->mass*(extents[0]*extents[0]+extents[2]*extents[2])/3
							, physics->mass*(extents[1]*extents[1]+extents[0]*extents[0])/3
						);
//	physics->mass			= 0;
//	physics->inertia		= Vector3r(0,0,0);
	physics->se3			= Se3r(position,q);

	physics->young			= boxYoungModulus;
	physics->poisson		= boxPoissonRatio;
	physics->frictionAngle		= boxFrictionDeg * Mathr::PI/180.0;

	aabb->diffuseColor		= Vector3r(1,0,0);

	gBox->extents			= extents;
	gBox->diffuseColor		= Vector3f(1,1,1);
	gBox->wire			= wire;
	gBox->visible			= true;
	gBox->shadowCaster		= false;
	
	iBox->extents			= extents;
	iBox->diffuseColor		= Vector3f(1,1,1);

	body->boundingVolume		= aabb;
	body->interactionGeometry	= iBox;
	body->geometricalModel		= gBox;
	body->physicalParameters	= physics;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void SDECImport::createActors(shared_ptr<ComplexBody>& rootBody)
{
	shared_ptr<AveragePositionRecorder> averagePositionRecorder = shared_ptr<AveragePositionRecorder>(new AveragePositionRecorder);
	averagePositionRecorder -> outputFile 		= positionRecordFile;
	averagePositionRecorder -> interval 		= recordIntervalIter;
	
//	Recording Forces
	forcerec = shared_ptr<ForceRecorder>(new ForceRecorder);
	forcerec -> outputFile 	= forceRecordFile;
	forcerec -> interval 	= recordIntervalIter;

	shared_ptr<InteractionGeometryDispatcher> interactionGeometryDispatcher(new InteractionGeometryDispatcher);
	interactionGeometryDispatcher->add("InteractionSphere","InteractionSphere","Sphere2Sphere4SDECContactModel");
	interactionGeometryDispatcher->add("InteractionSphere","InteractionBox","Box2Sphere4SDECContactModel");

	shared_ptr<InteractionPhysicsDispatcher> interactionPhysicsDispatcher(new InteractionPhysicsDispatcher);
	interactionPhysicsDispatcher->add("SDECParameters","SDECParameters","SDECMacroMicroElasticRelationships");
		
	shared_ptr<BoundingVolumeDispatcher> boundingVolumeDispatcher	= shared_ptr<BoundingVolumeDispatcher>(new BoundingVolumeDispatcher);
	boundingVolumeDispatcher->add("InteractionSphere","AABB","Sphere2AABBFunctor");
	boundingVolumeDispatcher->add("InteractionBox","AABB","Box2AABBFunctor");
	boundingVolumeDispatcher->add("InteractionDescriptionSet","AABB","InteractionDescriptionSet2AABBFunctor");

	

		
	shared_ptr<CundallNonViscousForceDamping> actionForceDamping(new CundallNonViscousForceDamping);
	actionForceDamping->damping = dampingForce;
	shared_ptr<CundallNonViscousMomentumDamping> actionMomentumDamping(new CundallNonViscousMomentumDamping);
	actionMomentumDamping->damping = dampingMomentum;
	shared_ptr<ActionDispatcher> actionDampingDispatcher(new ActionDispatcher);
	actionDampingDispatcher->add("ActionForce","ParticleParameters","CundallNonViscousForceDamping",actionForceDamping);
	actionDampingDispatcher->add("ActionMomentum","RigidBodyParameters","CundallNonViscousMomentumDamping",actionMomentumDamping);
	
	shared_ptr<ActionDispatcher> applyActionDispatcher(new ActionDispatcher);
	applyActionDispatcher->add("ActionForce","ParticleParameters","ApplyActionForce2Particle");
	applyActionDispatcher->add("ActionMomentum","RigidBodyParameters","ApplyActionMomentum2RigidBody");
	
	shared_ptr<TimeIntegratorDispatcher> timeIntegratorDispatcher(new TimeIntegratorDispatcher);
	
	
	timeIntegratorDispatcher->add("SDECParameters","LeapFrogIntegrator"); // FIXME - bug in locateMultivirtualFunctionCall1D ???
		
	
	shared_ptr<SDECTimeStepper> sdecTimeStepper(new SDECTimeStepper);
	sdecTimeStepper->sdecGroup = 10;
	sdecTimeStepper->interval = timeStepUpdateInterval;
	
	
	shared_ptr<SDECDynamicEngine> sdecDynamicEngine(new SDECDynamicEngine);
	sdecDynamicEngine->sdecGroup = 10;
	
	rootBody->actors.clear();
	rootBody->actors.push_back(shared_ptr<Actor>(new ActionReset));
	rootBody->actors.push_back(sdecTimeStepper);
	rootBody->actors.push_back(boundingVolumeDispatcher);
	rootBody->actors.push_back(shared_ptr<Actor>(new PersistentSAPCollider));
	rootBody->actors.push_back(interactionGeometryDispatcher);
	rootBody->actors.push_back(interactionPhysicsDispatcher);
	rootBody->actors.push_back(sdecDynamicEngine);
	rootBody->actors.push_back(actionDampingDispatcher);
	rootBody->actors.push_back(applyActionDispatcher);
	rootBody->actors.push_back(timeIntegratorDispatcher);
	rootBody->actors.push_back(averagePositionRecorder);
	rootBody->actors.push_back(forcerec);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void SDECImport::positionRootBody(shared_ptr<ComplexBody>& rootBody)
{
	rootBody->isDynamic		= false;

	Quaternionr q;
	q.fromAxisAngle( Vector3r(0,0,1),0);
	shared_ptr<ParticleParameters> physics(new ParticleParameters); // FIXME : fix indexable class BodyPhysicalParameters
	physics->se3			= Se3r(Vector3r(0,0,0),q);
	physics->mass			= 0;
	physics->velocity		= Vector3r::ZERO;
	physics->acceleration		= Vector3r::ZERO;
	
	shared_ptr<InteractionDescriptionSet> set(new InteractionDescriptionSet());
	
	set->diffuseColor		= Vector3f(0,0,1);

	shared_ptr<AABB> aabb(new AABB);
	aabb->diffuseColor		= Vector3r(0,0,1);
	
	rootBody->interactionGeometry	= dynamic_pointer_cast<InteractionDescription>(set);	
	rootBody->boundingVolume	= dynamic_pointer_cast<BoundingVolume>(aabb);
	rootBody->physicalParameters 	= physics;
	
}
