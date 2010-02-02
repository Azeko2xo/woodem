/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2005 by Andreas Plesch                                  *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/


#include "SDECMovingWall.hpp"

#include<yade/pkg-dem/BodyMacroParameters.hpp>
#include<yade/pkg-dem/ElasticContactLaw.hpp>
#include<yade/pkg-dem/MacroMicroElasticRelationships.hpp>
#include<yade/pkg-dem/ElasticCriterionTimeStepper.hpp>
#include<yade/pkg-dem/PositionOrientationRecorder.hpp>


#include<yade/pkg-common/BoxModel.hpp>
#include<yade/pkg-common/Aabb.hpp>
#include<yade/pkg-common/SphereModel.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg-common/InsertionSortCollider.hpp>
#include<yade/lib-serialization/IOFormatManager.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/pkg-common/BoundDispatcher.hpp>

#include<yade/pkg-common/ForceResetter.hpp>

#include<yade/pkg-common/PhysicalActionDamper.hpp>
#include<yade/pkg-common/PhysicalActionApplier.hpp>

#include<yade/pkg-common/CundallNonViscousDamping.hpp>
#include<yade/pkg-common/CundallNonViscousDamping.hpp>
#include<yade/pkg-common/GravityEngines.hpp>

#include<yade/pkg-common/TranslationEngine.hpp>

#include<yade/pkg-common/InteractionGeometryDispatcher.hpp>
#include<yade/pkg-common/InteractionPhysicsDispatcher.hpp>
#include<yade/core/Body.hpp>
#include<yade/pkg-common/Box.hpp>
#include<yade/pkg-common/Sphere.hpp>
#include<yade/pkg-common/StateMetaEngine.hpp>

YADE_REQUIRE_FEATURE(geometricalmodel)

SDECMovingWall::SDECMovingWall () : FileGenerator()
{
	nbSpheres = Vector3r(40,4,4);
	minRadius = 3;
	maxRadius = 4;
	groundSize = Vector3r(200,5,200);
	groundPosition = Vector3r(0,0,0);
	wallSize = Vector3r(5,200,200);
	wallPosition = Vector3r(-200,205,0);
	wallVelocity = 5;
	wallTranslationAxis = Vector3r(1,0,0);
	side1Size = Vector3r(5,200,200);
	side1Position = Vector3r(200,205,0);
	side1wire = false;
	side2Size = Vector3r(200,200,5);
	side2Position = Vector3r(0,205,-30);
	side2wire = true;
	side3Size = Vector3r(200,200,5);
	side3Position = Vector3r(0,205,30);
	side3wire = true;
	dampingForce = 0.3;
	dampingMomentum = 0.3;
	timeStepUpdateInterval = 300;
	sphereYoungModulus   = 15000000.0;
	//sphereYoungModulus   = 10000;
	spherePoissonRatio  = 0.2;
	sphereFrictionDeg   = 18.0;
	density = 2600;
	rotationBlocked = false;
	gravity = Vector3r(0,-9.81,0);
	disorder = 0.2;
       	useSpheresAsGround = false;
	spheresHeight = 0;
}


SDECMovingWall::~SDECMovingWall ()
{

}


void SDECMovingWall::postProcessAttributes(bool)
{

}




