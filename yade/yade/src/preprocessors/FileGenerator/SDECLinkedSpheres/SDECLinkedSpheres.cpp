#include "SDECLinkedSpheres.hpp"

#include "Rand.hpp"
#include "Box.hpp"
#include "AABB.hpp"
#include "Sphere.hpp"
#include "NonConnexBody.hpp"
#include "SAPCollider.hpp"
#include "SimpleNarrowCollider.hpp"
#include "SDECDiscreteElement.hpp"
#include "BallisticDynamicEngine.hpp"
#include <fstream>
#include "IOManager.hpp"
#include "SDECDynamicEngine.hpp"
#include "SDECDiscreteElement.hpp"
#include "SDECPermanentLink.hpp"
#include "Contact.hpp"


SDECLinkedSpheres::SDECLinkedSpheres () : Serializable()
{
	exec();
}

SDECLinkedSpheres::~SDECLinkedSpheres ()
{

}

void SDECLinkedSpheres::processAttributes()
{
}

void SDECLinkedSpheres::registerAttributes()
{
}

void SDECLinkedSpheres::exec()
{
	shared_ptr<NonConnexBody> rootBody(new NonConnexBody);
	int nbSpheres = 2;
	Quaternion q;
	q.fromAngleAxis(0, Vector3(0,0,1));

	shared_ptr<NarrowCollider> nc	= shared_ptr<NarrowCollider>(new SimpleNarrowCollider);
	nc->addCollisionFunctor("Sphere","Sphere","Sphere2Sphere4SDECContactModel");
	nc->addCollisionFunctor("Sphere","Box","Box2Sphere4SDECContactModel");

	rootBody->actors.resize(3);
	rootBody->actors[0] 		= shared_ptr<Actor>(new SAPCollider);
	rootBody->actors[1] 		= nc;
	rootBody->actors[2] 		= shared_ptr<Actor>(new SDECDynamicEngine);

	rootBody->permanentInteractions.resize(0);
//	rootBody->permanentInteractions[0] = shared_ptr<Contact>(new Contact);
//	rootBody->permanentInteractions[0]->interactionGeometry = shared_ptr<SDECPermanentLink>(new SDECPermanentLink);

	rootBody->isDynamic		= false;
	rootBody->velocity		= Vector3(0,0,0);
	rootBody->angularVelocity	= Vector3(0,0,0);
	rootBody->se3			= Se3(Vector3(0,0,0),q);

	shared_ptr<AABB> aabb;
	shared_ptr<Box> box;

	shared_ptr<SDECDiscreteElement> box1(new SDECDiscreteElement);

	aabb			= shared_ptr<AABB>(new AABB);
	box			= shared_ptr<Box>(new Box);
	box1->isDynamic		= false;
	box1->angularVelocity	= Vector3(0,0,0);
	box1->velocity		= Vector3(0,0,0);
	box1->mass		= 0;
	box1->inertia		= Vector3(0,0,0);
	box1->se3		= Se3(Vector3(0,0,0),q);
	aabb->color		= Vector3(1,0,0);
	aabb->center		= Vector3(0,0,10);
	aabb->halfSize		= Vector3(200,5,200);
	box1->bv		= dynamic_pointer_cast<BoundingVolume>(aabb);
	box->extents		= Vector3(200,5,200);
	box->diffuseColor	= Vector3(1,1,1);
	box->wire		= false;
	box->visible		= true;
	box1->cm		= dynamic_pointer_cast<CollisionGeometry>(box);
	box1->gm		= dynamic_pointer_cast<CollisionGeometry>(box);
	box1->kn		= 100000;
	box1->ks		= 10000;


	rootBody->bodies.push_back(dynamic_pointer_cast<Body>(box1));

	Vector3 translation;

	for(int i=0;i<nbSpheres;i++)
		for(int j=0;j<nbSpheres;j++)
			for(int k=0;k<nbSpheres;k++)
	{
		shared_ptr<SDECDiscreteElement> s(new SDECDiscreteElement);
		shared_ptr<AABB> aabb(new AABB);
		shared_ptr<Sphere> sphere(new Sphere);

		translation 		= Vector3(i,j,k)*10-Vector3(nbSpheres/2*10,nbSpheres/2*10-90,nbSpheres/2*10)+Vector3(Rand::symmetricRandom()*1.3,Rand::symmetricRandom(),Rand::symmetricRandom()*1.3);
		float radius 		= (Rand::intervalRandom(3,5));

		shared_ptr<BallisticDynamicEngine> ballistic(new BallisticDynamicEngine);
		ballistic->damping 	= 1.0;//0.95;
		s->actors.push_back(ballistic);

		s->isDynamic		= true;
		s->angularVelocity	= Vector3(0,0,0);
		s->velocity		= Vector3(0,0,0);
		s->mass			= 4.0/3.0*Constants::PI*radius*radius;
		s->inertia		= Vector3(2.0/5.0*s->mass*radius*radius,2.0/5.0*s->mass*radius*radius,2.0/5.0*s->mass*radius*radius);
		s->se3			= Se3(translation,q);

		aabb->color		= Vector3(0,1,0);
		aabb->center		= translation;
		aabb->halfSize		= Vector3(radius,radius,radius);
		s->bv			= dynamic_pointer_cast<BoundingVolume>(aabb);

		sphere->radius		= radius;
		sphere->diffuseColor	= Vector3(Rand::unitRandom(),Rand::unitRandom(),Rand::unitRandom());
		sphere->wire		= false;
		sphere->visible		= true;
		s->cm			= dynamic_pointer_cast<CollisionGeometry>(sphere);
		s->gm			= dynamic_pointer_cast<GeometricalModel>(sphere);
		s->kn			= 100000;
		s->ks			= 10000;

		rootBody->bodies.push_back(dynamic_pointer_cast<Body>(s));
	}

	vector<shared_ptr<Body> >::iterator ait    = (rootBody->bodies).begin();
	vector<shared_ptr<Body> >::iterator aitEnd = (rootBody->bodies).end();
	for( int idA=0 ; ait < aitEnd ; ++ait , ++idA )
	{
		vector<shared_ptr<Body> >::iterator bit    = ait;
		++bit;
		vector<shared_ptr<Body> >::iterator bitEnd = (rootBody->bodies).end();
		for( int idB=idA+1 ; bit < bitEnd ; ++bit , ++idB )
		{
			shared_ptr<SDECDiscreteElement> a = dynamic_pointer_cast<SDECDiscreteElement>(*ait);
			shared_ptr<SDECDiscreteElement> b = dynamic_pointer_cast<SDECDiscreteElement>(*bit);
			shared_ptr<Sphere>             as = dynamic_pointer_cast<Sphere>(a->cm);
			shared_ptr<Sphere>             bs = dynamic_pointer_cast<Sphere>(a->cm);

			if( a && b && as && bs &&
			( (a->se3.translation - b->se3.translation).length() < (as->radius + bs->radius) ) )
			{
				shared_ptr<Contact> 		c(new Contact);
				shared_ptr<SDECPermanentLink>	link(new SDECPermanentLink);

				c->id1 				= idA;
				c->id2 				= idB;
				link->initialKn			= 500000;
				link->initialKs			= 50000;
				link->heta			= 1;
				link->initialEquilibriumDistance= (a->se3.translation - b->se3.translation).length();
				link->radius1			= as->radius - fabs(as->radius - bs->radius)*0.5;
				link->radius2			= bs->radius - fabs(as->radius - bs->radius)*0.5;
				link->knMax			= 75000;
				link->ksMax			= 7500;

				c->interactionGeometry = link;

				//cout << "adding: " << idA << " " << idB << endl;
				rootBody->permanentInteractions.push_back(c);
			}
		}
	}



	IOManager::saveToFile("XMLManager", "../data/SDECLinkedSpheres.xml", "rootBody", rootBody);
}
