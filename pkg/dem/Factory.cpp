#include<yade/pkg/dem/Factory.hpp>
#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/Funcs.hpp>
#include<yade/pkg/dem/Clump.hpp>

YADE_PLUGIN(dem,(ParticleGenerator)(MinMaxSphereGenerator)(ParticleShooter)(AlignedMinMaxShooter)(ParticleFactory)(BoxFactory));
CREATE_LOGGER(ParticleFactory);

std::tuple<vector<shared_ptr<Particle>>,Vector3r,Vector3r>
MinMaxSphereGenerator::operator()(const shared_ptr<Material>&m){
	if(isnan(rRange[0]) || isnan(rRange[1]) || rRange[0]>rRange[1]) throw std::runtime_error("MinMaxSphereGenerator: rRange[0]>rRange[1], or they are NaN!");
	Real r=rRange[0]+Mathr::UnitRandom()*(rRange[1]-rRange[0]);
	vector<shared_ptr<Particle>> pp={DemFuncs::makeSphere(r,m)};
	return std::make_tuple(pp,Vector3r(-r,-r,-r),Vector3r(r,r,r));
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
	goalMass+=massFlowRate*scene->dt;
	vector<Vector3r> minima, maxima; // of particles created in this step

	while(totalMass<goalMass && (maxNum<0 || totalNum<maxNum) && (maxMass<0 || totalMass<maxMass)){
		shared_ptr<Material> mat;
		if(materials.size()==1) mat=materials[0];
		else{ // random choice of material with equal probability
			size_t i=max(size_t(materials.size()*Mathr::UnitRandom()),materials.size()-1);;
			mat=materials[i];
		}
		vector<shared_ptr<Particle>> pp;
		Vector3r extNeg, extPos; // extents of the generated objects
		std::tie(pp,extNeg,extPos)=(*generator)(mat);
		assert(!pp.empty());
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
			mn=extNeg+pos, mx=extPos+pos;
			vector<Particle::id_t> ids=collider->probeAabb(mn,mx);
			if(!ids.empty()){
				#ifdef YADE_DEBUG
					for(const auto& id: ids) LOG_TRACE("Collider reports intersection with #"<<id);
				#endif
				continue;
			}
			bool overlap=false;
			for(size_t i=0; i<minima.size(); i++){
				overlap=
					minima[i][0]<mx[0] && mn[0]<maxima[i][0] &&
				   minima[i][1]<mx[1] && mn[1]<maxima[i][1] &&
				   minima[i][2]<mx[2] && mn[2]<maxima[i][2];
				if(overlap){
					LOG_TRACE("Collision with "<<i<<"-th particle generated in this step.");
					break;
				}
			}
			if(!overlap) break;
		}

		// particle was generated successfully and we have place for it
		minima.push_back(mn); maxima.push_back(mx);
		totalNum+=1;
		
		Real color_=isnan(color)?Mathr::UnitRandom():color;
		if(pp.size()>1){ // clump was generated
			throw std::runtime_error("ParticleFactory: Clumps not yet tested properly.");
			vector<shared_ptr<Node>> nn;
			for(auto& p: pp){
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
			dem->nodes.push_back(clump);

			totalMass+=clump->getData<DemData>().mass;
		} else {
			auto& p=pp[0];
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
