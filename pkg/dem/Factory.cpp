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

WOO_PLUGIN(dem,(ParticleFactory)(ParticleGenerator)(MinMaxSphereGenerator)(ParticleShooter)(AlignedMinMaxShooter)(RandomFactory)(BoxFactory)(BoxFactory2d));
CREATE_LOGGER(RandomFactory);


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

py::object ParticleGenerator::pyDiamMass(bool zipped) const {
	#if 1
		if(!zipped){
			py::list diam, mass;
			for(const Vector2r& vv: genDiamMass){ diam.append(vv[0]); mass.append(vv[1]); }
			return py::object(py::make_tuple(diam,mass));
		} else {
			py::list ret;
			for(const auto& dm: genDiamMass){ ret.append(dm); }
			return ret;
		}
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
	Real r=.5*(dRange[0]+Mathr::UnitRandom()*(dRange[1]-dRange[0]));
	auto sphere=DemFuncs::makeSphere(r,mat);
	Real m=sphere->shape->nodes[0]->getData<DemData>().mass;
	if(save) genDiamMass.push_back(Vector2r(2*r,m));
	return vector<ParticleAndBox>({{sphere,AlignedBox3r(Vector3r(-r,-r,-r),Vector3r(r,r,r))}});
};

Real MinMaxSphereGenerator::critDt(Real density, Real young) {
	return DemFuncs::spherePWaveDt(dRange[0],density,young);
}




Real RandomFactory::critDt() {
	if(!generator) return Inf;
	Real ret=Inf;
	for(const auto& m: materials){
		const auto em=dynamic_pointer_cast<ElastMat>(m);
		if(!em) continue;
		ret=min(ret,generator->critDt(em->density,em->young));
	}
	return ret;
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
		Vector3r pos=Vector3r::Zero();
		vector<ParticleGenerator::ParticleAndBox> pee;
		int attempt=-1;
		while(true){
			attempt++;
			/***** too many tries, give up ******/
			if(attempt>=maxAttempts){
				generator->revokeLast(); // last particle could not be placed
				if(massFlowRate<=0){
					LOG_DEBUG("maxAttempts="<<maxAttempts<<" reached; since massFlowRate is not positive, we're done in this step");
					goto stepDone;
				}
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
			/***** each maxAttempts/attPerPar, try a new particles *****/	
			if((attempt%(maxAttempts/attemptPar))==0){
				LOG_DEBUG("attempt "<<attempt<<": trying with a new particle.");
				if(attempt>0) generator->revokeLast(); // if not at the beginning, revoke the last particle

				// random choice of material with equal probability
				if(materials.size()==1) mat=materials[0];
				else{ 
					size_t i=max(size_t(materials.size()*Mathr::UnitRandom()),materials.size()-1);;
					mat=materials[i];
				}
				// generate a new particle
				pee=(*generator)(mat);
				assert(!pee.empty());
				LOG_TRACE("Placing "<<pee.size()<<"-sized particle; first component is a "<<pee[0].par->getClassName()<<", extents from "<<pee[0].extents.min().transpose()<<" to "<<pee[0].extents.max().transpose());
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
					Vector3r subPos=peSphere->nodes[0]->pos;
					for(const auto& s: spheres.pack){
						// check dist && don't collide with another sphere from this clump
						// (abuses the *num* counter for clumpId)
						if((s.c-(pos+subPos)).squaredNorm()<pow(s.r+r,2)){
							LOG_TRACE("Collision with a particle in SpherePack (a particle generated in this step).");
							goto tryAgain;
						}
					}
					// don't add to spheres until all particles will have been checked for overlaps (below)
				#else
					// see intersection with existing particles
					bool overlap=false;
					vector<Particle::id_t> ids=collider->probeAabb(peBox.min(),peBox.max());
					for(const auto& id: ids){
						LOG_TRACE("Collider reports intersection with #"<<id);
						if(id>(Particle::id_t)dem->particles->size() || !(*dem->particles)[id]) continue;
						const shared_ptr<Shape>& sh2((*dem->particles)[id]->shape);
						// no spheres, or they are too close
						if(!peSphere || !sh2->isA<woo::Sphere>() || 1.1*(pos-sh2->nodes[0]->pos).squaredNorm()<pow(peSphere->radius+sh2->cast<Sphere>().radius,2)) goto tryAgain;
					}

					// intersection with particles generated by ourselves in this step
					for(size_t i=0; i<genBoxes.size(); i++){
						overlap=(genBoxes[i].squaredExteriorDistance(peBox)==0.);
						if(overlap){
							const auto& genSh(generated[i]->shape);
							// for spheres, try to compute whether they really touch
							if(!peSphere || !genSh->isA<Sphere>() || (pos-genSh->nodes[0]->pos).squaredNorm()<pow(peSphere->radius+genSh->cast<Sphere>().radius,2)){
								LOG_TRACE("Collision with "<<i<<"-th particle generated in this step.");
								goto tryAgain;
							}
						}
					}
				#endif
			}
			LOG_DEBUG("No collision (attempt "<<attempt<<"), particle will be created :-) ");
			#ifdef WOO_FACTORY_SPHERES_ONLY
				// num will be the same for all spheres within this clump (abuse the *num* counter)
				for(const auto& pe: pee){
					Vector3r subPos=pe.par->shape->nodes[0]->pos;
					Real r=pe.par->shape->cast<Sphere>().radius;
					spheres.pack.push_back(SpherePack::Sph((pos+subPos),r,/*clumpId*/(pee.size()==1?-1:num)));
				}
			#endif
			break;
			tryAgain: ; // try to position the same particle again
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
			//LOG_WARN("Clumps not yet tested properly.");
			vector<shared_ptr<Node>> nn;
			for(auto& pe: pee){
				auto& p=pe.par;
				p->mask=mask;
				#ifdef WOO_OPENGL
					assert(p->shape);
					p->shape->color=color_;
				#endif
				if(p->shape->nodes.size()!=1) LOG_WARN("Adding suspicious clump containing particle with more than one node (please check, this is perhaps not tested");
				for(const auto& n: p->shape->nodes){
					nn.push_back(n);
					n->pos+=pos;
				}
				dem->particles->insert(p);
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

