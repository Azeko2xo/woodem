#include<woo/pkg/dem/Conveyor.hpp>
#include<woo/pkg/dem/Factory.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Funcs.hpp>

// hack; check if that is really needed
#include<woo/pkg/dem/InsertionSortCollider.hpp>



WOO_PLUGIN(dem,(ConveyorFactory));
CREATE_LOGGER(ConveyorFactory);


Real ConveyorFactory::packVol() const {
	Real ret=0;
	if(!clumps.empty()){ for(const auto& c: clumps){ assert(c); ret+=c->volume; }}
	else{ for(const Real& r: radii) ret+=(4/3.)*M_PI*pow(r,3); }
	return ret;
}

void ConveyorFactory::postLoad(ConveyorFactory&,void* attr){
	if(attr==NULL || attr==&spherePack || attr==&clumps || attr==&centers || attr==&radii){
		if(spherePack){
			// spherePack given
			clumps.clear(); centers.clear(); radii.clear();
			if(spherePack->hasClumps()) clumps=SphereClumpGeom::fromSpherePack(spherePack);
			else{
				centers.reserve(spherePack->pack.size()); radii.reserve(spherePack->pack.size());
				for(const auto& s: spherePack->pack){ centers.push_back(s.c); radii.push_back(s.r); }
			}
			if(spherePack->cellSize[0]>0) cellLen=spherePack->cellSize[0];
			else if(isnan(cellLen)) throw std::runtime_error("ConveyorFactory: spherePack.cellSize[0]="+to_string(spherePack->cellSize[0])+": must be positive, or cellLen must be given.");
			spherePack.reset();
		}
		// handles the case of clumps generated in the above block as well
		if(!clumps.empty()){
			// with clumps given explicitly
			if(radii.size()!=clumps.size() || centers.size()!=clumps.size()){
				radii.resize(clumps.size()); centers.resize(clumps.size());
				for(size_t i=0; i<clumps.size(); i++){
					radii[i]=clumps[i]->equivRad;
					centers[i]=clumps[i]->pos;
				}
				sortPacking();
			}
		} else { 
			// no clumps
			if(radii.size()==centers.size() && !radii.empty()) sortPacking();
		}
	}
	/* this block applies in too may cases to name all attributes here;
	   besides computing the volume, it is cheap, so do it at every postLoad to make sure it gets done
	*/
	// those are invalid values, change them to NaN
	if(massRate<=0) massRate=NaN;
	if(vel<=0) vel=NaN;

	Real vol=packVol();
	Real maxRate;
	Real rho=(material?material->density:NaN);
	// minimum velocity to achieve given massRate (if any)
	packVel=massRate*cellLen/(rho*vol);
	// maximum rate achievable with given velocity
	maxRate=vel*rho*vol/cellLen;
	LOG_INFO("l="<<cellLen<<" m, V="<<vol<<" m³, rho="<<rho<<" kg/m³, vel="<<vel<<" m/s, packVel="<<packVel<<" m/s, massRate="<<massRate<<" kg/s, maxRate="<<maxRate<<" kg/s");
	// comparisons are true only if neither operand is NaN
	// error here if vel is being set
	if(vel<packVel && attr==&vel) throw std::runtime_error("ConveyorFactory: vel="+to_string(vel)+" m/s < "+to_string(packVel)+" m/s - minimum to achieve desired massRate="+to_string(massRate));
	// otherwise show the massRate error instead (very small tolerance as FP might not get it right)
	if(massRate>maxRate*(1+1e-6)) throw std::runtime_error("ConveyorFactory: massRate="+to_string(massRate)+" kg/s > "+to_string(maxRate)+" - maximum to achieve desired vel="+to_string(vel)+" m/s");
	if(isnan(massRate)) massRate=maxRate;
	if(isnan(vel)) vel=packVel;
	LOG_INFO("packVel="<<packVel<<"m/s, vel="<<vel<<"m/s, massRate="<<massRate<<"kg/s, maxRate="<<maxRate<<"kg/s; dilution factor "<<massRate/maxRate);


	avgRate=(vol*rho/cellLen)*vel; // (kg/m)*(m/s)→kg/s
}

void ConveyorFactory::sortPacking(){
	if(radii.size()!=centers.size()) throw std::logic_error("ConveyorFactory.sortPacking: radii.size()!=centers.size()");
	if(hasClumps() && radii.size()!=clumps.size()) throw std::logic_error("ConveyorFactory.sortPacking: clumps not empty and clumps.size()!=centers.size()");
	if(!cellLen>0 /*catches NaN as well*/) ValueError("ConveyorFactory.cellLen must be positive (not "+to_string(cellLen)+")");
	size_t N=radii.size();
	// sort spheres according to their x-coordinate
	// copy arrays to structs first
	struct CRC{ Vector3r c; Real r; shared_ptr<SphereClumpGeom> clump; };
	vector<CRC> ccrrcc(radii.size());
	bool doClumps=hasClumps();
	for(size_t i=0;i<N;i++){
		if(centers[i][0]<0 || centers[i][0]>=cellLen) centers[i][0]=Cell::wrapNum(centers[i][0],cellLen);
		ccrrcc[i]=CRC{centers[i],radii[i],doClumps?clumps[i]:shared_ptr<SphereClumpGeom>()};
	}
	// sort according to the x-coordinate
	std::sort(ccrrcc.begin(),ccrrcc.end(),[](const CRC& a, const CRC& b)->bool{ return a.c[0]<b.c[0]; });
	for(size_t i=0;i<N;i++){
		centers[i]=ccrrcc[i].c; radii[i]=ccrrcc[i].r;
		if(doClumps) clumps[i]=ccrrcc[i].clump;
	}
}

