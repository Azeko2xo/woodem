/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@mail.berlios.de                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include <boost/lexical_cast.hpp>

#include "TetrahedronsTest.hpp"

#include <yade/yade-package-common/ElasticBodyParameters.hpp>
#include "ElasticCriterionTimeStepper.hpp"
#include "InteractingMyTetrahedron.hpp"
#include "MyTetrahedronLaw.hpp"

#include <yade/yade-package-common/Box.hpp>
#include <yade/yade-package-common/AABB.hpp>
#include <yade/yade-core/MetaBody.hpp>
#include <yade/yade-package-common/SAPCollider.hpp>
#include <yade/yade-package-common/PersistentSAPCollider.hpp>
#include <yade/yade-lib-serialization/IOFormatManager.hpp>
#include <yade/yade-core/Interaction.hpp>

#include <yade/yade-package-common/BoundingVolumeMetaEngine.hpp>
#include <yade/yade-package-common/MetaInteractingGeometry2AABB.hpp>
#include <yade/yade-package-common/MetaInteractingGeometry.hpp>

#include <yade/yade-package-common/PhysicalActionContainerReseter.hpp>
#include <yade/yade-package-common/PhysicalActionContainerInitializer.hpp>

#include <yade/yade-package-common/PhysicalActionDamper.hpp>
#include <yade/yade-package-common/PhysicalActionApplier.hpp>

#include <yade/yade-package-common/CundallNonViscousForceDamping.hpp>
#include <yade/yade-package-common/CundallNonViscousMomentumDamping.hpp>
#include <yade/yade-package-common/GravityEngine.hpp>

#include <yade/yade-package-common/InteractingGeometryMetaEngine.hpp>
#include <yade/yade-package-common/InteractionGeometryMetaEngine.hpp>
#include <yade/yade-package-common/InteractionPhysicsMetaEngine.hpp>
#include <yade/yade-core/Body.hpp>
#include <yade/yade-package-common/InteractingBox.hpp>
#include <yade/yade-package-common/InteractingSphere.hpp>
#include <yade/yade-package-common/PhysicalParametersMetaEngine.hpp>

#include <yade/yade-package-common/BodyRedirectionVector.hpp>
#include <yade/yade-package-common/InteractionVecSet.hpp>
#include <yade/yade-package-common/PhysicalActionVectorVector.hpp>


TetrahedronsTest::TetrahedronsTest () : FileGenerator()
{
	nbTetrahedrons		= Vector3r(5,6,7);
	minSize			= 5;
	maxSize			= 6;

	groundSize		= Vector3r(100,5,100);

	/*
	 *
	 * Vaclav,
	 *
	 * damping has so huge values, because this example calculation has no
	 * shering force in the contact, therefore there cannot be fricion
	 * simulated. To make a good simulation (with tetrahedrons too) you
	 * should add shearing force. It is not possible to have friction
	 * without shearing :)
	 *
	 * You can see into ElasticContactLaw (press here ctrl-] , then ctrl-O ;)
	 * how shearing force is used. You may even want to use
	 * BodyMacroParameters and sth. like ElasticContactInteraction to
	 * calculate friction, and incremental between iterations shearing
	 * force
	 *
	 * Also you can check how the simulation behaves with damping equal to zero.
	 *
	 */
	dampingForce		= 0.9;
	dampingMomentum		= 0.9;

	timeStepUpdateInterval	= 300;

	youngModulus		= 80000000.0;

	density			= 2600;
	rotationBlocked		= false;
	gravity			= Vector3r(0,-9.81,0);
	disorder		= 0.2;
}


TetrahedronsTest::~TetrahedronsTest ()
{

}


void TetrahedronsTest::postProcessAttributes(bool)
{
}


void TetrahedronsTest::registerAttributes()
{
	REGISTER_ATTRIBUTE(nbTetrahedrons);
	REGISTER_ATTRIBUTE(minSize);
	REGISTER_ATTRIBUTE(maxSize);
	REGISTER_ATTRIBUTE(youngModulus);
	REGISTER_ATTRIBUTE(gravity);
	REGISTER_ATTRIBUTE(density);
	REGISTER_ATTRIBUTE(disorder);
	REGISTER_ATTRIBUTE(groundSize);
	REGISTER_ATTRIBUTE(dampingForce);
	REGISTER_ATTRIBUTE(dampingMomentum);
	REGISTER_ATTRIBUTE(rotationBlocked);
	//REGISTER_ATTRIBUTE(timeStepUpdateInterval); // not used. But you may want to use it....
}


