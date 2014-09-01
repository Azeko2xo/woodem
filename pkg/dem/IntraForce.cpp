#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<boost/range/algorithm/find_if.hpp>
#include<cstdlib>

WOO_PLUGIN(dem,(IntraFunctor)(IntraForce));
WOO_IMPL__CLASS_BASE_DOC_PY(woo_dem_IntraFunctor__CLASS_BASE_DOC_PY);

CREATE_LOGGER(IntraForce);

void IntraForce::addIntraStiffness(const shared_ptr<Particle>& p, const shared_ptr<Node>& n, Vector3r& ktrans, Vector3r& krot) {
	bool swap; // ignored
	shared_ptr<IntraFunctor> functor=getFunctor2D(p->shape,p->material,swap);
	if(!functor) return;
	assert(boost::range::find_if(p->shape->nodes,[&n](const shared_ptr<Node> n2){ return n.get()==n2.get(); })!=p->shape->nodes.end());
	functor->addIntraStiffnesses(p,n,ktrans,krot);
};

CREATE_LOGGER(IntraFunctor);
void IntraFunctor::addIntraStiffnesses(const shared_ptr<Particle>&, const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const{
	LOG_WARN("IntraFunctor::addIntraStiffnesses: not overridden for "+pyStr()+", internal stiffness ignored for timestep computation.");
}


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
		operator()(p->shape,p->material,p,/*skipContacts*/false);
	}
};