void ConveyorFactory::nodeLeavesBarrier(const shared_ptr<Node>& n){
	auto& dyn=n->getData<DemData>();
	dyn.setBlockedNone();
	Real c=isnan(color)?Mathr::UnitRandom():color;
	setAttachedParticlesColor(n,c);
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
		for(const auto& n: barrier){ nodeLeavesBarrier(n); }
		barrier.clear();
	} else {
		// we were made alive after being dead;
		// adjust last runs so that we don't think we need to catch up with the whole time being dead
		PeriodicEngine::fakeRun();
	}
}

void ConveyorFactory::setAttachedParticlesColor(const shared_ptr<Node>& n, Real c){
	assert(n); assert(n->hasData<DemData>());
	auto& dyn=n->getData<DemData>();
	if(!dyn.isClump()){
		// it is possible that the particle was meanwhile removed from the simulation, whence the check
		// we can suppose the node has one single particle (unless someone abused the node meanwhile with another particle!)
		if(!dyn.parRef.empty()) (*dyn.parRef.begin())->shape->color=c;
	} else {
		for(const auto& nn: dyn.cast<ClumpData>().nodes){
			assert(nn); assert(nn->hasData<DemData>());
			auto& ddyn=nn->getData<DemData>();
			if(!ddyn.parRef.empty()) (*ddyn.parRef.begin())->shape->color=c;
		}
	}
}


void ConveyorFactory::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	if(radii.empty() || radii.size()!=centers.size()) ValueError("ConveyorFactory: radii and values must be same-length and non-empty.");
	if(isnan(vel) || isnan(massRate) || !material) ValueError("ConveyorFactory: vel, massRate, material must be given.");
	if(barrierLayer<0){
		Real maxRad=-Inf;
		for(const Real& r:radii) maxRad=max(r,maxRad);
		if(isinf(maxRad)) throw std::logic_error("ConveyorFactory.radii: infinite value?");
		barrierLayer=maxRad*abs(barrierLayer);
		LOG_INFO("Setting barrierLayer="<<barrierLayer);
	}
	auto I=barrier.begin();
	while(I!=barrier.end()){
		const auto& n=*I;
		if((node->ori.conjugate()*(n->pos-node->pos))[0]>barrierLayer){
			nodeLeavesBarrier(n);
			I=barrier.erase(I); // erase and advance
		} else {
			I++; // just advance
		}
	}

	Real lenToDo;
	if(stepPrev<0){ // first time run
		if(startLen<=0) ValueError("ConveyorFactory.startLen must be positive or NaN (not "+to_string(startLen)+")");
		if(!isnan(startLen)) lenToDo=startLen;
		else lenToDo=(stepPeriod>0?stepPeriod*scene->dt*packVel:scene->dt*packVel);
	} else {
		if(!isnan(virtPrev)) lenToDo=(scene->time-virtPrev)*packVel; // time elapsed since last run
		else lenToDo=scene->dt*packVel*(stepPeriod>0?stepPeriod:1);
	}
	Real stepMass=0;
	LOG_DEBUG("lenToDo="<<lenToDo<<", time="<<scene->time<<", virtPrev="<<virtPrev<<", packVel="<<packVel);
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

		Real realSphereX=(vel/packVel)*(lenToDo-lenDone); // scale x-position by dilution (>1)
		Vector3r newPos=node->pos+node->ori*Vector3r(realSphereX,centers[nextIx][1],centers[nextIx][2]);

		shared_ptr<Node> n;
		
		if(!hasClumps()){
			auto sphere=DemFuncs::makeSphere(radii[nextIx],material);
			sphere->mask=mask;
			n=sphere->shape->nodes[0];
			//LOG_TRACE("x="<<x<<", "<<lenToDo<<"-("<<1+currWraps<<")*"<<cellLen<<"+"<<currX);
			dem->particles->insert(sphere);
			n->pos=newPos;
			LOG_TRACE("New sphere #"<<sphere->id<<", r="<<radii[nextIx]<<" at "<<n->pos.transpose());
		} else {
			const auto& clump=clumps[nextIx];
			vector<shared_ptr<Particle>> spheres;
			std::tie(n,spheres)=clump->makeClump(material,/*pos*/newPos,/*ori*/Quaternionr::Identity(),/*scale*/1.);
			for(auto& sphere: spheres){
				sphere->mask=mask;
				dem->particles->insert(sphere);
				LOG_TRACE("[cllump] new sphere #"<<sphere->id<<", r="<<radii[nextIx]<<" at "<<n->pos.transpose());
			}
		}

		auto& dyn=n->getData<DemData>();
		if(realSphereX<barrierLayer){
			setAttachedParticlesColor(n,isnan(barrierColor)?Mathr::UnitRandom():barrierColor);
			barrier.push_back(n);
			dyn.setBlockedAll();
		} else {
			setAttachedParticlesColor(n,isnan(color)?Mathr::UnitRandom():color);
		}

		// set velocity;
		dyn.vel=node->ori*(Vector3r::UnitX()*vel);
		if(scene->trackEnergy){
			scene->energy->add(-DemData::getEk_any(n,true,/*rotation zero, don't even compute it*/false,scene),"kinFactory",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		}

		if(save) genDiamMass.push_back(Vector2r(2*radii[nextIx],dyn.mass));
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

	setCurrRate(stepMass/(/*time*/lenToDo/packVel));

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

