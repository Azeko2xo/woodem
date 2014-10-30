#include<woo/pkg/dem/Conveyor.hpp>
#include<woo/pkg/dem/Inlet.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Funcs.hpp>
#include<woo/lib/smoothing/LinearInterpolate.hpp>

// hack; check if that is really needed
#include<woo/pkg/dem/InsertionSortCollider.hpp>

WOO_PLUGIN(dem,(ConveyorInlet));
CREATE_LOGGER(ConveyorInlet);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ConveyorInlet__CLASS_BASE_DOC_ATTRS_PY);

Real ConveyorInlet::critDt(){
	if(!material->isA<ElastMat>()){
		LOG_WARN("Material is not a ElastMat, unable to compute critical timestep.");
		return Inf;
	}
	const Real& density=material->density;
	const Real& young=material->cast<ElastMat>().young;
	// compute rMin
	Real rMin=Inf;
	if(hasClumps()){
		// ignores that spheres are clumped
		LOG_WARN("TODO: critDt may be bogus with clumps.");
		for(const auto& clump: clumps){
			for(const Real& r: clump->radii) rMin=min(rMin,r);
		}
	} else if(shapePack){
		shapePack->recomputeAll();
		for(const auto& r: shapePack->raws) rMin=min(rMin,r->equivRad);
	} else{
		if(radii.empty()) return Inf;
		for(const Real& r: radii) rMin=min(rMin,r);
	}
	// compute critDt from rMin
	if(isinf(rMin)){
		LOG_WARN("Minimum radius is infinite?!")
		return Inf;
	}
	return DemFuncs::spherePWaveDt(rMin,density,young);
}

Real ConveyorInlet::packVol() const {
	Real ret=0;
	if(!clumps.empty()){ for(const auto& c: clumps){ assert(c); ret+=c->volume; }}
	else{ for(const Real& r: radii) ret+=(4/3.)*M_PI*pow(r,3); }
	return ret;
}

