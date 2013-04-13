#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<cstdlib>

WOO_PLUGIN(dem,(IntraFunctor)(IntraForce));

CREATE_LOGGER(IntraForce);

void IntraForce::run(){
	DemField& dem=field->cast<DemField>();
	updateScenePtr();
	size_t size=dem.particles->size();
	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)
	#endif
	for(size_t i=0; i<size; i++){
		const shared_ptr<Particle>& p((*dem.particles)[i]);
		if(!p) continue;
		if(!p->shape || !p->material){
			LOG_ERROR("#"<<i<<" has no shape/material.");
			continue;
		}
		operator()(p->shape,p->material,p);
	}
};

