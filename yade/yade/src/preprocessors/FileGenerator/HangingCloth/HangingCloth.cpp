#include "HangingCloth.hpp"

#include "SimpleBody.hpp"
#include "Mesh2D.hpp"
#include "Sphere.hpp"
#include "RigidBodyParameters.hpp"
#include "AABB.hpp"
#include "ComplexBody.hpp"
#include "SimpleSpringDynamicEngine.hpp"
#include "PersistentSAPCollider.hpp"
#include "ComplexBody.hpp"
#include "ExplicitMassSpringDynamicEngine.hpp"
#include "MassSpringBody2RigidBodyDynamicEngine.hpp"

#include "IOManager.hpp"
#include "InteractionGeometryDispatcher.hpp"
#include "InteractionPhysicsDispatcher.hpp"

#include "ActionApplyDispatcher.hpp"
#include "ActionDampingDispatcher.hpp"
#include "ActionForceDamping.hpp"
#include "ActionMomentumDamping.hpp"

#include "BoundingVolumeDispatcher.hpp"
#include "GeometricalModelDispatcher.hpp"

#include "InteractionDescriptionSet2AABBFunctor.hpp"
#include "InteractionDescriptionSet.hpp"
#include "ParticleParameters.hpp"
#include "ParticleSetParameters.hpp"
#include "SpringGeometry.hpp"
#include "SpringPhysics.hpp"
#include "TimeIntegratorDispatcher.hpp"
#include "InteractionSphere.hpp"

HangingCloth::HangingCloth () : FileGenerator()
{

	width = 20;
	height = 20;
	stiffness = 500;
	springDamping   = 0.8;
	particleDamping = 0.997;
	mass = 10;
	cellSize = 20;
	fixPoint1 = true;
	fixPoint2 = true;
	fixPoint3 = true;
	fixPoint4 = true;
	dampingForce = 0.3;

}

HangingCloth::~HangingCloth ()
{

}

void HangingCloth::postProcessAttributes(bool)
{
}

void HangingCloth::registerAttributes()
{
	REGISTER_ATTRIBUTE(width);
	REGISTER_ATTRIBUTE(height);
	REGISTER_ATTRIBUTE(stiffness);
	REGISTER_ATTRIBUTE(dampingForce);
	REGISTER_ATTRIBUTE(springDamping);
	//REGISTER_ATTRIBUTE(particleDamping); // FIXME - ignored - delete it, or start using it
	REGISTER_ATTRIBUTE(mass);
	REGISTER_ATTRIBUTE(cellSize);
	REGISTER_ATTRIBUTE(fixPoint1);
	REGISTER_ATTRIBUTE(fixPoint2);
	REGISTER_ATTRIBUTE(fixPoint3);
	REGISTER_ATTRIBUTE(fixPoint4);
}