bool SDECMovingWall::generate()
{
	rootBody = shared_ptr<Scene>(new Scene);
	createActors(rootBody);
	positionRootBody(rootBody);

		
////////////////////////////////////
///////// ground

	if (!useSpheresAsGround)
	{
		shared_ptr<Body> ground;
		shared_ptr<Body> wall;
		shared_ptr<Body> side1;
		shared_ptr<Body> side2;
		shared_ptr<Body> side3;
		createBox(ground, groundPosition, groundSize, false);
		createBox(wall, wallPosition, wallSize, false);
		createBox(side1, side1Position, side1Size, side1wire);
		createBox(side2, side2Position, side2Size, side2wire);
		createBox(side3, side3Position, side3Size, side3wire);
		rootBody->bodies->insert(ground);
		rootBody->bodies->insert(wall);
		rootBody->bodies->insert(side1);
		rootBody->bodies->insert(side2);
		rootBody->bodies->insert(side3);
	}
	else
	{
		int nbSpheresi,nbSpheresj,nbSpheresk;
		Real radius = groundSize[0];

		if (groundSize[1]<radius)
			radius = groundSize[1];
		if (groundSize[2]<radius)
			radius = groundSize[2];

		nbSpheresi = (int)(groundSize[0]/radius);
		nbSpheresj = (int)(groundSize[1]/radius);
		nbSpheresk = (int)(groundSize[2]/radius);

		for(int i=0;i<nbSpheresi;i++)
			for(int j=0;j<nbSpheresj;j++)
				for(int k=0;k<nbSpheresk;k++)
				{
					shared_ptr<Body> sphere;
					createGroundSphere(sphere,radius,radius*(i-nbSpheresi*0.5+0.5),radius*(j-nbSpheresj*0.5+0.5),radius*(k-nbSpheresk*0.5+0.5));
					rootBody->bodies->insert(sphere);
				}
	}

///////// spheres

	for(int i=0;i<nbSpheres[0];i++)
		for(int j=0;j<nbSpheres[1];j++)
			for(int k=0;k<nbSpheres[2];k++)
			{
				shared_ptr<Body> sphere;
				createSphere(sphere,i,j,k);
				rootBody->bodies->insert(sphere);
			}
	
	message=""; return true;
}


void SDECMovingWall::createGroundSphere(shared_ptr<Body>& body,Real radius, Real i, Real j, Real k)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),1));
	shared_ptr<BodyMacroParameters> physics(new BodyMacroParameters);
	shared_ptr<Aabb> aabb(new Aabb);
	shared_ptr<SphereModel> gSphere(new SphereModel);
	shared_ptr<Sphere> iSphere(new Sphere);
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);
	
	Vector3r position		= Vector3r(i,j,k);
	
	body->isDynamic			= false;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= 0;
	physics->inertia		= Vector3r(0,0,0);
	physics->se3			= Se3r(position,q);
	physics->young			= sphereYoungModulus;
	physics->poisson		= spherePoissonRatio;
	physics->frictionAngle		= sphereFrictionDeg * Mathr::PI/180.0;

	aabb->diffuseColor		= Vector3r(0,1,0);

	gSphere->radius			= radius;
	gSphere->diffuseColor		= Vector3r(0.7,0.7,0.7);
	gSphere->wire			= false;
	gSphere->shadowCaster		= true;
	
	iSphere->radius			= radius;
	iSphere->diffuseColor		= Vector3r(0.8,0.3,0.3);

	body->shape	= iSphere;
	body->geometricalModel		= gSphere;
	body->bound		= aabb;
	body->physicalParameters	= physics;
}


void SDECMovingWall::createSphere(shared_ptr<Body>& body, int i, int j, int k)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),1));
	shared_ptr<BodyMacroParameters> physics(new BodyMacroParameters);
	shared_ptr<Aabb> aabb(new Aabb);
	shared_ptr<SphereModel> gSphere(new SphereModel);
	shared_ptr<Sphere> iSphere(new Sphere);
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);
	
	Vector3r position		= Vector3r(i,j+spheresHeight,k)*(2*maxRadius*1.1) // this formula is crazy !!
					  - Vector3r( nbSpheres[0]/2*(2*maxRadius*1.1) , -7-maxRadius*2 , nbSpheres[2]/2*(2*maxRadius*1.1) )
					  + Vector3r(Mathr::SymmetricRandom(),Mathr::SymmetricRandom(),Mathr::SymmetricRandom())*disorder*maxRadius;
	Real radius 			= (Mathr::IntervalRandom(minRadius,maxRadius));
	
	body->isDynamic			= true;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= 4.0/3.0*Mathr::PI*radius*radius*radius*density;
	physics->inertia		= Vector3r(2.0/5.0*physics->mass*radius*radius,2.0/5.0*physics->mass*radius*radius,2.0/5.0*physics->mass*radius*radius); //
	physics->se3			= Se3r(position,q);
	physics->young			= sphereYoungModulus;
	physics->poisson		= spherePoissonRatio;
	physics->frictionAngle		= sphereFrictionDeg * Mathr::PI/180.0;

	aabb->diffuseColor		= Vector3r(0,1,0);

	gSphere->radius			= radius;