void ConveyorInlet::postLoad(ConveyorInlet&,void* attr){
	if(attr==NULL || attr==&spherePack || attr==&clumps || attr==&centers || attr==&radii || attr==&shapePack){
		if(spherePack){
			// spherePack given
			clumps.clear(); centers.clear(); radii.clear();
			if(spherePack->hasClumps()) clumps=SphereClumpGeom::fromSpherePack(spherePack);
			else{
				centers.reserve(spherePack->pack.size()); radii.reserve(spherePack->pack.size());
				for(const auto& s: spherePack->pack){ centers.push_back(s.c); radii.push_back(s.r); }
			}
			if(spherePack->cellSize[0]>0) cellLen=spherePack->cellSize[0];
			else if(isnan(cellLen)) throw std::runtime_error("ConveyorInlet: spherePack.cellSize[0]="+to_string(spherePack->cellSize[0])+": must be positive, or cellLen must be given.");
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
		if(shapePack){
			if(!clumps.empty()) throw std::runtime_error("ConveyorInlet: shapePack and clumps cannot be specified simultaneously.");
			shapePack->sort(/*axis*/0);
		}
	}
	/* this block applies in too may cases to name all attributes here;
	   besides computing the volume, it is cheap, so do it at every postLoad to make sure it gets done
	*/
	// those are invalid values, change them to NaN
	if(massRate<=0) massRate=NaN;
	if(vel<=0) vel=NaN;

	Real vol=packVol();
	if(shapePack){
		vol=shapePack->solidVolume();
		cellLen=shapePack->cellSize[0];
	}
	Real maxRate;
	Real rho=(material?material->density:NaN);
	// minimum velocity to achieve given massRate (if any)
	packVel=massRate*cellLen/(rho*vol);
	// maximum rate achievable with given velocity
	maxRate=vel*rho*vol/cellLen;
	LOG_INFO("l="<<cellLen<<" m, V="<<vol<<" m³, rho="<<rho<<" kg/m³, vel="<<vel<<" m/s, packVel="<<packVel<<" m/s, massRate="<<massRate<<" kg/s, maxRate="<<maxRate<<" kg/s");
	// comparisons are true only if neither operand is NaN
	if(zTrim && vel>packVel) {
		// with z-trimming, reduce number of volume (proportionally) and call this function again
		Real zTrimVol=vol*(packVel/vel);
		LOG_INFO("zTrim in effect: trying to reduce volume from "<<vol<<" to "<<zTrimVol<<" (factor "<<(packVel/vel)<<")");
		if(!shapePack){
			sortPacking(zTrimVol);
			sortPacking(); // sort packing again, along x, as that will not be done in the above block again
		} else { // with shapePack
			shapePack->sort(/*axis*/2,/*trimVol*/zTrimVol);
			shapePack->sort(/*axis*/0);
		}
		zTrim=false;
		postLoad(*this,NULL);
		return;
	}
	// error here if vel is being set
	if(vel<packVel && attr==&vel) throw std::runtime_error("ConveyorInlet: vel="+to_string(vel)+" m/s < "+to_string(packVel)+" m/s - minimum to achieve desired massRate="+to_string(massRate));
	// otherwise show the massRate error instead (very small tolerance as FP might not get it right)
	if(massRate>maxRate*(1+1e-6)) throw std::runtime_error("ConveyorInlet: massRate="+to_string(massRate)+" kg/s > "+to_string(maxRate)+" - maximum to achieve desired vel="+to_string(vel)+" m/s");
	if(isnan(massRate)) massRate=maxRate;
	if(isnan(vel)) vel=packVel;
	LOG_INFO("packVel="<<packVel<<"m/s, vel="<<vel<<"m/s, massRate="<<massRate<<"kg/s, maxRate="<<maxRate<<"kg/s; dilution factor "<<massRate/maxRate);


	avgRate=(vol*rho/cellLen)*vel; // (kg/m)*(m/s)→kg/s

	// copy values to centers, radii
	if(shapePack){
		size_t N=shapePack->raws.size();
		centers.resize(N); radii.resize(N);
		LOG_WARN("Copying data from shapePack over to centers, radii ("<<N<<" items).");
		for(size_t i=0; i<N; i++){
			centers[i]=shapePack->raws[i]->pos;
			radii[i]=shapePack->raws[i]->equivRad;
		}
	}
}

void ConveyorInlet::sortPacking(const Real& zTrimVol){
	if(radii.size()!=centers.size()) throw std::logic_error("ConveyorInlet.sortPacking: radii.size()!=centers.size()");
	if(hasClumps() && radii.size()!=clumps.size()) throw std::logic_error("ConveyorInlet.sortPacking: clumps not empty and clumps.size()!=centers.size()");
	if(!(cellLen>0) /*catches NaN as well*/) ValueError("ConveyorInlet.cellLen must be positive (not "+to_string(cellLen)+")");
	size_t N=radii.size();
	// sort spheres according to their x-coordinate
	// copy arrays to structs first
	struct CRC{ Vector3r c; Real r; shared_ptr<SphereClumpGeom> clump; };
	vector<CRC> ccrrcc(radii.size());
	bool doClumps=hasClumps();
	for(size_t i=0;i<N;i++){
		if(centers[i][0]<0 || centers[i][0]>=cellLen) centers[i][0]=CompUtils::wrapNum(centers[i][0],cellLen);
		ccrrcc[i]=CRC{centers[i],radii[i],doClumps?clumps[i]:shared_ptr<SphereClumpGeom>()};
	}
	// sort according to the z-coordinate of the top, if z-trimming, otherwise according to the x-coord of the center
	if(zTrimVol>0) std::sort(ccrrcc.begin(),ccrrcc.end(),[](const CRC& a, const CRC& b)->bool{ return (a.c[2]+a.r<b.c[2]+b.r); });
	else std::sort(ccrrcc.begin(),ccrrcc.end(),[](const CRC& a, const CRC& b)->bool{ return a.c[0]<b.c[0]; });
	Real currVol=0.;
	for(size_t i=0;i<N;i++){
		centers[i]=ccrrcc[i].c; radii[i]=ccrrcc[i].r;
		if(doClumps) clumps[i]=ccrrcc[i].clump;
		// z-trimming
		if(zTrimVol>0){
			currVol+=(doClumps?clumps[i]->volume:(4/3.)*M_PI*pow(radii[i],3));
			if(currVol>zTrimVol){
				zTrimHt=centers[i][2]+radii[i];
				LOG_INFO("Z-sorted packing reached volume "<<currVol<<">="<<zTrimVol<<" at sphere/clump "<<i<<"/"<<N<<", zTrimHt="<<zTrimHt<<", discarding remaining spheres/clumps.");
				centers.resize(i+1); radii.resize(i+1); if(doClumps) clumps.resize(i+1);
				return;
			}
		}
	}
}

void ConveyorInlet::nodeLeavesBarrier(const shared_ptr<Node>& n){
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
			LOG_WARN("FIXME: ConveyorInlet.relLatVel is ignored when computing kinetic energy of new particles; energy balance will not be accurate.");
		}
	}
}

