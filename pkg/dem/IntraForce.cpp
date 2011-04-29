#include<yade/pkg/dem/IntraForce.hpp>
#include<yade/pkg/dem/Particle.hpp>
#include<cstdlib>

YADE_PLUGIN(dem,(IntraFunctor)(IntraDispatcher)(In2_Sphere_ElastMat));

void IntraDispatcher::action(){
	DemField* field=dynamic_cast<DemField*>(scene->field.get());
	assert(field);
	updateScenePtr();
	FOREACH(const shared_ptr<Particle>& p, field->particles){
		operator()(p->shape,p->material,p);
	}
};

void In2_Sphere_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle){
	FOREACH(const Particle::MapParticleContact::value_type& I,particle->contacts){
		const shared_ptr<Contact>& C(I.second); if(!C->isReal()) continue;
		bool isPA=(C->pA==particle);
		int sign=(isPA?1:-1);
		Vector3r F=C->geom->node->ori.conjugate()*C->phys->force*sign;
		Vector3r T=(C->phys->torque==Vector3r::Zero() ? Vector3r::Zero() : C->geom->node->ori.conjugate()*C->phys->torque)*sign;
		#ifdef YADE_DEBUG
			if(isnan(F[0])||isnan(F[1])||isnan(F[2])||isnan(T[0])||isnan(T[1])||isnan(T[2])){
				ostringstream oss; oss<<"NaN force/torque on particle #"<<particle->id<<" from ##"<<C->pA->id<<"+"<<C->pB->id<<":\n\tF="<<F<<", T="<<T; //"\n\tlocal F="<<C->phys->force*sign<<", T="<<C->phys->torque*sign<<"\n";
				throw std::runtime_error(oss.str().c_str());
			}
		#endif
		Vector3r xc=C->geom->node->pos-sh->nodes[0]->pos+((!isPA && scene->isPeriodic) ? scene->cell->intrShiftPos(C->cellDist) : Vector3r::Zero());
		sh->nodes[0]->dyn->addForceTorque(F,xc.cross(F)+T);
	}
}