// this gives a pretty nice radial color effect
//	position.normalize();
//	gSphere->diffuseColor		= position;
// a simple way to have alternating colors per layer
	gSphere->diffuseColor		= Vector3r(std::sin((float)j),std::cos((float)j),j/nbSpheres[1]);
	gSphere->wire			= false;
	gSphere->shadowCaster		= true;
	
	iSphere->radius			= radius;
	iSphere->diffuseColor		= Vector3r(0.8,0.3,0.3);

	body->shape	= iSphere;
	body->geometricalModel		= gSphere;
	body->bound		= aabb;
	body->physicalParameters	= physics;
}


void SDECMovingWall::createBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents, bool wire)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),1));
	shared_ptr<BodyMacroParameters> physics(new BodyMacroParameters);
	shared_ptr<Aabb> aabb(new Aabb);
	shared_ptr<BoxModel> gBox(new BoxModel);
	shared_ptr<Box> iBox(new Box);
	
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);

	body->isDynamic			= false;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= extents[0]*extents[1]*extents[2]*density*2; 
	physics->inertia		= Vector3r(
							  physics->mass*(extents[1]*extents[1]+extents[2]*extents[2])/3
							, physics->mass*(extents[0]*extents[0]+extents[2]*extents[2])/3
							, physics->mass*(extents[1]*extents[1]+extents[0]*extents[0])/3
						);
	//physics->mass			= 0;
	//physics->inertia		= Vector3r(0,0,0);
	physics->se3			= Se3r(position,q);
	physics->young			= sphereYoungModulus;
	physics->poisson		= spherePoissonRatio;
	physics->frictionAngle		= sphereFrictionDeg * Mathr::PI/180.0;

	aabb->diffuseColor		= Vector3r(1,0,0);

	gBox->extents			= extents;
	gBox->diffuseColor		= Vector3r(1,1,1);
	gBox->wire			= wire;
	gBox->shadowCaster		= true;
	
	iBox->extents			= extents;
	iBox->diffuseColor		= Vector3r(1,1,1);

	body->bound		= aabb;
	body->shape	= iBox;
	body->geometricalModel		= gBox;
	body->physicalParameters	= physics;
}


