#include "HangingCloth.hpp"


#include "Mesh2D.hpp"
#include "Sphere.hpp"
#include "RigidBody.hpp"
#include "AABB.hpp"
#include "NonConnexBody.hpp"
#include "SimpleSpringDynamicEngine.hpp"
#include "BallisticDynamicEngine.hpp"
#include "SAPCollider.hpp"
#include "SimpleNarrowCollider.hpp"
#include "MassSpringBody.hpp"
#include "ExplicitMassSpringDynamicEngine.hpp"
#include "MassSpringBody2RigidBodyDynamicEngine.hpp"
#include <fstream>
#include "IOManager.hpp"

HangingCloth::HangingCloth () : FileGenerator()
{

}

HangingCloth::~HangingCloth ()
{

}

void HangingCloth::postProcessAttributes(bool)
{
}

void HangingCloth::registerAttributes()
{
}

string HangingCloth::generate()
{
	//FIXME : not working
	
	rootBody = shared_ptr<NonConnexBody>(new NonConnexBody);
	int width = 20;
	int height = 20;
	float mass = 10;
	const int cellSize = 20;
	Quaternionr q;
	int nbSpheres = 10;
	q.fromAxisAngle(Vector3r(0,0,1),0);

	shared_ptr<NarrowCollider> nc	= shared_ptr<NarrowCollider>(new SimpleNarrowCollider);
	nc->addCollisionFunctor("Sphere","Sphere","Sphere2Sphere4ClosestFeatures");
	nc->addCollisionFunctor("Sphere","Mesh2D","Sphere2Mesh2D4ClosestFeatures");


	rootBody->actors.resize(3);
	rootBody->actors[0] 		= shared_ptr<Actor>(new SAPCollider);
	rootBody->actors[1] 		= nc;
	rootBody->actors[2] 		= shared_ptr<Actor>(new MassSpringBody2RigidBodyDynamicEngine);




	rootBody->isDynamic      = false;
	rootBody->velocity       = Vector3r(0,0,0);
	rootBody->angularVelocity= Vector3r(0,0,0);
	rootBody->se3		 = Se3r(Vector3r(0,0,0),q);

	//for(int i=0;i<7;i++)
	//	rootBody->kinematic->subscribedBodies.push_back(i);

	shared_ptr<MassSpringBody> cloth(new MassSpringBody);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Mesh2D> mesh2d(new Mesh2D);
	cloth->actors.push_back(shared_ptr<Actor>(new ExplicitMassSpringDynamicEngine));
	cloth->isDynamic	= true;
	cloth->angularVelocity	= Vector3r(0,0,0);
	cloth->velocity		= Vector3r(0,0,0);
	cloth->mass		= mass;
	cloth->se3		= Se3r(Vector3r(0,0,0),q);
	cloth->stiffness	= 500;
	cloth->damping		= 0.1;
	for(int i=0;i<width*height;i++)
		cloth->properties.push_back(NodeProperties((float)(width*height)/mass));

	cloth->properties[offset(0,0)].invMass = 0;
	cloth->properties[offset(width-1,0)].invMass = 0;
	//cloth->properties[offset(0,height-1)].invMass = 0;
	//cloth->properties[offset(width-1,height-1)].invMass = 0;

	aabb->color		= Vector3r(1,0,0);
	aabb->center		= Vector3r(0,0,0);
	aabb->halfSize		= Vector3r((cellSize*(width-1))*0.5,0,(cellSize*(height-1))*0.5);
	cloth->bv		= dynamic_pointer_cast<BoundingVolume>(aabb);

	mesh2d->width = width;
	mesh2d->height = height;

	for(int i=0;i<width;i++)
		for(int j=0;j<height;j++)
			mesh2d->vertices.push_back(Vector3r(i*cellSize-(cellSize*(width-1))*0.5,0,j*cellSize-(cellSize*(height-1))*0.5));

	for(int i=0;i<width-1;i++)
		for(int j=0;j<height-1;j++)
		{
			mesh2d->edges.push_back(Edge(offset(i,j),offset(i+1,j)));
			mesh2d->edges.push_back(Edge(offset(i,j),offset(i,j+1)));
			mesh2d->edges.push_back(Edge(offset(i,j+1),offset(i+1,j)));

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
		mesh2d->edges.push_back(Edge(offset(i,height-1),offset(i+1,height-1)));

	for(int j=0;j<height-1;j++)
		mesh2d->edges.push_back(Edge(offset(width-1,j),offset(width-1,j+1)));

	for(unsigned int i=0;i<mesh2d->edges.size();i++)
	{
		Vector3r v1 = mesh2d->vertices[mesh2d->edges[i].first];
		Vector3r v2 = mesh2d->vertices[mesh2d->edges[i].second];
		cloth->initialLengths.push_back((v1-v2).length());
	}

	mesh2d->diffuseColor	= Vector3f(0,0,1);
	mesh2d->wire		= false;
	mesh2d->visible		= true;
	cloth->cm		= dynamic_pointer_cast<CollisionGeometry>(mesh2d);
	cloth->gm		= dynamic_pointer_cast<CollisionGeometry>(mesh2d);

	shared_ptr<Body> b;
	b = dynamic_pointer_cast<Body>(cloth);
	rootBody->bodies->insert(b);


	for(int i=0;i<1/*nbSpheres*/;i++)
	{
		shared_ptr<RigidBody> s(new RigidBody);
		shared_ptr<AABB> aabb(new AABB);
		shared_ptr<Sphere> csphere(new Sphere);
		shared_ptr<Sphere> gsphere(new Sphere);


		Vector3r translation(0,-60,0);
		float radius = 50;

		//Vector3r translation(100*Mathr::symmetricRandom(),10+100*Mathr::unitRandom(),100*Mathr::symmetricRandom());
		//float radius = 0.5*(20+10*Mathr::unitRandom());
		//shared_ptr<BallisticDynamicEngine> ballistic(new BallisticDynamicEngine);
		//ballistic->damping 	= 0.999;
		//s->dynamic		= dynamic_pointer_cast<DynamicEngine>(ballistic);

		s->isDynamic		= true;
		s->angularVelocity	= Vector3r(0,0,0);
		s->velocity		= Vector3r(0,0,0);
		s->mass			= 4.0/3.0*Mathr::PI*radius*radius;
		s->inertia		= Vector3r(2.0/5.0*s->mass*radius*radius,2.0/5.0*s->mass*radius*radius,2.0/5.0*s->mass*radius*radius);
		s->se3			= Se3r(translation,q);

		aabb->color		= Vector3r(0,1,0);
		aabb->center		= translation;
		aabb->halfSize		= Vector3r(radius,radius,radius);
		s->bv			= dynamic_pointer_cast<BoundingVolume>(aabb);

		csphere->radius		= radius*1.2;
		csphere->diffuseColor	= Vector3f(Mathr::unitRandom(),Mathr::unitRandom(),Mathr::unitRandom());
		csphere->wire		= false;
		csphere->visible	= true;

		gsphere->radius		= radius;
		gsphere->diffuseColor	= Vector3f(Mathr::unitRandom(),Mathr::unitRandom(),Mathr::unitRandom());
		gsphere->wire		= false;
		gsphere->visible	= true;

		s->cm			= dynamic_pointer_cast<CollisionGeometry>(csphere);
		s->gm			= dynamic_pointer_cast<GeometricalModel>(gsphere);

		b = dynamic_pointer_cast<Body>(s);
		rootBody->bodies->insert(b);
	}

	return "";
}