string HangingCloth::generate()
{

	rootBody = shared_ptr<ComplexBody>(new ComplexBody);

	Quaternionr q;
	q.fromAxisAngle( Vector3r(0,0,1),0);

	shared_ptr<InteractionGeometryDispatcher> interactionGeometryDispatcher(new InteractionGeometryDispatcher);
	interactionGeometryDispatcher->add("InteractionSphere","InteractionSphere","Sphere2Sphere4SDECContactModel");
	interactionGeometryDispatcher->add("InteractionSphere","InteractionBox","Box2Sphere4SDECContactModel");

	shared_ptr<InteractionPhysicsDispatcher> interactionPhysicsDispatcher(new InteractionPhysicsDispatcher);
	interactionPhysicsDispatcher->add("SDECParameters","SDECParameters","SDECLinearContactModel");

	shared_ptr<BoundingVolumeDispatcher> boundingVolumeDispatcher	= shared_ptr<BoundingVolumeDispatcher>(new BoundingVolumeDispatcher);
	boundingVolumeDispatcher->add("InteractionSphere","AABB","Sphere2AABBFunctor");
	boundingVolumeDispatcher->add("InteractionBox","AABB","Box2AABBFunctor");
	boundingVolumeDispatcher->add("InteractionDescriptionSet","AABB","InteractionDescriptionSet2AABBFunctor");

	shared_ptr<GeometricalModelDispatcher> geometricalModelDispatcher	= shared_ptr<GeometricalModelDispatcher>(new GeometricalModelDispatcher);
	geometricalModelDispatcher->add("ParticleSetParameters","Mesh2D","ParticleSet2Mesh2D");

	shared_ptr<ActionForceDamping> actionForceDamping(new ActionForceDamping);
	actionForceDamping->damping = dampingForce;
	shared_ptr<ActionDampingDispatcher> actionDampingDispatcher(new ActionDampingDispatcher);
	actionDampingDispatcher->add("ActionForce","ParticleParameters","ActionForceDamping",actionForceDamping);
	
	shared_ptr<ActionApplyDispatcher> applyActionDispatcher(new ActionApplyDispatcher);
	applyActionDispatcher->add("ActionForce","ParticleParameters","ActionForce2Particle");

	shared_ptr<TimeIntegratorDispatcher> timeIntegratorDispatcher(new TimeIntegratorDispatcher);
	timeIntegratorDispatcher->add("ParticleParameters","LeapFrogIntegrator");

	rootBody->actors.push_back(boundingVolumeDispatcher);
	rootBody->actors.push_back(geometricalModelDispatcher);
	rootBody->actors.push_back(shared_ptr<Actor>(new PersistentSAPCollider));
	//rootBody->actors.push_back(interactionGeometryDispatcher);
	//rootBody->actors.push_back(interactionPhysicsDispatcher);
	rootBody->actors.push_back(shared_ptr<Actor>(new ExplicitMassSpringDynamicEngine));
	rootBody->actors.push_back(actionDampingDispatcher);
	rootBody->actors.push_back(applyActionDispatcher);
	rootBody->actors.push_back(timeIntegratorDispatcher);


	//FIXME : use a default one
	shared_ptr<ParticleSetParameters> physics2(new ParticleSetParameters);
	physics2->se3		= Se3r(Vector3r(0,0,0),q);

	rootBody->isDynamic	= false;

	shared_ptr<InteractionDescriptionSet> set(new InteractionDescriptionSet());
	set->diffuseColor	= Vector3f(0,0,1);

	shared_ptr<AABB> aabb(new AABB);
	aabb->diffuseColor	= Vector3r(0,0,1);

	shared_ptr<Mesh2D> mesh2d(new Mesh2D);
	mesh2d->diffuseColor	= Vector3f(0,0,1);
	mesh2d->wire		= false;
	mesh2d->visible		= true;
	mesh2d->shadowCaster	= false;

	rootBody->geometricalModel			= mesh2d;
	rootBody->interactionGeometry			= set;
	rootBody->boundingVolume			= aabb;
	rootBody->physicalParameters	= physics2;

	rootBody->permanentInteractions->clear();

	for(int i=0;i<width;i++)
		for(int j=0;j<height;j++)
		{
			shared_ptr<Body> node(new SimpleBody);

			node->isDynamic		= true;

			shared_ptr<ParticleParameters> particle(new ParticleParameters);
			particle->velocity		= Vector3r(0,0,0);
			particle->mass			= mass/(Real)(width*height);
			particle->se3			= Se3r(Vector3r(i*cellSize-(cellSize*(width-1))*0.5,0,j*cellSize-(cellSize*(height-1))*0.5),q);

			shared_ptr<AABB> aabb(new AABB);
			aabb->diffuseColor	= Vector3r(0,1,0);

			shared_ptr<InteractionSphere> iSphere(new InteractionSphere);
			iSphere->diffuseColor	= Vector3f(0,0,1);
			iSphere->radius		= cellSize/2.0;

			node->boundingVolume		= aabb;
			//node->geometricalModel		= ??;
			node->interactionGeometry		= iSphere;
			node->physicalParameters= particle;

			rootBody->bodies->insert(node);
			mesh2d->vertices.push_back(particle->se3.translation);
		}

	for(int i=0;i<width-1;i++)
		for(int j=0;j<height-1;j++)
		{
			mesh2d->edges.push_back(Edge(offset(i,j),offset(i+1,j)));
			mesh2d->edges.push_back(Edge(offset(i,j),offset(i,j+1)));
			mesh2d->edges.push_back(Edge(offset(i,j+1),offset(i+1,j)));

			rootBody->permanentInteractions->insert(createSpring(rootBody,offset(i,j),offset(i+1,j)));
			rootBody->permanentInteractions->insert(createSpring(rootBody,offset(i,j),offset(i,j+1)));
			rootBody->permanentInteractions->insert(createSpring(rootBody,offset(i,j+1),offset(i+1,j)));

			vector<int> face1,face2;
			face1.push_back(offset(i,j));
			face1.push_back(offset(i+1,j));
			face1.push_back(offset(i,j+1));

			face2.push_back(offset(i+1,j));
			face2.push_back(offset(i+1,j+1));
			face2.push_back(offset(i,j+1));

			mesh2d->faces.push_back(face1);
			mesh2d->faces.push_back(face2);
		}

	for(int i=0;i<width-1;i++)
	{
		mesh2d->edges.push_back(Edge(offset(i,height-1),offset(i+1,height-1)));
		rootBody->permanentInteractions->insert(createSpring(rootBody,offset(i,height-1),offset(i+1,height-1)));

	}

	for(int j=0;j<height-1;j++)
	{
		mesh2d->edges.push_back(Edge(offset(width-1,j),offset(width-1,j+1)));
		rootBody->permanentInteractions->insert(createSpring(rootBody,offset(width-1,j),offset(width-1,j+1)));
	}

	if (fixPoint1)
	{
		Body * body = static_cast<Body*>((*(rootBody->bodies))[offset(0,0)].get());
		ParticleParameters * p = static_cast<ParticleParameters*>(body->physicalParameters.get());
		p->invMass = 0;
		body->interactionGeometry->diffuseColor = Vector3f(1.0,0.0,0.0);
		body->isDynamic = false;
	}

	if (fixPoint2)
	{
		Body * body = static_cast<Body*>((*(rootBody->bodies))[offset(width-1,0)].get());
		ParticleParameters * p = static_cast<ParticleParameters*>(body->physicalParameters.get());
		p->invMass = 0;
		body->interactionGeometry->diffuseColor = Vector3f(1.0,0.0,0.0);
		body->isDynamic = false;
	}

	if (fixPoint3)
	{
		Body * body = static_cast<Body*>((*(rootBody->bodies))[offset(0,height-1)].get());
		ParticleParameters * p = static_cast<ParticleParameters*>(body->physicalParameters.get());
		p->invMass = 0;
		body->interactionGeometry->diffuseColor = Vector3f(1.0,0.0,0.0);
		body->isDynamic = false;
	}

	if (fixPoint4)
	{
		Body * body = static_cast<Body*>((*(rootBody->bodies))[offset(width-1,height-1)].get());
		ParticleParameters * p = static_cast<ParticleParameters*>(body->physicalParameters.get());
		p->invMass = 0;
		body->interactionGeometry->diffuseColor = Vector3f(1.0,0.0,0.0);
		body->isDynamic = false;
	}

	return "";
}


shared_ptr<Interaction>& HangingCloth::createSpring(const shared_ptr<ComplexBody>& rootBody,int i,int j)
{
	SimpleBody * b1 = static_cast<SimpleBody*>((*(rootBody->bodies))[i].get());
	SimpleBody * b2 = static_cast<SimpleBody*>((*(rootBody->bodies))[j].get());

	spring = shared_ptr<Interaction>(new Interaction( b1->getId() , b2->getId() ));
	shared_ptr<SpringGeometry>	geometry(new SpringGeometry);
	shared_ptr<SpringPhysics>	physics(new SpringPhysics);

	geometry->p1			= b1->physicalParameters->se3.translation;
	geometry->p2			= b2->physicalParameters->se3.translation;

	physics->initialLength		= (geometry->p1-geometry->p2).length();
	physics->stiffness		= stiffness;
	physics->damping		= springDamping;

	spring->interactionGeometry = geometry;
	spring->interactionPhysics = physics;
	spring->isReal = true;
	spring->isNew = false;

	return spring;
}