void ConveyorInlet::notifyDead(){
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

void ConveyorInlet::setAttachedParticlesColor(const shared_ptr<Node>& n, Real c){
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


void ConveyorInlet::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	if(radii.empty() || radii.size()!=centers.size()) ValueError("ConveyorInlet: radii and centers must be same-length and non-empty (if shapePack is given, radii and centers should have been populated automatically)");
	if(isnan(vel) || isnan(massRate) || !material) ValueError("ConveyorInlet: vel, massRate, material must be given.");
	if(clipX.size()!=clipZ.size()) ValueError("ConveyorInlet: clipX and clipZ must have the same size ("+to_string(clipX.size())+"!="+to_string(clipZ.size()));
	if(barrierLayer<0){
		Real maxRad=-Inf;
		for(const Real& r:radii) maxRad=max(r,maxRad);
		if(isinf(maxRad)) throw std::logic_error("ConveyorInlet.radii: infinite value?");
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
		if(startLen<=0) ValueError("ConveyorInlet.startLen must be positive or NaN (not "+to_string(startLen)+")");
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
		// done forever
		if(Inlet::everythingDone()) return;

		LOG_TRACE("Doing next particle: mass/maxMass="<<mass<<"/"<<maxMass<<", num/maxNum"<<num<<"/"<<maxNum);
		if(nextIx<0) nextIx=centers.size()-1;
		Real nextX=centers[nextIx][0];
		Real dX=lastX-nextX+((lastX<nextX && (nextIx==(int)centers.size()-1))?cellLen:0); // when wrapping, fix the difference
		LOG_DEBUG("len toDo/done "<<lenToDo<<"/"<<lenDone<<", lastX="<<lastX<<", nextX="<<nextX<<", dX="<<dX<<", nextIx="<<nextIx);
		if(isnan(abs(dX)) || isnan(abs(nextX)) || isnan(abs(lenDone)) || isnan(abs(lenToDo))) std::logic_error("ConveyorInlet: some parameters are NaN.");
		if(lenDone+dX>lenToDo){
			// the next sphere would not fit
			lastX=CompUtils::wrapNum(lastX-(lenToDo-lenDone),cellLen); // put lastX before the next sphere
			LOG_DEBUG("Conveyor done: next sphere "<<nextIx<<" would not fit, setting lastX="<<lastX);
			break;
		}
		lastX=CompUtils::wrapNum(nextX,cellLen);
		lenDone+=dX;

		if(!clipX.empty()){
			clipLastX=CompUtils::wrapNum(clipLastX+dX,*clipX.rbegin());
			Real zMax=linearInterpolate<Real,Real>(clipLastX,clipX,clipZ,clipPos);
			// particle over interpolated clipZ(clipX)
			// skip it and go to the next repetition of the loop
			if(centers[nextIx][2]>zMax){
				// repeat the code from the end of the loop (below) and do a new cycle
				nextIx-=1;
				continue;
			}
		}
	
		Real realSphereX=(vel/packVel)*(lenToDo-lenDone); // scale x-position by dilution (>1)
		Vector3r newPos=node->pos+node->ori*Vector3r(realSphereX,centers[nextIx][1],centers[nextIx][2]);

		vector<shared_ptr<Node>> nn;
		
		if(shapePack){
			const auto& r=shapePack->raws[nextIx];
			vector<shared_ptr<Particle>> pp;
			// use conveyor's identity which is added to the particle's orientation
			std::tie(nn,pp)=r->makeParticles(material,/*pos*/newPos,/*ori*/node->ori,/*mask*/mask,/*scale*/1.);
			for(auto& p: pp){
				dem->particles->insert(p);
				LOG_TRACE("[shapePack] new particle #"<<p->id<<", "<<p->shape->pyStr());
			}
		} else {
			if(!hasClumps()){
				auto sphere=DemFuncs::makeSphere(radii[nextIx],material);
				sphere->mask=mask;
				nn.push_back(sphere->shape->nodes[0]);
				//LOG_TRACE("x="<<x<<", "<<lenToDo<<"-("<<1+currWraps<<")*"<<cellLen<<"+"<<currX);
				dem->particles->insert(sphere);
				nn[0]->pos=newPos;
				LOG_TRACE("New sphere #"<<sphere->id<<", r="<<radii[nextIx]<<" at "<<n->pos.transpose());
			} else {
				const auto& clump=clumps[nextIx];
				vector<shared_ptr<Particle>> spheres;
				std::tie(nn,spheres)=clump->makeParticles(material,/*pos*/newPos,/*ori*/Quaternionr::Identity(),/*mask*/mask,/*scale*/1.);
				for(auto& sphere: spheres){
					dem->particles->insert(sphere);
					LOG_TRACE("[clump] new sphere #"<<sphere->id<<", r="<<radii[nextIx]<<" at "<<n->pos.transpose());
				}
			}
		}
		Real nnMass=0.;
		for(const auto& n: nn){
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
				scene->energy->add(-DemData::getEk_any(n,true,/*rotation zero, don't even compute it*/false,scene),"kinInlet",kinEnergyIx,EnergyTracker::ZeroDontCreate);
			}

			#ifdef WOO_OPENGL
				boost::mutex::scoped_lock lock(dem->nodesMutex);
			#endif
			dyn.linIx=dem->nodes.size();
			dem->nodes.push_back(n);

			stepMass+=dyn.mass;
			mass+=dyn.mass;
			nnMass+=dyn.mass;
		}

		// add mass of all nodes
		if(save) genDiamMassTime.push_back(Vector3r(2*radii[nextIx],nnMass,scene->time));

		num+=1;
	
		// decrease; can go negative, handled at the beginning of the loop
		// this same code is repeated above if clipping short-circuits the loop, keep in sync!
		nextIx-=1; 
	};

	setCurrRate(stepMass/(/*time*/lenToDo/packVel));

	dem->contacts->dirty=true; // re-initialize the collider
	#if 1
		for(const auto& e: scene->engines){
			if(dynamic_pointer_cast<InsertionSortCollider>(e)){ e->cast<InsertionSortCollider>().forceInitSort=true; break; }
		}
	#endif
}

