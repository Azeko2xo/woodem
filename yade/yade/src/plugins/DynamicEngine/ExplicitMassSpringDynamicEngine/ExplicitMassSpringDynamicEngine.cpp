#include "ExplicitMassSpringDynamicEngine.hpp"
#include "Omega.hpp"
#include "MassSpringBody.hpp"
#include "Mesh2D.hpp"

ExplicitMassSpringDynamicEngine::ExplicitMassSpringDynamicEngine () : DynamicEngine()
{
	first = true;
}

ExplicitMassSpringDynamicEngine::~ExplicitMassSpringDynamicEngine ()
{

}

void ExplicitMassSpringDynamicEngine::postProcessAttributes(bool)
{

}

void ExplicitMassSpringDynamicEngine::registerAttributes()
{
}


void ExplicitMassSpringDynamicEngine::respondToCollisions(Body * body)
{

	float dt = Omega::instance().dt;
	MassSpringBody * massSpring = dynamic_cast<MassSpringBody*>(body);

	Vector3r gravity = Omega::instance().getGravity();
	
	float damping	= massSpring->damping;
	float stiffness	= massSpring->stiffness;

	shared_ptr<Mesh2D> mesh = dynamic_pointer_cast<Mesh2D>(massSpring->gm);
	vector<Vector3r>& vertices = mesh->vertices;
	vector<Edge>& edges	  = mesh->edges;

	if (first)
	{		
		forces.resize(vertices.size());		
		prevVelocities.resize(vertices.size());
		/*vector<NodeProperties>::iterator pi = massSpring->properties.begin();
		vector<NodeProperties>::iterator piEnd = massSpring->properties.end();
		vector<Vector3r>::iterator pvi = prevVelocities.begin();
		for( ; pi!=piEnd ; ++pi,++pvi)
			*pvi = (*pi).velocity;*/
	}
	
	std::fill(forces.begin(),forces.end(),Vector3r(0,0,0));
	
	vector<pair<int,Vector3r> >::iterator efi    = massSpring->externalForces.begin();
	vector<pair<int,Vector3r> >::iterator efiEnd = massSpring->externalForces.end();
	for(; efi!=efiEnd; ++efi)
		forces[(*efi).first] += (*efi).second;
	
	vector<Edge>::iterator ei = edges.begin();
	vector<Edge>::iterator eiEnd = edges.end();
	for(int i=0 ; ei!=eiEnd; ++ei,i++)
	{
		Vector3r v1 = vertices[(*ei).first];
		Vector3r v2 = vertices[(*ei).second];
		float l  = (v2-v1).length();
		float l0 = massSpring->initialLengths[i];
		Vector3r dir = (v2-v1);
		dir.normalize();
		float e  = (l-l0)/l0;
		float relativeVelocity = dir.dot((massSpring->properties[(*ei).second].velocity-massSpring->properties[(*ei).first].velocity));
		Vector3r f3 = (e*stiffness+relativeVelocity*damping)*dir;
		forces[(*ei).first]  += f3;
		forces[(*ei).second] -= f3;
	}

		
	for(unsigned int i=0; i < vertices.size(); i++)
        {
		Vector3r acc = Vector3r(0,0,0);

		if (massSpring->properties[i].invMass!=0)
			acc = Omega::instance().getGravity() + forces[i]*massSpring->properties[i].invMass;
					
		if (!first)
			massSpring->properties[i].velocity = 0.997*(prevVelocities[i]+0.5*dt*acc); //0.995
		
		prevVelocities[i] = (massSpring->properties[i].velocity+0.5*dt*acc);
		
		vertices[i] += prevVelocities[i]*dt;				
        }
	
	// FIXME: where should we update bounding volume
	//body->updateBoundingVolume(body->se3);
	first = false;

	massSpring->externalForces.clear();
}

