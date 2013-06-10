#include<woo/pkg/dem/Deleter.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Funcs.hpp>
WOO_PLUGIN(dem,(BoxDeleter));
CREATE_LOGGER(BoxDeleter);

void BoxDeleter::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	Real stepMass=0.;
	// iterate over indices so that iterators are not invalidated
	for(size_t i=0; i<dem->particles->size(); i++){
		const auto& p=(*dem->particles)[i];
		if(!p || !p->shape || p->shape->nodes.size()!=1) continue;
		if(mask & !(p->mask&mask)) continue;
		if(p->shape->nodes[0]->getData<DemData>().isClumped()) continue;
		const Vector3r pos=p->shape->nodes[0]->pos;
		if(inside!=box.contains(pos)) continue; // keep this particle
		LOG_TRACE("Saving particle #"<<i<<" to deleted");
		if(save) deleted.push_back((*dem->particles)[i]);
		if(scene->trackEnergy) scene->energy->add(DemData::getEk_any(p->shape->nodes[0],true,true,scene),"kinDelete",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		const Real& m=p->shape->nodes[0]->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		if(recoverRadius && dynamic_cast<Sphere*>(p->shape.get())){
			auto& s=p->shape->cast<Sphere>();
			s.radius=cbrt(3*m/(4*M_PI*p->material->density));
		}
		LOG_TRACE("Will delete particle #"<<i);
		dem->removeParticle(i);
		//dem->particles.remove(i);
		LOG_DEBUG("Particle #"<<i<<" deleted");
	}
	for(size_t i=0; i<dem->clumps.size(); i++){
		const auto& c=dem->clumps[i];
		if(inside!=box.contains(c->pos)){
			ClumpData& cd=c->getData<DemData>().cast<ClumpData>();
			// check mask of constituents first
			for(Particle::id_t memberId: cd.memberIds){
				const shared_ptr<Particle>& member=(*dem->particles)[memberId];
				if(mask & !(mask&member->mask)) goto keepClump;
			}
			if(scene->trackEnergy) scene->energy->add(DemData::getEk_any(c,true,true,scene),"kinDelete",kinEnergyIx,EnergyTracker::ZeroDontCreate);
			for(Particle::id_t memberId: cd.memberIds){
				deleted.push_back((*dem->particles)[memberId]);
			}
			num++;
			for(const auto& n: cd.nodes){ mass+=n->getData<DemData>().mass; stepMass+=n->getData<DemData>().mass; }
			dem->removeClump(i);
			LOG_DEBUG("Clump #"<<i<<" deleted");
			i--; // do this id again, might be a different clump now
		}
		keepClump: ;
	}
	// use the whole stepPeriod for the first time (might be residuum from previous packing), if specified
	// otherwise the rate might be artificially high at the beginning
	Real currRateNoSmooth=stepMass/(((stepPrev<0 && stepPeriod>0?stepPeriod:scene->step-stepPrev))*scene->dt); 
	if(isnan(currRate)||stepPrev<0) currRate=currRateNoSmooth;
	else currRate=(1-currRateSmooth)*currRate+currRateSmooth*currRateNoSmooth;
}
py::tuple BoxDeleter::pyDiamMass() const {
	py::list dd, mm;
	for(const auto& del: deleted){
		if(!del || !del->shape || del->shape->nodes.size()!=1 || !dynamic_pointer_cast<Sphere>(del->shape)) continue;
		Real d=2*del->shape->cast<Sphere>().radius;
		Real m=del->shape->nodes[0]->getData<DemData>().mass;
		dd.append(d); mm.append(m);
	}
	return py::make_tuple(dd,mm);
}

Real BoxDeleter::pyMassOfDiam(Real min, Real max) const {
	Real ret=0.;
	for(const auto& del: deleted){
		if(!del || !del->shape || del->shape->nodes.size()!=1 || !dynamic_pointer_cast<Sphere>(del->shape)) continue;
		Real d=2*del->shape->cast<Sphere>().radius;
		if(d>=min && d<=max) ret+= del->shape->nodes[0]->getData<DemData>().mass;
	}
	return ret;
}

py::object BoxDeleter::pyPsd(bool mass, bool cumulative, bool normalize, int num, const Vector2r& dRange, bool zip){
	if(!save) throw std::runtime_error("BoxDeleter.save must be True for calling BoxDeleter.psd()");
	vector<Vector2r> psd=DemFuncs::psd(deleted,cumulative,normalize,num,dRange,
		/*radius getter*/[](const shared_ptr<Particle>&p) ->Real { return 2*p->shape->cast<Sphere>().radius; },
		/*weight getter*/[&](const shared_ptr<Particle>&p) -> Real{ return mass?p->shape->nodes[0]->getData<DemData>().mass:1.; }
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