string TetrahedronsTest::generate()
{
	rootBody = shared_ptr<MetaBody>(new MetaBody);
	createActors(rootBody);
	positionRootBody(rootBody);

////////////////////////////////////
///////// Container
	
	rootBody->persistentInteractions	= shared_ptr<InteractionContainer>(new InteractionVecSet);
	rootBody->volatileInteractions		= shared_ptr<InteractionContainer>(new InteractionVecSet);
	rootBody->physicalActions		= shared_ptr<PhysicalActionContainer>(new PhysicalActionVectorVector);
	rootBody->bodies 			= shared_ptr<BodyContainer>(new BodyRedirectionVector);
		
////////////////////////////////////
///////// ground

	shared_ptr<Body> ground;
	createBox(ground, Vector3r(0,0,0), groundSize);
	rootBody->bodies->insert(ground);

///////// tetrahedrons
	float all = nbTetrahedrons[0]*nbTetrahedrons[1]*nbTetrahedrons[2];
	float current = 0.0;

	setMessage("generating tetrahedrons...");
	for(int i=0;i<nbTetrahedrons[0];i++)
	{
		if(shouldTerminate()) return "";

		for(int j=0;j<nbTetrahedrons[1];j++)
			for(int k=0;k<nbTetrahedrons[2];k++)
			{
				shared_ptr<Body> tet;
				createTetrahedron(tet,i,j,k);
				rootBody->bodies->insert(tet);
				
				setProgress(current++/all);
			}
	}
	
	return "foo bar "+boost::lexical_cast<std::string>(42);
}


void TetrahedronsTest::createTetrahedron(shared_ptr<Body>& body, int i, int j, int k)
{
	body = shared_ptr<Body>(new Body(0,1));
	shared_ptr<ElasticBodyParameters> physics(new ElasticBodyParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Tetrahedron> tet(new Tetrahedron);
	
	Quaternionr q;// = Quaternionr::IDENTITY;
	// random orientation
	q.fromAxisAngle( Vector3r(Mathr::symmetricRandom(),Mathr::symmetricRandom(),Mathr::symmetricRandom()),Mathr::symmetricRandom());
	q.normalize();

	// semi-random position in the space
	Vector3r position		= Vector3r(i,j,k)*(2*maxSize*1.1) // this formula is crazy !!
					  - Vector3r( nbTetrahedrons[0]/2*(2*maxSize*1.1) , -7-maxSize*2 , nbTetrahedrons[2]/2*(2*maxSize*1.1) )
					  + Vector3r( 	 Mathr::symmetricRandom()
					  		,Mathr::symmetricRandom()
							,Mathr::symmetricRandom())*maxSize*disorder;
	
	Real radius 			= (Mathr::intervalRandom(minSize,maxSize));
	
	body->isDynamic			= true;

	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);

	makeTet(tet,radius);
	tet->diffuseColor		= Vector3f(Mathf::unitRandom(),Mathf::unitRandom(),Mathf::unitRandom());
	tet->wire			= false;
	tet->visible			= true;
	tet->shadowCaster		= false;

	// Vaclav,
	//
	// Here mass and inertia are totally wrong, because the formulas are for sphere, not for tetrahedron.
	// I should put here correct formulas to get scientific results. But this is a quick example.
	//
	// So it's your job to write good formulas here.
	//
	// They are used at the end of each interation calculation, and it's important that they are correct, because:
	//
	// class NewtonsForceLaw               uses mass                to calculate body acceleration        from given force
	// class NewtonsMomentumLaw            uses inertia             to calculate body angularAcceleration from given momentum
	//
	// class LeapFrogPositionIntegrator    uses acceleration        to calculate body translation
	// class LeapFrogOrientationIntegrator uses angularAcceleration to calculate body rotation
	//
	// (those four classes cannot be modifed, they do their job just fine :)
	// (also this exact information written above, is explicitly defined in createActors, this file, line 314)
	//
	// So currently these numbers are wrong, so tetrahedrons will rotate unrealistically (as you can see yourself :)
	//
	physics->mass			= 4.0/3.0*Mathr::PI*radius*radius*radius*density;
	//
	// in fact I'm curious to see how this example behaves with correct inertia and mass.
	// Before you do anything else you can try to write a good formula here. I belive that
	// tetrahedrons will behave much more correctly.
	physics->inertia		= Vector3r(2.0/5.0*physics->mass*radius*radius,2.0/5.0*physics->mass*radius*radius,2.0/5.0*physics->mass*radius*radius);
	////////////////////////////////////////////////////////////////////////////////////

	physics->se3			= Se3r(position,q);
	physics->young			= youngModulus;

	aabb->diffuseColor		= Vector3r(0,1,0);

	// interactingGeometry is filled with data by Tetrahedron2InteractingMyTetrahedron
	shared_ptr<InteractingGeometry> imt(new InteractingMyTetrahedron);
	imt->diffuseColor               = Vector3f(0.5,0.5,1.0);
	body->interactingGeometry	= imt;

	body->geometricalModel		= tet;
	body->boundingVolume		= aabb;

	// Vaclav,
	//
	// Here I use ElasticBodyParameters for physics, because this very simple example
	// of four spheres in a tetrahedron needs only young modulus. (stiffness)
	// and nothing more.
	//
	// if you need some other physical data about tetrahedron (not just stiffness)
	// then you will need to make a new class.
	//
	body->physicalParameters	= physics;
}


