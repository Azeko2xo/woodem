#include<woo/pkg/dem/Factory.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/Funcs.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Sphere.hpp>

#include<woo/lib/sphere-pack/SpherePack.hpp>

// hack
#include<woo/pkg/dem/InsertionSortCollider.hpp>

#include<boost/range/algorithm/lower_bound.hpp>

#if 0
	#include<woo/extra/numpy_boost.hpp>
#endif

#include<boost/tuple/tuple_comparison.hpp>

WOO_PLUGIN(dem,(ParticleFactory)(ParticleGenerator)(MinMaxSphereGenerator)(PsdSphereGenerator)(ParticleShooter)(AlignedMinMaxShooter)(RandomFactory)(BoxFactory)(BoxDeleter)(ConveyorFactory));
CREATE_LOGGER(PsdSphereGenerator);
CREATE_LOGGER(RandomFactory);
CREATE_LOGGER(ConveyorFactory);
CREATE_LOGGER(BoxDeleter);


py::tuple ParticleGenerator::pyPsd(bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) const {
	if(!save) throw std::runtime_error("ParticleGenerator.save must be True for calling ParticleGenerator.psd()");
	vector<Vector2r> psd=DemFuncs::psd(genDiamMass,/*cumulative*/cumulative,/*normalize*/normalize,num,dRange,
		/*radius getter*/[](const Vector2r& diamMass) ->Real { return diamMass[0]; },
		/*weight getter*/[&](const Vector2r& diamMass) -> Real{ return mass?diamMass[1]:1.; }
	);
	py::list diameters,percentage;
	for(const auto& dp: psd){ diameters.append(dp[0]); percentage.append(dp[1]); }
	return py::make_tuple(diameters,percentage);
}

py::object ParticleGenerator::pyDiamMass() const {
	#if 1
		py::list diam, mass;
		for(const Vector2r& vv: genDiamMass){ diam.append(vv[0]); mass.append(vv[1]); }
		return py::object(py::make_tuple(diam,mass));
	#else
		boost::multi_array<double,2> ret(boost::extents[genDiamMass.size()][2]);
		for(size_t i=0;i<genDiamMass.size();i++){ ret[i][0]=genDiamMass[i][0]; ret[i][1]=genDiamMass[i][1]; }
		//PyObject* result=numpy_from_boost_array(ret).py_ptr();
		PyObject* o=numpy_boost<double,2>(numpy_from_boost_array(ret)).py_ptr();
		//return py::incref(o);
		return py::object(py::handle<>(py::borrowed(o)));
	#endif
}

Real ParticleGenerator::pyMassOfDiam(Real min, Real max) const{
	Real ret=0.;
	for(const Vector2r& vv: genDiamMass){
		if(vv[0]>=min && vv[0]<=max) ret+=vv[1];
	}
	return ret;
};

vector<ParticleGenerator::ParticleAndBox>
MinMaxSphereGenerator::operator()(const shared_ptr<Material>&mat){
	if(isnan(dRange[0]) || isnan(dRange[1]) || dRange[0]>dRange[1]) throw std::runtime_error("MinMaxSphereGenerator: dRange[0]>dRange[1], or they are NaN!");
	Real r=dRange[0]+Mathr::UnitRandom()*(dRange[1]-dRange[0]);
	auto sphere=DemFuncs::makeSphere(r,mat);
	Real m=sphere->shape->nodes[0]->getData<DemData>().mass;
	if(save) genDiamMass.push_back(Vector2r(2*r,m));
	return vector<ParticleAndBox>({{sphere,AlignedBox3r(Vector3r(-r,-r,-r),Vector3r(r,r,r))}});
};


void PsdSphereGenerator::postLoad(PsdSphereGenerator&,void*){
	if(psdPts.empty()) return;
	for(int i=0; i<(int)psdPts.size()-1; i++){
		if(psdPts[i][0]<0 || psdPts[i][1]<0) throw std::runtime_error("PsdSphereGenerator.psdPts: negative values not allowed.");
		if(psdPts[i][0]>psdPts[i+1][0]) throw std::runtime_error("PsdSphereGenerator.psdPts: diameters (the x-component) must be increasing ("+to_string(psdPts[i][0])+">="+to_string(psdPts[i+1][0])+")");
		if(psdPts[i][1]>psdPts[i+1][1]) throw std::runtime_error("PsdSphereGenerator.psdPts: passing values (the y-component) must be increasing ("+to_string(psdPts[i][1])+">"+to_string(psdPts[i+1][1])+")");
	}
	Real maxPass=psdPts.back()[1];
	if(maxPass!=1.0){
		LOG_INFO("Normalizing psdPts so that highest value of passing is 1.0 rather than "<<maxPass);
		for(Vector2r& v: psdPts) v[1]/=maxPass;
	}
	weightPerBin.resize(psdPts.size());
	std::fill(weightPerBin.begin(),weightPerBin.end(),0);
	weightTotal=0;
}

