#include<yade/pkg/dem/Factory.hpp>
#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/Funcs.hpp>
#include<yade/pkg/dem/Clump.hpp>

YADE_PLUGIN(dem,(ParticleGenerator)(MinMaxSphereGenerator)(PsdSphereGenerator)(ParticleShooter)(AlignedMinMaxShooter)(ParticleFactory)(BoxFactory));
CREATE_LOGGER(PsdSphereGenerator);
CREATE_LOGGER(ParticleFactory);

vector<ParticleGenerator::ParticleExtExt>
MinMaxSphereGenerator::operator()(const shared_ptr<Material>&m){
	if(isnan(rRange[0]) || isnan(rRange[1]) || rRange[0]>rRange[1]) throw std::runtime_error("MinMaxSphereGenerator: rRange[0]>rRange[1], or they are NaN!");
	Real r=rRange[0]+Mathr::UnitRandom()*(rRange[1]-rRange[0]);
	return vector<ParticleExtExt>({{DemFuncs::makeSphere(r,m),Vector3r(-r,-r,-r),Vector3r(r,r,r)}});
};


void PsdSphereGenerator::postLoad(PsdSphereGenerator&){
	if(psd.empty()) return;
	for(int i=0; i<(int)psd.size()-1; i++){
		if(psd[i][0]>=psd[i+1][0]) throw std::runtime_error("PsdSphereGenerator.psd: diameters (the x-component) must be strictly increasing ("+to_string(psd[i][0])+">="+to_string(psd[i+1][0])+")");
		if(psd[i][1]>psd[i+1][1]) throw std::runtime_error("PsdSphereGenerator.psd: passing values (the y-component) must be increasing ("+to_string(psd[i][1])+">"+to_string(psd[i+1][1])+")");
	}
	Real maxPass=(*psd.rbegin())[1];
	if(maxPass!=1.0){
		LOG_INFO("Normalizing PSD so that highest value of passing is 1.0 rather than "<<maxPass);
		for(Vector2r& v: psd) v[1]/=maxPass;
	}
	numPerBin.resize(psd.size());
	std::fill(numPerBin.begin(),numPerBin.end(),0);
	numTot=0;
}

vector<ParticleGenerator::ParticleExtExt>
PsdSphereGenerator::operator()(const shared_ptr<Material>&m){
	if(psd.empty()) throw std::runtime_error("PsdSphereGenerator.psd is empty.");
	Real maxBinDiff=-Inf; int maxBin=-1;
	// find the bin which is the most below the expected percentage according to the PSD
	if(numTot<=0) maxBin=0;
	else{
		for(size_t i=0;i<psd.size();i++){
			Real binDiff=psd[i][1]-numPerBin[i]/numTot;
			if(binDiff>maxBinDiff){ maxBinDiff=binDiff; maxBin=i; }
		}
	}
	assert(maxBin>=0);
	numPerBin[maxBin]++;
	numTot++;
	Real r=psd[maxBin][0]/2.;
	return vector<ParticleExtExt>({{DemFuncs::makeSphere(r,m),Vector3r(-r,-r,-r),Vector3r(r,r,r)}});
};



