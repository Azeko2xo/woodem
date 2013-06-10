#include<woo/pkg/dem/Conveyor.hpp>
#include<woo/pkg/dem/Factory.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Funcs.hpp>

// hack; check if that is really needed
#include<woo/pkg/dem/InsertionSortCollider.hpp>



WOO_PLUGIN(dem,(ConveyorFactory));
CREATE_LOGGER(ConveyorFactory);


void ConveyorFactory::postLoad(ConveyorFactory&,void*){
	if(radii.size()==centers.size() && !radii.empty()){
		sortPacking();
	}
	if(!radii.empty() && material){
		Real vol=0; for(const Real& r: radii) vol+=(4./3.)*M_PI*pow(r,3);
		if(!isnan(prescribedRate)){
			vel=prescribedRate/(vol*material->density/cellLen);
		}
		avgRate=(vol*material->density/cellLen)*vel; // (kg/m)*(m/s)→kg/s
	} else {
		avgRate=NaN;
	}
}


void ConveyorFactory::sortPacking(){
	if(radii.size()!=centers.size()) throw std::logic_error("ConveyorFactory.sortPacking: radii.size()!=centers.size()");
	if(!cellLen>0 /*catches NaN as well*/) ValueError("ConveyorFactor.cellLen must be positive (not "+to_string(cellLen)+")");
	// copy arrays to structs first
	typedef std::tuple<Vector3r,Real> CentRad;
	vector<CentRad> vecCentRad(radii.size());
	size_t N=radii.size();
	for(size_t i=0;i<N;i++){
		if(centers[i][0]<0 || centers[i][0]>=cellLen) centers[i][0]=Cell::wrapNum(centers[i][0],cellLen);
		vecCentRad[i]=std::make_tuple(centers[i],radii[i]);
	}
	// sort according to the x-coordinate
	std::sort(vecCentRad.begin(),vecCentRad.end(),[](const CentRad& a, const CentRad& b)->bool{ return std::get<0>(a)[0]<std::get<0>(b)[0]; });
	for(size_t i=0;i<N;i++){
		std::tie(centers[i],radii[i])=vecCentRad[i];
	}
}

void ConveyorFactory::particleLeavesBarrier(const shared_ptr<Particle>& p){
	auto& dyn=p->shape->nodes[0]->getData<DemData>();
	dyn.setBlockedNone();
	p->shape->color=isnan(color)?Mathr::UnitRandom():color;
	// assign velocity with randomized lateral components
	if(!isnan(relLatVel) && relLatVel!=0){
		dyn.vel=node->ori*(Vector3r(vel,(2*Mathr::UnitRandom()-1)*relLatVel*vel,(2*Mathr::UnitRandom()-1)*relLatVel*vel));
		static bool warnedEnergyIgnored=false;
		if(scene->trackEnergy && !warnedEnergyIgnored){
			warnedEnergyIgnored=true;
			LOG_WARN("FIXME: ConveyorFactory.relLatVel is ignored when computing kinetic energy of new particles; energy balance will not be accurate.");
		}
	}
}

void ConveyorFactory::notifyDead(){
	if(dead){
		// we were just made dead; remove the barrier and set zero rate
		if(zeroRateAtStop) currRate=0.;
		/* remove particles from the barrier */
		for(const auto& p: barrier){ particleLeavesBarrier(p); }
		barrier.clear();
	} else {
		// we were made alive after being dead;
		// adjust last runs so that we don't think we need to catch up with the whole time being dead
		PeriodicEngine::fakeRun();
	}
}