vector<ParticleGenerator::ParticleAndBox>
PsdSphereGenerator::operator()(const shared_ptr<Material>&mat){
	if(psdPts.empty()) throw std::runtime_error("PsdSphereGenerator.psdPts is empty.");
	if(mass && !(mat->density>0)) throw std::runtime_error("PsdSphereGenerator: material density must be positive (not "+to_string(mat->density));
	Real r,m;
	shared_ptr<Particle> sphere;
	Real maxBinDiff=-Inf; int maxBin=-1;
	// find the bin which is the most below the expected percentage according to the psdPts
	// we use mass or number automatically: weightTotal and weightPerBin accumulates number or mass
	if(weightTotal<=0) maxBin=0;
	else{
		for(size_t i=0;i<psdPts.size();i++){
			Real binDesired=psdPts[i][1]-(i>0?psdPts[i-1][1]:0.);
			Real binDiff=binDesired-weightPerBin[i]*1./weightTotal;
			LOG_TRACE("bin "<<i<<" (d="<<psdPts[i][0]<<"): should be "<<binDesired<<", current "<<weightPerBin[i]*1./weightTotal<<", diff="<<binDiff);
			if(binDiff>maxBinDiff){ maxBinDiff=binDiff; maxBin=i; }
		}
	}
	LOG_TRACE("** maxBin="<<maxBin<<", d="<<psdPts[maxBin][0]);
	assert(maxBin>=0);
	if(discrete){
		// just pick the radius found
		r=psdPts[maxBin][0]/2.; 
	} else {
		// here we are between the maxBin and the previous one (interpolate between them, add to maxBin anyways)
		if(maxBin==0){ r=psdPts[0][0]/2.; }
		else{
			Real a=psdPts[maxBin-1][0]/2., b=psdPts[maxBin][0]/2.;
			Real rnd=Mathr::UnitRandom();
			// size grows linearly with size, mass with 3rd power; therefore 2nd-order power-law for mass
			// interpolate on inverse-power-law
			// r=(a^-2+rnd*(b^-2-a^-2))^(-1/2.)
			if(mass) r=sqrt(1/(1/(a*a)+rnd*(1/(b*b)-1/(a*a))));
			// interpolate linearly
			else r=a+rnd*(b-a);
			LOG_TRACE((mass?"Power-series":"Linear")<<" interpolation "<<a<<".."<<b<<" @ "<<rnd<<" = "<<r)
		}
	}

	sphere=DemFuncs::makeSphere(r,mat);
	m=sphere->shape->nodes[0]->getData<DemData>().mass;
	weightPerBin[maxBin]+=(mass?m:1.);
	weightTotal+=(mass?m:1.);
	assert(r>=.5*psdPts[0][0]);
	if(save) genDiamMass.push_back(Vector2r(2*r,m));
	return vector<ParticleAndBox>({{sphere,AlignedBox3r(Vector3r(-r,-r,-r),Vector3r(r,r,r))}});
};

