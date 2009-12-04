/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

///
// FIXME : add CohesiveContactLaw engine to make it work
///


#include "SDECLinkedSpheres.hpp"

#include<yade/pkg-dem/ElasticContactLaw.hpp>
#include<yade/pkg-dem/ElasticCohesiveLaw.hpp>
#include<yade/pkg-dem/MacroMicroElasticRelationships.hpp>
#include<yade/pkg-dem/BodyMacroParameters.hpp>
#include<yade/pkg-dem/SDECLinkPhysics.hpp>
#include<yade/pkg-dem/SpheresContactGeometry.hpp>
#include<yade/pkg-dem/ElasticCriterionTimeStepper.hpp>


#include<yade/pkg-common/Box.hpp>
#include<yade/pkg-common/AABB.hpp>
#include<yade/pkg-common/Sphere.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg-common/InsertionSortCollider.hpp>
#include<yade/lib-serialization/IOFormatManager.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/pkg-common/BoundDispatcher.hpp>
#include<yade/pkg-common/MetaInteractingGeometry2AABB.hpp>
#include<yade/pkg-common/MetaInteractingGeometry.hpp>

#include<yade/pkg-common/PhysicalActionDamper.hpp>
#include<yade/pkg-common/PhysicalActionApplier.hpp>
#include<yade/pkg-common/CundallNonViscousDamping.hpp>
#include<yade/pkg-common/CundallNonViscousDamping.hpp>
#include<yade/pkg-common/GravityEngines.hpp>

#include<yade/pkg-common/InteractionGeometryDispatcher.hpp>
#include<yade/pkg-common/InteractionPhysicsDispatcher.hpp>
#include<yade/core/Body.hpp>
#include<yade/pkg-common/InteractingBox.hpp>
#include<yade/pkg-common/InteractingSphere.hpp>

#include<yade/pkg-common/PhysicalActionContainerReseter.hpp>
#include<yade/pkg-common/StateMetaEngine.hpp>

YADE_REQUIRE_FEATURE(geometricalmodel)

SDECLinkedSpheres::SDECLinkedSpheres () : FileGenerator()
{
	nbSpheres = Vector3r(2,2,20);
	minRadius = 5.01;
	maxRadius = 5.01;
	disorder = 0;
	spacing = 10;
	supportSize = 0.5;
	support1 = 1;
	support2 = 1;
	dampingForce = 0.2;
	dampingMomentum = 0.2;
	timeStepUpdateInterval = 300;
	//sphereYoungModulus  = 15000000.0;
	sphereYoungModulus  =   10000000;
	spherePoissonRatio  = 0.2;
	sphereFrictionDeg   = 18.0;
	density = 2.6;
	momentRotationLaw = true;
	gravity = Vector3r(0,-9.81,0);
}


SDECLinkedSpheres::~SDECLinkedSpheres ()
{

}


void SDECLinkedSpheres::postProcessAttributes(bool)
{
}