void ConveyorFactory::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	if(isnan(vel)) ValueError("ConveyorFactory.vel==NaN");
	if(radii.empty() || radii.size()!=centers.size()) ValueError("ConveyorFactory: radii and values must be same-length and non-empty.");
	if(barrierLayer<0){
		Real maxRad=-Inf;
		for(const Real& r:radii) maxRad=max(r,maxRad);
		if(isinf(maxRad)) throw std::logic_error("ConveyorFactory.radii: infinite value?");
		barrierLayer=maxRad*abs(barrierLayer);
		LOG_INFO("Setting barrierLayer="<<barrierLayer);
	}
	auto I=barrier.begin();
	while(I!=barrier.end()){
		const auto& p=*I;
		if((node->ori.conjugate()*(p->shape->nodes[0]->pos-node->pos))[0]>barrierLayer){
			particleLeavesBarrier(p);
			I=barrier.erase(I); // erase and advance
		} else {
			I++; // just advance
		}
	}

	Real lenToDo;
	if(stepPrev<0){ // first time run
		if(startLen<=0) ValueError("ConveyorFactory.startLen must be positive or NaN (not "+to_string(startLen)+")");
		if(!isnan(startLen)) lenToDo=startLen;
		else lenToDo=(stepPeriod>0?stepPeriod*scene->dt*vel:scene->dt*vel);
	} else {
		if(!isnan(virtPrev)) lenToDo=(scene->time-virtPrev)*vel; // time elapsed since last run
		else lenToDo=scene->dt*vel*(stepPeriod>0?stepPeriod:1);
	}
	Real stepMass=0;
	LOG_DEBUG("lenToDo="<<lenToDo<<", time="<<scene->time<<", virtPrev="<<virtPrev<<", vel="<<vel);
	Real lenDone=0;
	while(true){
		// done foerver
		if((maxMass>0 && mass>maxMass) || (maxNum>0 && num>=maxNum)){
			dead=true;
			notifyDead();
			return;
		}
		LOG_TRACE("Doing next particle: mass/maxMass="<<mass<<"/"<<maxMass<<", num/maxNum"<<num<<"/"<<maxNum);
		if(nextIx<0) nextIx=centers.size()-1;
		Real nextX=centers[nextIx][0];
		Real dX=lastX-nextX+((lastX<nextX && (nextIx==(int)centers.size()-1))?cellLen:0); // when wrapping, fix the difference
		LOG_DEBUG("len toDo/done "<<lenToDo<<"/"<<lenDone<<", lastX="<<lastX<<", nextX="<<nextX<<", dX="<<dX<<", nextIx="<<nextIx);
		if(isnan(abs(dX)) || isnan(abs(nextX)) || isnan(abs(lenDone)) || isnan(abs(lenToDo))) std::logic_error("ConveyorFactory: some parameters are NaN.");
		if(lenDone+dX>lenToDo){
			// the next sphere would not fit
			lastX=Cell::wrapNum(lastX-(lenToDo-lenDone),cellLen); // put lastX before the next sphere
			LOG_DEBUG("Conveyor done: next sphere "<<nextIx<<" would not fit, setting lastX="<<lastX);
			break;
		}
		lastX=Cell::wrapNum(nextX,cellLen);
		lenDone+=dX;

		auto sphere=DemFuncs::makeSphere(radii[nextIx],material);
		sphere->mask=mask;
		const auto& n=sphere->shape->nodes[0];
		auto& dyn=n->getData<DemData>();
		Real realSphereX=lenToDo-lenDone;
		//LOG_TRACE("x="<<x<<", "<<lenToDo<<"-("<<1+currWraps<<")*"<<cellLen<<"+"<<currX);
		n->pos=node->pos+node->ori*Vector3r(realSphereX,centers[nextIx][1],centers[nextIx][2]);
		dyn.vel=node->ori*(Vector3r::UnitX()*vel);

		if(scene->trackEnergy){
			scene->energy->add(-DemData::getEk_any(n,true,/*rotation zero, don't even compute it*/false,scene),"kinFactory",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		}


		if(realSphereX<barrierLayer){
			sphere->shape->color=isnan(barrierColor)?Mathr::UnitRandom():barrierColor;
			barrier.push_back(sphere);
			dyn.setBlockedAll();
		} else {
			sphere->shape->color=isnan(color)?Mathr::UnitRandom():color;
		}

		dem->particles->insert(sphere);
		if(save) genDiamMass.push_back(Vector2r(2*radii[nextIx],dyn.mass));
		LOG_TRACE("New sphere #"<<sphere->id<<", r="<<radii[nextIx]<<" at "<<n->pos.transpose());
		#ifdef WOO_OPENGL
			boost::mutex::scoped_lock lock(dem->nodesMutex);
		#endif
		dyn.linIx=dem->nodes.size();
		dem->nodes.push_back(n);

		stepMass+=dyn.mass;
		mass+=dyn.mass;
		num+=1;
		nextIx-=1; // decrease; can go negative, handled at the beginning of the loop
	};

	setCurrRate(stepMass/(/*time*/lenToDo/vel));

	dem->contacts->dirty=true; // re-initialize the collider
	#if 1
		for(const auto& e: scene->engines){
			if(dynamic_pointer_cast<InsertionSortCollider>(e)){ e->cast<InsertionSortCollider>().forceInitSort=true; break; }
		}
	#endif
}

py::object ConveyorFactory::pyDiamMass() const {
	py::list diam, mass;
	for(const Vector2r& vv: genDiamMass){ diam.append(vv[0]); mass.append(vv[1]); }
	return py::object(py::make_tuple(diam,mass));
}

Real ConveyorFactory::pyMassOfDiam(Real min, Real max) const {
	Real ret=0.;
	for(const Vector2r& vv: genDiamMass){
		if(vv[0]>=min && vv[0]<=max) ret+=vv[1];
	}
	return ret;
}


py::tuple ConveyorFactory::pyPsd(bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) const {
	if(!save) throw std::runtime_error("ConveyorFactory.save must be True for calling ConveyorFactory.psd()");
	vector<Vector2r> psd=DemFuncs::psd(genDiamMass,/*cumulative*/cumulative,/*normalize*/normalize,num,dRange,
		/*radius getter*/[](const Vector2r& diamMass) ->Real { return diamMass[0]; },
		/*weight getter*/[&](const Vector2r& diamMass) -> Real{ return mass?diamMass[1]:1.; }
	);
	py::list diameters,percentage;
	for(const auto& dp: psd){ diameters.append(dp[0]); percentage.append(dp[1]); }
	return py::make_tuple(diameters,percentage);
}