void SDECMovingWall::createActors(shared_ptr<Scene>& rootBody)
{
	shared_ptr<InteractionGeometryDispatcher> interactionGeometryDispatcher(new InteractionGeometryDispatcher);
	interactionGeometryDispatcher->add("Ig2_Sphere_Sphere_ScGeom");
	interactionGeometryDispatcher->add("Ig2_Box_Sphere_ScGeom");

	shared_ptr<InteractionPhysicsDispatcher> interactionPhysicsDispatcher(new InteractionPhysicsDispatcher);
	interactionPhysicsDispatcher->add("MacroMicroElasticRelationships");
		
	shared_ptr<BoundDispatcher> boundDispatcher	= shared_ptr<BoundDispatcher>(new BoundDispatcher);
	boundDispatcher->add("Bo1_Sphere_Aabb");
	boundDispatcher->add("Bo1_Box_Aabb");
	
	shared_ptr<GravityEngine> gravityCondition(new GravityEngine);
	gravityCondition->gravity = gravity;
	
	shared_ptr<CundallNonViscousForceDamping> actionForceDamping(new CundallNonViscousForceDamping);
	actionForceDamping->damping = dampingForce;
	shared_ptr<CundallNonViscousMomentumDamping> actionMomentumDamping(new CundallNonViscousMomentumDamping);
	actionMomentumDamping->damping = dampingMomentum;
	shared_ptr<PhysicalActionDamper> actionDampingDispatcher(new PhysicalActionDamper);
	actionDampingDispatcher->add(actionForceDamping);
	actionDampingDispatcher->add(actionMomentumDamping);
	
	shared_ptr<PhysicalActionApplier> applyActionDispatcher(new PhysicalActionApplier);
	applyActionDispatcher->add("NewtonsForceLaw");
	applyActionDispatcher->add("NewtonsMomentumLaw");
	
	shared_ptr<StateMetaEngine> positionIntegrator(new StateMetaEngine);
	positionIntegrator->add("LeapFrogPositionIntegrator");
	shared_ptr<StateMetaEngine> orientationIntegrator(new StateMetaEngine);
	orientationIntegrator->add("LeapFrogOrientationIntegrator");
// moving wall
	shared_ptr<TranslationEngine> kinematic = shared_ptr<TranslationEngine>(new TranslationEngine);
	kinematic->velocity  = wallVelocity;
	wallTranslationAxis.Normalize();
	kinematic->translationAxis  = wallTranslationAxis;

	kinematic->subscribedBodies.push_back(1); //wall should be second inserted body, id=1

	shared_ptr<ElasticCriterionTimeStepper> sdecTimeStepper(new ElasticCriterionTimeStepper);
	sdecTimeStepper->sdecGroupMask = 1;
	sdecTimeStepper->timeStepUpdateInterval = timeStepUpdateInterval;
	
	rootBody->engines.clear();
	rootBody->engines.push_back(shared_ptr<Engine>(new ForceResetter));
	rootBody->engines.push_back(sdecTimeStepper);
	rootBody->engines.push_back(boundDispatcher);	
	rootBody->engines.push_back(shared_ptr<Engine>(new InsertionSortCollider));
	rootBody->engines.push_back(interactionGeometryDispatcher);
	rootBody->engines.push_back(interactionPhysicsDispatcher);
	rootBody->engines.push_back(shared_ptr<Engine>(new ElasticContactLaw));
	rootBody->engines.push_back(gravityCondition);
	rootBody->engines.push_back(actionDampingDispatcher);
	rootBody->engines.push_back(applyActionDispatcher);
	rootBody->engines.push_back(positionIntegrator);

	if(!rotationBlocked)
		rootBody->engines.push_back(orientationIntegrator);

	shared_ptr<PositionOrientationRecorder> positionOrientationRecorder(new PositionOrientationRecorder);
	//rootBody->engines.push_back(positionOrientationRecorder);

	rootBody->engines.push_back(kinematic);
	
	rootBody->initializers.clear();
	rootBody->initializers.push_back(boundDispatcher);
}


void SDECMovingWall::positionRootBody(shared_ptr<Scene>& rootBody) 
{
	rootBody->isDynamic		= false;
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);

	shared_ptr<ParticleParameters> physics(new ParticleParameters); // FIXME : fix indexable class PhysicalParameters
	physics->se3				= Se3r(Vector3r(0,0,0),q);
	physics->mass				= 0;
	physics->velocity			= Vector3r(0,0,0);
	physics->acceleration			= Vector3r::ZERO;
		
	
	shared_ptr<Aabb> aabb(new Aabb);
	aabb->diffuseColor			= Vector3r(0,0,1);
	
	rootBody->bound		= YADE_PTR_CAST<Bound>(aabb);
	rootBody->physicalParameters 		= physics;
}


YADE_PLUGIN((SDECMovingWall));

YADE_REQUIRE_FEATURE(PHYSPAR);

