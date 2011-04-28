#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/ContactContainer.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>

YADE_PLUGIN(dem,(DemField)(Particle)(Contact)(Shape)(Material)(Bound)(ContactContainer));

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
