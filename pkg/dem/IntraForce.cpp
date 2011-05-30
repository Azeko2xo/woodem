#include<yade/pkg/dem/IntraForce.hpp>
#include<yade/pkg/dem/Particle.hpp>
#include<cstdlib>

YADE_PLUGIN(dem,(IntraFunctor)(IntraForce));

void IntraForce::run(){
	DemField& dem=field->cast<DemField>();
	updateScenePtr();
	FOREACH(const shared_ptr<Particle>& p, dem.particles){
		operator()(p->shape,p->material,p);
	}
};