void TetrahedronsTest::createBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents)
{
	// this is just the bottom surface on which all is lying
	// we could even make a box (or even RotatingBox) by making several boxes, as
	// in the RotatingBox example FileGenerator

	body = shared_ptr<Body>(new Body(0,1));
	shared_ptr<ElasticBodyParameters> physics(new ElasticBodyParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Box> gBox(new Box);
	shared_ptr<InteractingBox> iBox(new InteractingBox);
	
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
	physics->se3			= Se3r(position,q);
	physics->young			= youngModulus;

	aabb->diffuseColor		= Vector3r(1,0,0);

	gBox->extents			= extents;
	gBox->diffuseColor		= Vector3f(1,1,1);
	gBox->wire			= false;
	gBox->visible			= true;
	gBox->shadowCaster		= true;
	
	iBox->extents			= extents;
	iBox->diffuseColor		= Vector3f(1,1,1);

	body->boundingVolume		= aabb;
	body->interactingGeometry	= iBox;
	body->geometricalModel		= gBox;
	body->physicalParameters	= physics;
}


void TetrahedronsTest::createActors(shared_ptr<MetaBody>& rootBody)
{
	shared_ptr<PhysicalActionContainerInitializer> physicalActionInitializer(new PhysicalActionContainerInitializer);

	// all the strings here are just class names
	// those class names in each class are registered with REGISTER_CLASS_NAME(SomeClass);
	//
	physicalActionInitializer->physicalActionNames.push_back("Force");
	physicalActionInitializer->physicalActionNames.push_back("Momentum");
	
	shared_ptr<InteractionPhysicsMetaEngine> interactionPhysicsDispatcher(new InteractionPhysicsMetaEngine);
	// so for this simple example I use  ElasticBodyParameters           : to store young modulus,
	// and                               ElasticBodySimpleRelationship   : to calculate the stiffnes of current interaction
	//                                                                     using the young modulus of two bodies, it creates
	//                                                                     a class SimpleElasticInteraction
	//
	// Vaclav, it is very likely that you will need some other physical representation of interaction than SimpleElasticInteraction
	//
	interactionPhysicsDispatcher->add("ElasticBodyParameters","ElasticBodyParameters","ElasticBodySimpleRelationship");
		
	shared_ptr<InteractionGeometryMetaEngine> interactionGeometryDispatcher(new InteractionGeometryMetaEngine);
	interactionGeometryDispatcher->add("InteractingMyTetrahedron","InteractingMyTetrahedron","InteractingMyTetrahedron2InteractingMyTetrahedron4InteractionOfMyTetrahedron");
	interactionGeometryDispatcher->add("InteractingMyTetrahedron","InteractingBox","InteractingMyTetrahedron2InteractingBox4InteractionOfMyTetrahedron");

	shared_ptr<InteractingGeometryMetaEngine> interactingGeometryDispatcher	= shared_ptr<InteractingGeometryMetaEngine>(new InteractingGeometryMetaEngine);
	interactingGeometryDispatcher->add("Tetrahedron","InteractingMyTetrahedron","Tetrahedron2InteractingMyTetrahedron");
	
	shared_ptr<BoundingVolumeMetaEngine> boundingVolumeDispatcher	= shared_ptr<BoundingVolumeMetaEngine>(new BoundingVolumeMetaEngine);
	boundingVolumeDispatcher->add("InteractingSphere","AABB","InteractingSphere2AABB");
	boundingVolumeDispatcher->add("InteractingBox","AABB","InteractingBox2AABB");
	boundingVolumeDispatcher->add("InteractingMyTetrahedron","AABB","InteractingMyTetrahedron2AABB");
	boundingVolumeDispatcher->add("MetaInteractingGeometry","AABB","MetaInteractingGeometry2AABB");
	
	shared_ptr<GravityEngine> gravityCondition(new GravityEngine);
	gravityCondition->gravity = gravity;
	
	shared_ptr<CundallNonViscousForceDamping> actionForceDamping(new CundallNonViscousForceDamping);
	actionForceDamping->damping = dampingForce;

	shared_ptr<CundallNonViscousMomentumDamping> actionMomentumDamping(new CundallNonViscousMomentumDamping);
	actionMomentumDamping->damping = dampingMomentum;

	shared_ptr<PhysicalActionDamper> actionDampingDispatcher(new PhysicalActionDamper);
	actionDampingDispatcher->add("Force","RigidBodyParameters","CundallNonViscousForceDamping",actionForceDamping);
	actionDampingDispatcher->add("Momentum","RigidBodyParameters","CundallNonViscousMomentumDamping",actionMomentumDamping);
	
	shared_ptr<PhysicalActionApplier> applyActionDispatcher(new PhysicalActionApplier);
	applyActionDispatcher->add("Force","RigidBodyParameters","NewtonsForceLaw");
	applyActionDispatcher->add("Momentum","RigidBodyParameters","NewtonsMomentumLaw");

	// 
	// they must be separate, because for each body both of those integrators must perform the necessary work.
	// if they were in a single MetaEngine then LeapFrogOrientationIntegrator would "win" the competition
	// because it is closer polymorphically to RigidBodyParameters (and
	// ElasticBodyParameters derives directly from RigidBodyParameters). 
	//
	// And the bodies would ONLY rotate...
	shared_ptr<PhysicalParametersMetaEngine> positionIntegrator(new PhysicalParametersMetaEngine);
	positionIntegrator->add("ParticleParameters","LeapFrogPositionIntegrator");

	// so another separate MetaEngine
	shared_ptr<PhysicalParametersMetaEngine> orientationIntegrator(new PhysicalParametersMetaEngine);
	orientationIntegrator->add("RigidBodyParameters","LeapFrogOrientationIntegrator");
	//////////////////
 	
/*
 * updating timestep is not used in this simplified example. But you may decide to use it later.
 *
	shared_ptr<ElasticCriterionTimeStepper> sdecTimeStepper(new ElasticCriterionTimeStepper);
	sdecTimeStepper->sdecGroupMask = 1;
	sdecTimeStepper->timeStepUpdateInterval = timeStepUpdateInterval;
*/

	rootBody->engines.clear();
	rootBody->engines.push_back(shared_ptr<Engine>(new PhysicalActionContainerReseter));
	//rootBody->engines.push_back(sdecTimeStepper);
	rootBody->engines.push_back(boundingVolumeDispatcher);	
	rootBody->engines.push_back(shared_ptr<Engine>(new PersistentSAPCollider));
	rootBody->engines.push_back(interactionGeometryDispatcher);
	rootBody->engines.push_back(interactionPhysicsDispatcher);
	rootBody->engines.push_back(shared_ptr<Engine>(new MyTetrahedronLaw));
	rootBody->engines.push_back(gravityCondition);
	rootBody->engines.push_back(actionDampingDispatcher);
	rootBody->engines.push_back(applyActionDispatcher);
	rootBody->engines.push_back(positionIntegrator);
	if(!rotationBlocked)
		rootBody->engines.push_back(orientationIntegrator);
	
	rootBody->initializers.clear();
	rootBody->initializers.push_back(physicalActionInitializer);
	rootBody->initializers.push_back(interactingGeometryDispatcher);
	rootBody->initializers.push_back(boundingVolumeDispatcher);
}


void TetrahedronsTest::positionRootBody(shared_ptr<MetaBody>& rootBody) 
{
	rootBody->isDynamic		= false;
	
	Quaternionr q;
	q.fromAxisAngle( Vector3r(0,0,1),0);

	shared_ptr<ParticleParameters> physics(new ParticleParameters); // FIXME : fix indexable class PhysicalParameters
	physics->se3				= Se3r(Vector3r(0,0,0),q);
	physics->mass				= 0;
	physics->velocity			= Vector3r(0,0,0);
	physics->acceleration			= Vector3r::ZERO;
		
	shared_ptr<MetaInteractingGeometry> set(new MetaInteractingGeometry());
	set->diffuseColor			= Vector3f(0,0,1);
	
	shared_ptr<AABB> aabb(new AABB);
	aabb->diffuseColor			= Vector3r(0,0,1);
	
	rootBody->interactingGeometry		= dynamic_pointer_cast<InteractingGeometry>(set);	
	rootBody->boundingVolume		= dynamic_pointer_cast<BoundingVolume>(aabb);
	rootBody->physicalParameters 		= physics;
}


void TetrahedronsTest::makeTet(shared_ptr<Tetrahedron>& tet, Real size)
{
	tet->v1=size*(Mathr::unitRandom()*0.7+1.0)*Vector3r(0,0,1);
	tet->v2=size*(Mathr::unitRandom()*0.7+1.0)*Vector3r(0.73,-0.6,-0.33);
	tet->v3=size*(Mathr::unitRandom()*0.7+1.0)*Vector3r(-0.88,-0.33,-0.33);
	tet->v4=size*(Mathr::unitRandom()*0.7+1.0)*Vector3r(0.16,0.93,-0.33);
}

