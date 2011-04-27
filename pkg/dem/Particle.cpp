#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>

YADE_PLUGIN(dem,(DemField)(Particle)(Contact)(Shape)(Material)(Bound));

py::dict Particle::pyContacts(){
	py::dict ret;
	FOREACH(MapParticleContact::value_type i,contacts){
		ret[i.first]=i.second;
	};
	return ret;
}

void DemField::addContact(shared_ptr<Contact> c){
	c->pA->contacts[c->pB->id]=c;
	c->pB->contacts[c->pA->id]=c;
};

void DemField::removeContact(shared_ptr<Contact> c){
	c->pA->contacts.erase(c->pB->id);
	c->pB->contacts.erase(c->pA->id);
};

shared_ptr<Contact> DemField::findContact(Particle::id_t idA, Particle::id_t idB){
	if(!particles->exists(idA)) return shared_ptr<Contact>();
	Particle::MapParticleContact::iterator I((*particles)[idA]->contacts.find(idB));
	if(I!=(*particles)[idA]->contacts.end()) return I->second;
	return shared_ptr<Contact>();
};

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