bool SDECLinkedSpheres::generate()
{
	rootBody = shared_ptr<Scene>(new Scene);
	createActors(rootBody);
	positionRootBody(rootBody);

	
////////////////////////////////////

	shared_ptr<Body> ground;
	shared_ptr<Body> supportBox1;
	shared_ptr<Body> supportBox2;
	
	createBox(ground, Vector3r(0,0,0), Vector3r(200,5,200));
	createBox(supportBox1, Vector3r(0,0,((Real)(nbSpheres[2])/2.0-supportSize+1.5)*spacing), Vector3r(20,50,20));
	createBox(supportBox2, Vector3r(0,0,-((Real)(nbSpheres[2])/2.0-supportSize+2.5)*spacing), Vector3r(20,50,20));
			
	rootBody->bodies->insert(ground);
	if (support1)
		rootBody->bodies->insert(supportBox1);
	if (support2)
		rootBody->bodies->insert(supportBox2);

/////////////////////////////////////

	for(int i=0;i<nbSpheres[0];i++)
		for(int j=0;j<nbSpheres[1];j++)
			for(int k=0;k<nbSpheres[2];k++)
			{
				shared_ptr<Body> sphere;
				createSphere(sphere,i,j,k);
				rootBody->bodies->insert(sphere);
			}
	
/////////////////////////////////////

	rootBody->interactions->clear();
	
	shared_ptr<Body> bodyA;

	BodyContainer::iterator bi    = rootBody->bodies->begin();
	BodyContainer::iterator biEnd = rootBody->bodies->end();
	BodyContainer::iterator bi2;

	++bi; // skips ground
	if (support1)
		++bi; // skips supportBox1
	if (support2)
		++bi; // skips supportBox2
		
		
	for( ; bi!=biEnd ; ++bi )
	{
		bodyA =*bi;
		bi2=bi;
		++bi2;
		for( ; bi2!=biEnd ; ++bi2 )
		{
			shared_ptr<Body> bodyB = *bi2;

			shared_ptr<BodyMacroParameters> a = YADE_PTR_CAST<BodyMacroParameters>(bodyA->physicalParameters);
			shared_ptr<BodyMacroParameters> b = YADE_PTR_CAST<BodyMacroParameters>(bodyB->physicalParameters);
			shared_ptr<InteractingSphere>	as = YADE_PTR_CAST<InteractingSphere>(bodyA->shape);
			shared_ptr<InteractingSphere>	bs = YADE_PTR_CAST<InteractingSphere>(bodyB->shape);

			if ((a->se3.position - b->se3.position).Length() < (as->radius + bs->radius))  
			{
				shared_ptr<Interaction> 		link(new Interaction( bodyA->getId() , bodyB->getId() ));
				shared_ptr<SpheresContactGeometry>		geometry(new SpheresContactGeometry);
				shared_ptr<SDECLinkPhysics>	physics(new SDECLinkPhysics);
				
				geometry->radius1			= as->radius - fabs(as->radius - bs->radius)*0.5;
				geometry->radius2			= bs->radius - fabs(as->radius - bs->radius)*0.5;

				physics->initialKn			= 50000000; // FIXME - BIG problem here.
				physics->initialKs			= 5000000;
				physics->heta				= 1;
				physics->initialEquilibriumDistance	= (a->se3.position - b->se3.position).Length();
				physics->knMax				= 5500000;
				physics->ksMax				= 550000;

				link->interactionGeometry 		= geometry;
				link->interactionPhysics 		= physics;
				rootBody->interactions->insert(link);
			}
		}
	}
	
	message="total number of permament links created: " 
		+ lexical_cast<string>(rootBody->interactions->size()) 
		+ "\nWARNING: link bonds are nearly working, but the formulas are waiting for total rewrite!"
		+"\nWARNING: interactions will not generate any force since we use SpheresContactGeometry instead of SDECLinkGeometry now.";
	return true;
}


void SDECLinkedSpheres::createSphere(shared_ptr<Body>& body, int i, int j, int k)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),55));
	shared_ptr<BodyMacroParameters> physics(new BodyMacroParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Sphere> gSphere(new Sphere);
	shared_ptr<InteractingSphere> iSphere(new InteractingSphere);
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);
	
		
	Vector3r position 		= Vector3r(i,j,k)*spacing
					  - Vector3r(nbSpheres[0]/2*spacing,nbSpheres[1]/2*spacing-90,nbSpheres[2]/2*spacing) 
					  + Vector3r(Mathr::SymmetricRandom()*disorder,Mathr::SymmetricRandom()*disorder,Mathr::SymmetricRandom()*disorder);

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
	gSphere->diffuseColor		= Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom());
	gSphere->wire			= false;
	gSphere->shadowCaster		= true;
	
	iSphere->radius			= radius;
	iSphere->diffuseColor		= Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom());

	body->shape	= iSphere;
	body->geometricalModel		= gSphere;
	body->bound		= aabb;
	body->physicalParameters	= physics;
}


void SDECLinkedSpheres::createBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),55));
	shared_ptr<BodyMacroParameters> physics(new BodyMacroParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Box> gBox(new Box);
	shared_ptr<InteractingBox> iBox(new InteractingBox);
	
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
//	physics->mass			= 0;
//	physics->inertia		= Vector3r(0,0,0);
	physics->se3			= Se3r(position,q);
	physics->young			= sphereYoungModulus;
	physics->poisson		= spherePoissonRatio;
	physics->frictionAngle		= sphereFrictionDeg * Mathr::PI/180.0;

	aabb->diffuseColor		= Vector3r(1,0,0);

	gBox->extents			= extents;
	gBox->diffuseColor		= Vector3r(1,1,1);
	gBox->wire			= false;
	gBox->shadowCaster		= true;
	
	iBox->extents			= extents;
	iBox->diffuseColor		= Vector3r(1,1,1);

	body->bound		= aabb;
	body->shape	= iBox;
	body->geometricalModel		= gBox;
	body->physicalParameters	= physics;
}


