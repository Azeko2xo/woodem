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
	bool deleting=(markMask==0);
	for(size_t i=0; i<dem->nodes.size(); i++){
		const auto& n=dem->nodes[i];
		if(inside!=box.contains(n->pos)) continue; // node inside, do nothing
		if(!n->hasData<DemData>()) continue;
		const auto& dyn=n->getData<DemData>();
		// check all particles attached to this node
		for(const Particle* p: dyn.parRef){
			if(!p || !p->shape) continue;
			// mask is checked for both deleting and marking
			if(mask && !(mask & p->mask)) continue;
			// marking, but mask has the markMask already
			if(!deleting && (markMask&p->mask)==markMask) continue;
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
					if(!deleting && (markMask&p->mask)==markMask) goto otherNotOk;
					// don't check positions of all nodes of each particles, just assume that
					// all nodes of that particle are in the clump
				}
					
			}
			LOG_TRACE("DemField.nodes["<<i<<"]: clump marked for deletion, with all its particles.");
			delClumpIxs.insert(i);
			otherNotOk: ;
		}
	}
	for(const auto& id: delParIds){
		const shared_ptr<Particle>& p((*dem->particles)[id]);
		if(deleting && scene->trackEnergy) scene->energy->add(DemData::getEk_any(p->shape->nodes[0],true,true,scene),"kinDelete",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		const Real& m=p->shape->nodes[0]->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		if(dynamic_cast<Sphere*>(p->shape.get())){
			auto& s=p->shape->cast<Sphere>();
			Real r=s.radius;
			if(recoverRadius) r=cbrt(3*m/(4*M_PI*p->material->density));
			if(save) diamMass.push_back(Vector2r(2*r,m));
		}
		LOG_TRACE("DemField.par["<<id<<"] will be "<<(deleting?"deleted.":"marked."));
		if(deleting) dem->removeParticle(id);
		else p->mask|=markMask;
		LOG_DEBUG("DemField.par["<<id<<"] "<<(deleting?"deleted.":"marked."));
	}
	for(const auto& ix: delClumpIxs){
		const shared_ptr<Node>& n(dem->nodes[ix]);
		if(deleting && scene->trackEnergy) scene->energy->add(DemData::getEk_any(n,true,true,scene),"kinDelete",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		Real m=n->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		if(save) diamMass.push_back(Vector2r(2*n->getData<DemData>().cast<ClumpData>().equivRad,m));
		LOG_TRACE("DemField.nodes["<<ix<<"] (clump) will be "<<(deleting?"deleted":"marked")<<", with all its particles.");
		if(deleting) dem->removeClump(ix);
		else {
			// apply markMask on all clumps (all particles attached to all nodes in this clump)
			const auto& cd=n->getData<DemData>().cast<ClumpData>();
			for(const auto& nn: cd.nodes) for(Particle* p: nn->getData<DemData>().parRef) p->mask|=markMask;
		}
		LOG_TRACE("DemField.nodes["<<ix<<"] (clump) "<<(deleting?"deleted.":"marked."));
	}

	// use the whole stepPeriod for the first time (might be residuum from previous packing), if specified
	// otherwise the rate might be artificially high at the beginning
	Real currRateNoSmooth=stepMass/(((stepPrev<0 && stepPeriod>0?stepPeriod:scene->step-stepPrev))*scene->dt); 
	if(isnan(currRate)||stepPrev<0) currRate=currRateNoSmooth;
	else currRate=(1-currRateSmooth)*currRate+currRateSmooth*currRateNoSmooth;
}
py::object BoxDeleter::pyDiamMass(bool zipped) const {
	if(!zipped){
		py::list dd, mm;
		for(const auto& dm: diamMass){ dd.append(dm[0]); mm.append(dm[1]); }
		return py::make_tuple(dd,mm);
	} else {
		py::list ret;
		for(const auto& dm: diamMass){ ret.append(dm); }
		return ret;
	}
}

Real BoxDeleter::pyMassOfDiam(Real min, Real max) const {
	Real ret=0.;
	for(const auto& dm: diamMass){ if(dm[0]>=min && dm[0]<=max) ret+=dm[1]; }
	return ret;
}

py::object BoxDeleter::pyPsd(bool mass, bool cumulative, bool normalize, int num, const Vector2r& dRange, bool zip, bool emptyOk){
	if(!save) throw std::runtime_error("BoxDeleter.save must be True for calling BoxDeleter.psd()");
	vector<Vector2r> psd=DemFuncs::psd(/*deleted*/diamMass,cumulative,normalize,num,dRange,
		/*diameter getter*/[](const Vector2r& dm)->Real{ return dm[0]; },
		/*weight getter*/[&mass](const Vector2r& dm)->Real{ return mass?dm[1]:1.; },
		/*emptyOk*/ emptyOk
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

