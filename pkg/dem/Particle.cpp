#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/ContactContainer.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/lib/pyutil/except.hpp>

YADE_PLUGIN(dem,(CPhys)(CGeom)(CData)(DemField)(Particle)(Contact)(Shape)(Material)(Bound)(ContactContainer));

py::dict Particle::pyContacts(){
	py::dict ret;
	FOREACH(MapParticleContact::value_type i,contacts){
		ret[i.first]=i.second;
	};
	return ret;
}

void Contact::swapOrder(){
	if(geom || phys){ throw std::logic_error("Particles in contact cannot be swapped if they have geom or phys already."); }
	std::swap(pA,pB);
	cellDist*=-1;
}

void Contact::reset(){
	geom=shared_ptr<CGeom>();
	phys=shared_ptr<CPhys>();
	stepMadeReal=-1;
}

Particle::id_t Contact::pyId1(){ return pA->id; }
Particle::id_t Contact::pyId2(){ return pB->id; }

void Particle::checkOneNode(bool dyn){
	if(!shape || shape->nodes.size()!=1 || (dyn && !shape->nodes[0]->dyn)) yade::AttributeError("Particle #"+lexical_cast<string>(id)+" has no Shape, or the shape has no/multiple nodes"+(!dyn?".":", or node.dyn is None."));
}

Vector3r& Particle::getPos(){ checkOneNode(false); return shape->nodes[0]->pos; };
void Particle::setPos(const Vector3r& p){ checkOneNode(false); shape->nodes[0]->pos=p; }
Quaternionr& Particle::getOri(){ checkOneNode(false); return shape->nodes[0]->ori; };
void Particle::setOri(const Quaternionr& p){ checkOneNode(false); shape->nodes[0]->ori=p; }

Vector3r& Particle::getVel(){ checkOneNode(); return shape->nodes[0]->dyn->vel; };
void Particle::setVel(const Vector3r& p){ checkOneNode(); shape->nodes[0]->dyn->vel=p; }
Vector3r& Particle::getAngVel(){ checkOneNode(); return shape->nodes[0]->dyn->angVel; };
void Particle::setAngVel(const Vector3r& p){ checkOneNode(); shape->nodes[0]->dyn->angVel=p; }

Vector3r Particle::getForce(){ checkOneNode(); return shape->nodes[0]->dyn->force; };
Vector3r Particle::getTorque(){ checkOneNode(); return shape->nodes[0]->dyn->torque; };