void ParticleFactory::run(){
	DemField* dem=dynamic_cast<DemField*>(field.get());
	if(!generator) throw std::runtime_error("ParticleFactor.generator==None!");
	if(!shooter) throw std::runtime_error("ParticleFactor.shooter==None!");
	if(materials.empty()) throw std::runtime_error("ParticleFactory.materials is empty!");
	if(!collider){
		for(const auto& e: scene->engines){ collider=dynamic_pointer_cast<Collider>(e); if(collider) break; }
		if(!collider) throw std::runtime_error("ParticleFactory: no Collider found within engines (needed for collisions detection)");
	}

	// to be attained in this step;
	goalMass+=massFlowRate*scene->dt*(scene->step-this->stepPrev); // stepLast==-1 if never run, which is OK
	vector<Vector3r> minima, maxima; // of particles created in this step

	while(totalMass<goalMass && (maxNum<0 || totalNum<maxNum) && (maxMass<0 || totalMass<maxMass)){
		shared_ptr<Material> mat;
		if(materials.size()==1) mat=materials[0];
		else{ // random choice of material with equal probability
			size_t i=max(size_t(materials.size()*Mathr::UnitRandom()),materials.size()-1);;
			mat=materials[i];
		}
		vector<ParticleGenerator::ParticleExtExt> pee=(*generator)(mat);
		assert(!pee.empty());
		Vector3r pos=Vector3r::Zero(), mn, mx;
		int attempt=-1;
		while(true){
			attempt++;
			if(attempt>=abs(maxAttempts)){
				if(maxAttempts<0){
					LOG_INFO("maxAttempts="<<maxAttempts<<" reached, making myself dead.");
					this->dead=true;
					return;
				}
					else throw std::runtime_error("ParticleFactory.maxAttempts reached ("+lexical_cast<string>(maxAttempts)+")");
			}	
			pos=randomPosition(); // overridden in child classes
			LOG_TRACE("Trying pos="<<pos.transpose());
			for(const auto& pe: pee){
				bool overlap=false;
				mn=pos+pe.extMin, mx=pos+pe.extMax;
				vector<Particle::id_t> ids=collider->probeAabb(mn,mx);
				if(!ids.empty()){
					#ifdef YADE_DEBUG
						for(const auto& id: ids) LOG_TRACE("Collider reports intersection with #"<<id);
					#endif
					goto tryAgain;
				}
				for(size_t i=0; i<minima.size(); i++){
					overlap=
						minima[i][0]<mx[0] && mn[0]<maxima[i][0] &&
						minima[i][1]<mx[1] && mn[1]<maxima[i][1] &&
						minima[i][2]<mx[2] && mn[2]<maxima[i][2];
					if(overlap){
						LOG_TRACE("Collision with "<<i<<"-th particle generated in this step.");
						goto tryAgain;
					}
				}
			}
			LOG_TRACE("No collision, particle will be created :-) ");
			break;
			tryAgain: ; // reiterate
		}

		// particle was generated successfully and we have place for it
		for(const auto& pe: pee){
			minima.push_back(pe.extMin+pos); maxima.push_back(pe.extMax+pos);
		}

		totalNum+=1;
		
		Real color_=isnan(color)?Mathr::UnitRandom():color;
		if(pee.size()>1){ // clump was generated
			throw std::runtime_error("ParticleFactory: Clumps not yet tested properly.");
			vector<shared_ptr<Node>> nn;
			for(auto& pe: pee){
				auto& p=pe.par;
				p->mask=mask;
				#ifdef YADE_OPENGL
					assert(p->shape);
					p->shape->color=color_;
				#endif
				dem->particles.insert(p);
				for(const auto& n: p->shape->nodes){
					nn.push_back(n);
					n->pos+=pos;
				}
			}
			shared_ptr<Node> clump=ClumpData::makeClump(nn,/*intersection*/false);
			// TODO: track energy of the shooter
			auto& dyn=clump->getData<DemData>();
			(*shooter)(dyn.vel,dyn.angVel);
			// TODO: compute initial angular momentum, since wi will (very likely) use the aspherical integrator
			ClumpData::applyToMembers(clump,/*reset*/false); // apply velocity
			dem->clumps.push_back(clump);
			#ifdef YADE_OPENGL
				boost::mutex::scoped_lock lock(dem->nodesMutex);
			#endif
			dem->nodes.push_back(clump);

			totalMass+=clump->getData<DemData>().mass;
		} else {
			auto& p=pee[0].par;
			p->mask=mask;
			assert(p->shape);
			#ifdef YADE_OPENGL
				p->shape->color=color_;
			#endif
			assert(p->shape->nodes.size()==1); // if this fails, enable the block below
			assert(p->shape->nodes[0]->pos==Vector3r::Zero());
			p->shape->nodes[0]->pos+=pos;
			auto& dyn=p->shape->nodes[0]->getData<DemData>();
			(*shooter)(dyn.vel,dyn.angVel);
			totalMass+=dyn.mass;
			assert(p->shape->nodes[0]->hasData<DemData>());
			dem->particles.insert(p);
			#ifdef YADE_OPENGL
				boost::mutex::scoped_lock lock(dem->nodesMutex);
			#endif
			dem->nodes.push_back(p->shape->nodes[0]);
			// handle multi-nodal particle (unused now)
			#if 0
				// TODO: track energy of the shooter
				Vector3r vel,angVel;
				shooter(dyn.vel,dyn.angVel);
				// in case the particle is multinodal, apply the same to all nodes
				for(const auto& n: p.shape->nodes){
					auto& dyn=n->getData<DemData>();
					dyn.vel=vel; dyn.angVel=angVel;
					totalMass+=dyn.mass;

					dem->nodes.push_back(n);
				}
			#endif
		}
	};
}