void SDECLinkedSpheres::createActors(shared_ptr<Scene>& rootBody)
{
	shared_ptr<InteractionGeometryDispatcher> interactionGeometryDispatcher(new InteractionGeometryDispatcher);
	interactionGeometryDispatcher->add("InteractingSphere2InteractingSphere4SpheresContactGeometry");
	interactionGeometryDispatcher->add("InteractingBox2InteractingSphere4SpheresContactGeometry");

	shared_ptr<InteractionPhysicsDispatcher> interactionPhysicsDispatcher(new InteractionPhysicsDispatcher);
	interactionPhysicsDispatcher->add("MacroMicroElasticRelationships");
		
	shared_ptr<BoundDispatcher> boundDispatcher	= shared_ptr<BoundDispatcher>(new BoundDispatcher);
	boundDispatcher->add("InteractingSphere2AABB");
	boundDispatcher->add("InteractingBox2AABB");
	boundDispatcher->add("MetaInteractingGeometry2AABB");
		
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
 	
	shared_ptr<ElasticCriterionTimeStepper> sdecTimeStepper(new ElasticCriterionTimeStepper);
	sdecTimeStepper->sdecGroupMask = 55;
	sdecTimeStepper->timeStepUpdateInterval = timeStepUpdateInterval;

	
	shared_ptr<ElasticContactLaw> constitutiveLaw(new ElasticContactLaw);
	constitutiveLaw->sdecGroupMask = 55;
	constitutiveLaw->momentRotationLaw = momentRotationLaw;
// FIXME FIXME FIXME ....	
	shared_ptr<ElasticCohesiveLaw> constitutiveLaw2(new ElasticCohesiveLaw);
	constitutiveLaw2->sdecGroupMask = 55;
	constitutiveLaw2->momentRotationLaw = momentRotationLaw;
	
	rootBody->engines.clear();
	rootBody->engines.push_back(sdecTimeStepper);
	rootBody->engines.push_back(shared_ptr<Engine>(new PhysicalActionContainerReseter));
	rootBody->engines.push_back(boundDispatcher);
	rootBody->engines.push_back(shared_ptr<Engine>(new InsertionSortCollider));
	rootBody->engines.push_back(interactionGeometryDispatcher);
	rootBody->engines.push_back(interactionPhysicsDispatcher);
	rootBody->engines.push_back(constitutiveLaw);
	rootBody->engines.push_back(constitutiveLaw2);
	rootBody->engines.push_back(gravityCondition);
	rootBody->engines.push_back(actionDampingDispatcher);
	rootBody->engines.push_back(applyActionDispatcher);
	rootBody->engines.push_back(positionIntegrator);
	rootBody->engines.push_back(orientationIntegrator);

	rootBody->initializers.clear();
	rootBody->initializers.push_back(boundDispatcher);
}
	

void SDECLinkedSpheres::positionRootBody(shared_ptr<Scene>& rootBody)
{
	rootBody->isDynamic		= false;
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);
	
	shared_ptr<ParticleParameters> physics(new ParticleParameters); // FIXME : fix indexable class PhysicalParameters
	physics->se3			= Se3r(Vector3r(0,0,0),q);
	physics->mass			= 0;
	physics->velocity		= Vector3r::ZERO;
	physics->acceleration		= Vector3r::ZERO;
	
	shared_ptr<MetaInteractingGeometry> set(new MetaInteractingGeometry());
	set->diffuseColor		= Vector3r(0,0,1);

	shared_ptr<AABB> aabb(new AABB);
	aabb->diffuseColor		= Vector3r(0,0,1);
	
	rootBody->shape	= YADE_PTR_CAST<Shape>(set);	
	rootBody->bound	= YADE_PTR_CAST<Bound>(aabb);
	rootBody->physicalParameters 	= physics;
	
}

YADE_PLUGIN((SDECLinkedSpheres));

YADE_REQUIRE_FEATURE(PHYSPAR);