py::tuple PsdSphereGenerator::pyInputPsd(bool scale, bool cumulative, int num) const {
	Real factor=1.; // no scaling at all
	if(scale){
		if(mass) for(const auto& vv: genDiamMass) factor+=vv[1]; // scale by total mass of all generated particles
		else factor=genDiamMass.size(); //  scale by number of particles
	}
	py::list dia, frac; // diameter and fraction axes
	if(cumulative){
		if(psdPts[0][1]>0.){ dia.append(psdPts[0][0]); frac.append(0); }
		for(size_t i=0;i<psdPts.size();i++){
			dia.append(psdPts[i][0]); frac.append(psdPts[i][1]*factor);
			if(discrete && i<psdPts.size()-1){ dia.append(psdPts[i+1][0]); frac.append(psdPts[i][1]*factor); }
		}
	} else {
		if(discrete){
			// points shown as bars with relative width given in *num*
			Real wd=(psdPts.back()[0]-psdPts.front()[0])/num;
			for(size_t i=0;i<psdPts.size();i++){
				Vector2r offset=(i==0?Vector2r(0,wd):(i==psdPts.size()-1?Vector2r(-wd,0):Vector2r(-wd/2.,wd/2.)));
				Vector2r xx=psdPts[i][0]*Vector2r::Ones()+offset;
				Real y=factor*(psdPts[i][1]-(i==0?0.:psdPts[i-1][1]));
				dia.append(xx[0]); frac.append(0.);
				dia.append(xx[0]); frac.append(y);
				dia.append(xx[1]); frac.append(y);
				dia.append(xx[1]); frac.append(0.);
			}
		}
		else{
			dia.append(psdPts[0][0]); frac.append(0);
			Real xSpan=(psdPts.back()[0]-psdPts[0][0]);
			for(size_t i=0;i<psdPts.size()-1;i++){
				Real dx=(psdPts[i+1][0]-psdPts[i][0]);
				Real y=factor*(psdPts[i+1][1]-psdPts[i][1])*1./(num*dx/xSpan);
				dia.append(psdPts[i][0]); frac.append(y);
				dia.append(psdPts[i+1][0]); frac.append(y);
			}
			dia.append(psdPts.back()[0]); frac.append(0.);
		}
	}
	return py::make_tuple(dia,frac);
}