py::object ConveyorInlet::pyDiamMass(bool zipped) const {
	return DemFuncs::seqVectorToPy(genDiamMassTime,/*itemGetter*/[](const Vector3r& i)->Vector2r{ return i.head<2>(); },/*zip*/zipped);
}

Real ConveyorInlet::pyMassOfDiam(Real min, Real max) const {
	Real ret=0.;
	for(const auto& vv: genDiamMassTime){
		if(vv[0]>=min && vv[0]<=max) ret+=vv[1];
	}
	return ret;
}


py::object ConveyorInlet::pyPsd(bool mass, bool cumulative, bool normalize, const Vector2r& dRange, const Vector2r& tRange, int num) const {
	if(!save) throw std::runtime_error("ConveyorInlet.save must be True for calling ConveyorInlet.psd()");
	auto tOk=[&tRange](const Real& t){ return isnan(tRange.minCoeff()) || (tRange[0]<=t && t<tRange[1]); };
	vector<Vector2r> psd=DemFuncs::psd(genDiamMassTime,/*cumulative*/cumulative,/*normalize*/normalize,num,dRange,
		/*radius getter*/[&tOk](const Vector3r& dmt) ->Real { return tOk(dmt[2])?dmt[0]:NaN; },
		/*weight getter*/[&](const Vector3r& dmt) -> Real{ return mass?dmt[1]:1.; }
	);
	return DemFuncs::seqVectorToPy(psd,[](const Vector2r& i)->Vector2r{ return i; },/*zip*/false);
}

