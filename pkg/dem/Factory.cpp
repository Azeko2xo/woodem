#include<yade/pkg/dem/Factory.hpp>
#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/Funcs.hpp>
#include<yade/pkg/dem/Clump.hpp>
#include<yade/pkg/dem/Sphere.hpp>

// hack
#include<yade/pkg/dem/InsertionSortCollider.hpp>

#include<boost/range/algorithm/lower_bound.hpp>

#include<yade/extra/numpy_boost.hpp>

YADE_PLUGIN(dem,(ParticleGenerator)(MinMaxSphereGenerator)(PsdSphereGenerator)(ParticleShooter)(AlignedMinMaxShooter)(ParticleFactory)(BoxFactory)(BoxDeleter));
CREATE_LOGGER(PsdSphereGenerator);
CREATE_LOGGER(ParticleFactory);
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

py::object ParticleGenerator::pyDiamMass(){
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

vector<ParticleGenerator::ParticleAndBox>
MinMaxSphereGenerator::operator()(const shared_ptr<Material>&mat){
	if(isnan(dRange[0]) || isnan(dRange[1]) || dRange[0]>dRange[1]) throw std::runtime_error("MinMaxSphereGenerator: dRange[0]>dRange[1], or they are NaN!");
	Real r=dRange[0]+Mathr::UnitRandom()*(dRange[1]-dRange[0]);
	auto sphere=DemFuncs::makeSphere(r,mat);
	Real m=sphere->shape->nodes[0]->getData<DemData>().mass;
	if(save) genDiamMass.push_back(Vector2r(2*r,m));
	return vector<ParticleAndBox>({{sphere,AlignedBox3r(Vector3r(-r,-r,-r),Vector3r(r,r,r))}});
};


void PsdSphereGenerator::postLoad(PsdSphereGenerator&){
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
	assert(maxBin>=0);
	LOG_TRACE("** maxBin="<<maxBin<<", d="<<psdPts[maxBin][0]);
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


void ParticleFactory::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	if(!generator) throw std::runtime_error("ParticleFactor.generator==None!");
	if(!shooter) throw std::runtime_error("ParticleFactor.shooter==None!");
	if(materials.empty()) throw std::runtime_error("ParticleFactory.materials is empty!");
	if(!collider){
		for(const auto& e: scene->engines){ collider=dynamic_pointer_cast<Collider>(e); if(collider) break; }
		if(!collider) throw std::runtime_error("ParticleFactory: no Collider found within engines (needed for collisions detection)");
	}
	if(dynamic_pointer_cast<InsertionSortCollider>(collider)) static_pointer_cast<InsertionSortCollider>(collider)->forceInitSort=true;

	// as if some time has already elapsed at the very start
	// otherwise mass flow rate is too big since one particle during Δt exceeds it easily
	// equivalent to not running the first time, but without time waste
	if(stepPrev==-1 && stepPeriod>0) stepPrev=-stepPeriod; 
	long nSteps=scene->step-this->stepPrev;
	this->stepPrev=scene->step;
	// to be attained in this step;
	stepGoalMass+=massFlowRate*scene->dt*nSteps; // stepLast==-1 if never run, which is OK
	vector<AlignedBox3r> genBoxes; // of particles created in this step
	vector<shared_ptr<Particle>> generated;
	Real stepMass=0.;
	

	while(mass<stepGoalMass && (maxNum<0 || num<maxNum) && (maxMass<0 || mass<maxMass)){
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
				// make translated copy
				AlignedBox3r peBox(pe.extents); peBox.translate(pos); 
				// box is not entirely within the factory shape
				if(!validateBox(peBox)){ LOG_TRACE("validateBox failed."); goto tryAgain; }

				const shared_ptr<yade::Sphere>& peSphere=dynamic_pointer_cast<yade::Sphere>(pe.par->shape);

				// see intersection with existing particles
				vector<Particle::id_t> ids=collider->probeAabb(peBox.min(),peBox.max());
				for(const auto& id: ids){
					LOG_TRACE("Collider reports intersection with #"<<id);
					if(id>(Particle::id_t)dem->particles->size() || !(*dem->particles)[id]) continue;
					const shared_ptr<Shape>& sh2((*dem->particles)[id]->shape);
					// no spheres, or they are too close
					if(!peSphere || !dynamic_pointer_cast<yade::Sphere>(sh2) || 1.1*(pos-sh2->nodes[0]->pos).squaredNorm()<pow(peSphere->radius+sh2->cast<Sphere>().radius,2)) goto tryAgain;
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
				dem->particles->insert(p);
				for(const auto& n: p->shape->nodes){
					nn.push_back(n);
					n->pos+=pos;
				}
			}
			shared_ptr<Node> clump=ClumpData::makeClump(nn,/*no central node pre-given*/shared_ptr<Node>(),/*intersection*/false);
			// TODO: track energy of the shooter
			auto& dyn=clump->getData<DemData>();
			(*shooter)(dyn.vel,dyn.angVel);
			// TODO: compute initial angular momentum, since wi will (very likely) use the aspherical integrator
			ClumpData::applyToMembers(clump,/*reset*/false); // apply velocity
			dem->clumps.push_back(clump);
			#ifdef YADE_OPENGL
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
			#ifdef YADE_OPENGL
				p->shape->color=color_;
			#endif
			assert(p->shape->nodes.size()==1); // if this fails, enable the block below
			const auto& node0=p->shape->nodes[0];
			assert(node0->pos==Vector3r::Zero());
			node0->pos+=pos;
			auto& dyn=node0->getData<DemData>();
			(*shooter)(dyn.vel,dyn.angVel);
			mass+=dyn.mass;
			stepMass+=dyn.mass;
			assert(node0->hasData<DemData>());
			dem->particles->insert(p);
			#ifdef YADE_OPENGL
				boost::mutex::scoped_lock lock(dem->nodesMutex);
			#endif
			dyn.linIx=dem->nodes.size();
			dem->nodes.push_back(node0);
			// handle multi-nodal particle (unused now)
			#if 0
				// TODO: track energy of the shooter
				Vector3r vel,angVel;
				shooter(dyn.vel,dyn.angVel);
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
	Real currRateNoSmooth=stepMass/(nSteps*scene->dt);
	//cerr<<"[mass="<<mass<<",stepGoalMass="<<stepGoalMass<<",currRateNoSmooth="<<currRateNoSmooth<<"]";
	if(isnan(currRate)) currRate=currRateNoSmooth;
	else currRate=(1-currRateSmooth)*currRate+currRateSmooth*currRateNoSmooth;
}

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
		if(save) deleted.push_back((*dem->particles)[i]);
		const Real& m=p->shape->nodes[0]->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		// FIXME: compute energy that disappeared
		dem->removeParticle(i);
		//dem->particles.remove(i);
		LOG_DEBUG("Particle #"<<p<<" deleted");
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
			for(Particle::id_t memberId: cd.memberIds){
				deleted.push_back((*dem->particles)[memberId]);
			}
			num++;
			for(const auto& n: cd.nodes){ mass+=n->getData<DemData>().mass; stepMass+=n->getData<DemData>().mass; }
			dem->removeClump(i);
			LOG_DEBUG("Clump #"<<i<<" deleted");
		}
		keepClump: ;
	}
	Real currRateNoSmooth=stepMass/((scene->step-stepPrev)*scene->dt);
	if(isnan(currRate)||stepPrev<0) currRate=currRateNoSmooth;
	else currRate=(1-currRateSmooth)*currRate+currRateSmooth*currRateNoSmooth;
	stepPrev=scene->step;
}
py::tuple BoxDeleter::pyDiamMass(){
	py::list dd, mm;
	for(const auto& del: deleted){
		if(!del || !del->shape || del->shape->nodes.size()!=1 || !dynamic_pointer_cast<Sphere>(del->shape)) continue;
		Real d=2*del->shape->cast<Sphere>().radius;
		Real m=del->shape->nodes[0]->getData<DemData>().mass;
		dd.append(d); mm.append(m);
	}
	return py::make_tuple(dd,mm);
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

