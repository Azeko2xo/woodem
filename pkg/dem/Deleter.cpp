#include<woo/pkg/dem/Deleter.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Funcs.hpp>
WOO_PLUGIN(dem,(BoxDeleter));
CREATE_LOGGER(BoxDeleter);

void BoxDeleter::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	Real stepMass=0.;
	std::set<Particle::id_t> delParIds;
	std::set<Particle::id_t> delClumpIxs;
	for(size_t i=0; i<dem->nodes.size(); i++){
		const auto& n=dem->nodes[i];
		if(inside!=box.contains(n->pos)) continue; // node inside, do nothing
		if(!n->hasData<DemData>()) continue;
		const auto& dyn=n->getData<DemData>();
		// check all particles attached to this nodes
		for(const Particle* p: dyn.parRef){
			if(!p || !p->shape) continue;
			if(mask && !(mask & p->mask)) continue;
			// check that all other nodes of that particle may also be deleted
			bool otherOk=true;
			for(const auto& nn: p->shape->nodes){
				// useless to check n again
				if(nn.get()!=n.get() && !(inside!=box.contains(nn->pos))){ otherOk=false; break; }
			}
			if(!otherOk) continue;
			LOG_TRACE("DemField.par["<<i<<"] marked for deletion.");
			delParIds.insert(p->id);
		}
		// if this is a clump, check positions of all attached nodes, and masks of their particles
		if(dyn.isClump()){
			assert(dynamic_pointer_cast<ClumpData>(n->getDataPtr<DemData>()));
			const auto& cd=n->getDataPtr<DemData>()->cast<ClumpData>();
			for(const auto& nn: cd.nodes){
				if(inside!=box.contains(nn->pos)) goto otherNotOk;
				for(const Particle* p: nn->getData<DemData>().parRef){
					assert(p);
					if(mask && !(mask & p->mask)) goto otherNotOk;
					// don't check positions of all nodes of each particles, just assume that
					// all nodes of that particle are in the clump
				}
					
			}
			LOG_TRACE("DemField.nodes["<<i<<"]: clump marked for deletion, with all its particles.");
			delClumpIxs.insert(i);
			otherNotOk: ;
		}
	}
	// remove particles marked for deletion
	for(const auto& id: delParIds){
		const shared_ptr<Particle>& p((*dem->particles)[id]);
		if(scene->trackEnergy) scene->energy->add(DemData::getEk_any(p->shape->nodes[0],true,true,scene),"kinDelete",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		// if(save) deleted.push_back(p);
		const Real& m=p->shape->nodes[0]->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		if(dynamic_cast<Sphere*>(p->shape.get())){
			auto& s=p->shape->cast<Sphere>();
			if(recoverRadius) s.radius=cbrt(3*m/(4*M_PI*p->material->density));
			if(save) diamMass.push_back(Vector2r(2*s.radius,m));
		}
		LOG_TRACE("DemField.par["<<id<<"] will be deleted.");
		dem->removeParticle(id);
		LOG_DEBUG("DemField.par["<<id<<"] deleted.");
	}
	for(const auto& ix: delClumpIxs){
		const shared_ptr<Node>& n(dem->nodes[ix]);
		if(scene->trackEnergy) scene->energy->add(DemData::getEk_any(n,true,true,scene),"kinDelete",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		Real m=n->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		if(save) diamMass.push_back(Vector2r(2*n->getData<DemData>().cast<ClumpData>().equivRad,m));
		LOG_TRACE("DemField.nodes["<<ix<<"] (clump) will be deleted, with all its particles.");
		dem->removeClump(ix);
		LOG_TRACE("DemField.nodes["<<ix<<"] (clump) deleted.");
	}


	// use the whole stepPeriod for the first time (might be residuum from previous packing), if specified
	// otherwise the rate might be artificially high at the beginning
	Real currRateNoSmooth=stepMass/(((stepPrev<0 && stepPeriod>0?stepPeriod:scene->step-stepPrev))*scene->dt); 
	if(isnan(currRate)||stepPrev<0) currRate=currRateNoSmooth;
	else currRate=(1-currRateSmooth)*currRate+currRateSmooth*currRateNoSmooth;
}
py::tuple BoxDeleter::pyDiamMass() const {
	py::list dd, mm;
	for(const auto& dm: diamMass){ dd.append(dm[0]); mm.append(dm[1]); }
	#if 0
		for(const auto& del: deleted){
			if(!del || !del->shape || del->shape->nodes.size()!=1 || !dynamic_pointer_cast<Sphere>(del->shape)) continue;
			Real d=2*del->shape->cast<Sphere>().radius;
			Real m=del->shape->nodes[0]->getData<DemData>().mass;
			dd.append(d); mm.append(m);
		}
	#endif
	return py::make_tuple(dd,mm);
}

Real BoxDeleter::pyMassOfDiam(Real min, Real max) const {
	Real ret=0.;
	#if 0
		for(const auto& del: deleted){
			if(!del || !del->shape || del->shape->nodes.size()!=1 || !dynamic_pointer_cast<Sphere>(del->shape)) continue;
			Real d=2*del->shape->cast<Sphere>().radius;
			if(d>=min && d<=max) ret+= del->shape->nodes[0]->getData<DemData>().mass;
		}
	#endif
	for(const auto& dm: diamMass){
		if(dm[0]>=min && dm[0]<=max) ret+=dm[1];
	}
	return ret;
}

py::object BoxDeleter::pyPsd(bool mass, bool cumulative, bool normalize, int num, const Vector2r& dRange, bool zip){
	if(!save) throw std::runtime_error("BoxDeleter.save must be True for calling BoxDeleter.psd()");
	vector<Vector2r> psd=DemFuncs::psd(/*deleted*/diamMass,cumulative,normalize,num,dRange,
		#if 0
			/*diameter getter*/[](const shared_ptr<Particle>&p) ->Real { return 2*p->shape->cast<Sphere>().radius; },
			/*weight getter*/[&](const shared_ptr<Particle>&p) -> Real{ return mass?p->shape->nodes[0]->getData<DemData>().mass:1.; }
		#endif
		/*diameter getter*/[](const Vector2r& dm)->Real{ return dm[0]; },
		/*weight getter*/[&mass](const Vector2r& dm)->Real{ return mass?dm[1]:1.; }
	);
	if(zip){
		py::list ret;
		for(const auto& dp: psd) ret.append(py::make_tuple(dp[0],dp[1]));
		return ret;
	} else {
		py::list diameters,percentage;
		for(const auto& dp: psd){ diameters.append(dp[0]); percentage.append(dp[1]); }
		return py::make_tuple(diameters,percentage);
	}
}