void RandomFactory::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	if(!generator) throw std::runtime_error("RandomFactory.generator==None!");
	if(materials.empty()) throw std::runtime_error("RandomFactory.materials is empty!");
	if(!collider){
		for(const auto& e: scene->engines){ collider=dynamic_pointer_cast<Collider>(e); if(collider) break; }
		if(!collider) throw std::runtime_error("RandomFactory: no Collider found within engines (needed for collisions detection)");
	}
	if(dynamic_pointer_cast<InsertionSortCollider>(collider)) static_pointer_cast<InsertionSortCollider>(collider)->forceInitSort=true;
	if(isnan(massFlowRate)) throw std::runtime_error("RandomFactory.massFlowRate must be given (is "+to_string(massFlowRate)+"); if you want to generate as many particles as possible, say massFlowRate=0.");
	if(massFlowRate<=0 && maxAttempts==0) throw std::runtime_error("RandomFactory.massFlowRate<=0 (no massFlowRate prescribed), but RandomFactory.maxAttempts==0. (unlimited number of attempts); this would cause infinite loop.");
	if(maxAttempts<0){
		std::runtime_error("RandomFactory.maxAttempts must be non-negative. Negative value, leading to meaking engine dead, is achieved by setting atMaxAttempts=RandomFactory.maxAttDead now.");
	}

	// as if some time has already elapsed at the very start
	// otherwise mass flow rate is too big since one particle during Δt exceeds it easily
	// equivalent to not running the first time, but without time waste
	if(stepPrev==-1 && stepPeriod>0) stepPrev=-stepPeriod; 
	long nSteps=scene->step-stepPrev;
	// to be attained in this step;
	stepGoalMass+=massFlowRate*scene->dt*nSteps; // stepLast==-1 if never run, which is OK
	vector<AlignedBox3r> genBoxes; // of particles created in this step
	vector<shared_ptr<Particle>> generated;
	Real stepMass=0.;

	#ifdef WOO_FACTORY_SPHERES_ONLY
		SpherePack spheres;
		spheres.pack.reserve(dem->particles->size()*1.2); // something extra for generated particles
		// HACK!!!
		bool isBox=dynamic_cast<BoxFactory*>(this);
		AlignedBox3r box;
		if(isBox){ box=this->cast<BoxFactory>().box; }
		for(const auto& p: *dem->particles){
			if(dynamic_pointer_cast<Sphere>(p->shape) && (!isBox || box.contains(p->shape->nodes[0]->pos))) spheres.pack.push_back(SpherePack::Sph(p->shape->nodes[0]->pos,p->shape->cast<Sphere>().radius));
		}
	#endif

	while(true){
		// finished forever
		if((maxMass>0 && mass>=maxMass) || (maxNum>0 && num>=maxNum)){
			LOG_INFO("mass or number reached, making myself dead.");
			dead=true;
			if(zeroRateAtStop) currRate=0.;
			return;
		}
		// finished in this step
		if(massFlowRate>0 && mass>=stepGoalMass) break;

		shared_ptr<Material> mat;
		if(materials.size()==1) mat=materials[0];
		else{ // random choice of material with equal probability
			size_t i=max(size_t(materials.size()*Mathr::UnitRandom()),materials.size()-1);;
			mat=materials[i];
		}
		vector<ParticleGenerator::ParticleAndBox> pee=(*generator)(mat);
		assert(!pee.empty());
		LOG_TRACE("Placing "<<pee.size()<<"-sized particle; first component is a "<<pee[0].par->getClassName()<<", extents from "<<pee[0].extents.min().transpose()<<" to "<<pee[0].extents.max().transpose());
		Vector3r pos=Vector3r::Zero();
		int attempt=-1;
		while(true){
			attempt++;
			if(attempt>=maxAttempts){
				if(massFlowRate<=0) goto stepDone;
				switch(atMaxAttempts){
					case MAXATT_ERROR: throw std::runtime_error("RandomFactory.maxAttempts reached ("+lexical_cast<string>(maxAttempts)+")"); break;
					case MAXATT_DEAD:{
						LOG_INFO("maxAttempts="<<maxAttempts<<" reached, making myself dead.");
						this->dead=true;
						return;
					}
					case MAXATT_WARN: LOG_WARN("maxAttempts "<<maxAttempts<<" reached before required mass amount was generated; continuing, since maxAttemptsError==False"); break;
					case MAXATT_SILENT: break;
					default: woo::ValueError("Invalid value of RandomFactory.atMaxAttempts="+to_string(atMaxAttempts)+".");
				}
			}	
			pos=randomPosition(); // overridden in child classes
			LOG_TRACE("Trying pos="<<pos.transpose());
			for(const auto& pe: pee){
				// make translated copy
				AlignedBox3r peBox(pe.extents); peBox.translate(pos); 
				// box is not entirely within the factory shape
				if(!validateBox(peBox)){ LOG_TRACE("validateBox failed."); goto tryAgain; }

				const shared_ptr<woo::Sphere>& peSphere=dynamic_pointer_cast<woo::Sphere>(pe.par->shape);

				#ifdef WOO_FACTORY_SPHERES_ONLY
					if(!peSphere) throw std::runtime_error(__FILE__ ": Only Spheres are supported in this build!");
					Real r=peSphere->radius;
					for(const auto& s: spheres.pack){
						if((s.c-pos).squaredNorm()<pow(s.r+r,2)){
							LOG_TRACE("Collision with a particle in SpherePack.");
							goto tryAgain;
						}
					}
					spheres.pack.push_back(SpherePack::Sph(pos,r));
				#else
					// see intersection with existing particles
					bool overlap=false;
					vector<Particle::id_t> ids=collider->probeAabb(peBox.min(),peBox.max());
					for(const auto& id: ids){
						LOG_TRACE("Collider reports intersection with #"<<id);
						if(id>(Particle::id_t)dem->particles->size() || !(*dem->particles)[id]) continue;
						const shared_ptr<Shape>& sh2((*dem->particles)[id]->shape);
						// no spheres, or they are too close
						if(!peSphere || !dynamic_pointer_cast<woo::Sphere>(sh2) || 1.1*(pos-sh2->nodes[0]->pos).squaredNorm()<pow(peSphere->radius+sh2->cast<Sphere>().radius,2)) goto tryAgain;
					}

					// intersection with particles generated by ourselves in this step
					for(size_t i=0; i<genBoxes.size(); i++){
						overlap=(genBoxes[i].squaredExteriorDistance(peBox)==0.);
						if(overlap){
							const auto& genSh(generated[i]->shape);
							// for spheres, try to compute whether they really touch
							if(!peSphere || !dynamic_pointer_cast<Sphere>(genSh) || (pos-genSh->nodes[0]->pos).squaredNorm()<pow(peSphere->radius+genSh->cast<Sphere>().radius,2)){
								LOG_TRACE("Collision with "<<i<<"-th particle generated in this step.");
								goto tryAgain;
							}
						}
					}
				#endif
			}
			LOG_TRACE("No collision, particle will be created :-) ");
			break;
			tryAgain: ; // reiterate
		}

		// particle was generated successfully and we have place for it
		for(const auto& pe: pee){
			genBoxes.push_back(AlignedBox3r(pe.extents).translate(pos));
			generated.push_back(pe.par);
		}

		num+=1;

		#ifdef WOO_OPENGL			
			Real color_=isnan(color)?Mathr::UnitRandom():color;
		#endif
		if(pee.size()>1){ // clump was generated
			throw std::runtime_error("RandomFactory: Clumps not yet tested properly.");
			vector<shared_ptr<Node>> nn;
			for(auto& pe: pee){
				auto& p=pe.par;
				p->mask=mask;
				#ifdef WOO_OPENGL
					assert(p->shape);
					p->shape->color=color_;
				#endif
				dem->particles->insert(p);
				for(const auto& n: p->shape->nodes){
					nn.push_back(n);
					n->pos+=pos;
				}
			}
			shared_ptr<Node> clump=ClumpData::makeClump(nn,/*no central node pre-given*/shared_ptr<Node>(),/*intersection*/false);
			auto& dyn=clump->getData<DemData>();
			if(shooter) (*shooter)(dyn.vel,dyn.angVel);
			if(scene->trackEnergy) scene->energy->add(-DemData::getEk_any(clump,true,true,scene),"kinFactory",kinEnergyIx,EnergyTracker::ZeroDontCreate);
			if(dyn.angVel!=Vector3r::Zero()){
				throw std::runtime_error("pkg/dem/RandomFactory.cpp: generated particle has non-zero angular velocity; angular momentum should be computed so that rotation integration is correct, but it was not yet implemented.");
				// TODO: compute initial angular momentum, since we will (very likely) use the aspherical integrator
			}
			ClumpData::applyToMembers(clump,/*reset*/false); // apply velocity
			dem->clumps.push_back(clump);
			#ifdef WOO_OPENGL
				boost::mutex::scoped_lock lock(dem->nodesMutex);
			#endif
			dyn.linIx=dem->nodes.size();
			dem->nodes.push_back(clump);

			mass+=clump->getData<DemData>().mass;
			stepMass+=clump->getData<DemData>().mass;
		} else {
			auto& p=pee[0].par;
			p->mask=mask;
			assert(p->shape);
			#ifdef WOO_OPENGL
				p->shape->color=color_;
			#endif
			assert(p->shape->nodes.size()==1); // if this fails, enable the block below
			const auto& node0=p->shape->nodes[0];
			assert(node0->pos==Vector3r::Zero());
			node0->pos+=pos;
			auto& dyn=node0->getData<DemData>();
			if(shooter) (*shooter)(dyn.vel,dyn.angVel);
			if(scene->trackEnergy) scene->energy->add(-DemData::getEk_any(node0,true,true,scene),"kinFactory",kinEnergyIx,EnergyTracker::ZeroDontCreate);
			mass+=dyn.mass;
			stepMass+=dyn.mass;
			assert(node0->hasData<DemData>());
			dem->particles->insert(p);
			#ifdef WOO_OPENGL
				boost::mutex::scoped_lock lock(dem->nodesMutex);
			#endif
			dyn.linIx=dem->nodes.size();
			dem->nodes.push_back(node0);
			// handle multi-nodal particle (unused now)
			#if 0
				// TODO: track energy of the shooter
				Vector3r vel,angVel;
				if(shooter) shooter(dyn.vel,dyn.angVel);
				// in case the particle is multinodal, apply the same to all nodes
				for(const auto& n: p.shape->nodes){
					auto& dyn=n->getData<DemData>();
					dyn.vel=vel; dyn.angVel=angVel;
					mass+=dyn.mass;

					n->linIx=dem->nodes.size();
					dem->nodes.push_back(n);
				}
			#endif
		}
	};

	stepDone:
	setCurrRate(stepMass/(nSteps*scene->dt));
}


#ifdef BOX_FACTORY_PERI
bool BoxFactory::validatePeriodicBox(const AlignedBox3r& b) const {
	if(periSpanMask==0) return box.contains(b);
	// otherwise just enlarge our box in all directions
	AlignedBox3r box2(box);
	for(int i:{0,1,2}){
		if(periSpanMask&(1<<i)) continue;
		Real extra=b.sizes()[i]/2.;
		box2.min()[i]-=extra; box2.max()[i]+=extra;
	}
	return box2.contains(b);
}
#endif

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
