#include<woo/pkg/dem/Inlet.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/Funcs.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Psd.hpp> // PsdSphereGenerator::sanitizePsd

#include<woo/lib/sphere-pack/SpherePack.hpp>
#include<woo/lib/smoothing/LinearInterpolate.hpp>

// hack
#include<woo/pkg/dem/InsertionSortCollider.hpp>

#include<boost/range/algorithm/lower_bound.hpp>

#include<boost/tuple/tuple_comparison.hpp>

WOO_PLUGIN(dem,(Inlet)(ParticleGenerator)(MinMaxSphereGenerator)(ParticleShooter)(AlignedMinMaxShooter)(RandomInlet)(BoxInlet)(BoxInlet2d)(CylinderInlet)(ArcInlet)/*(ArcShooter)*/(SpatialBias)(AxialBias)(PsdAxialBias));
WOO_IMPL_LOGGER(RandomInlet);

WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Inlet__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ParticleGenerator__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_MinMaxSphereGenerator_CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_ParticleShooter__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_AlignedMinMaxShooter__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_RandomInlet__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxInlet__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxInlet2d__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_CylinderInlet__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_ArcInlet__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_PY(woo_dem_SpatialBias__CLASS_BASE_DOC_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_AxialBias__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_PsdAxialBias__CLASS_BASE_DOC_ATTRS);

WOO_IMPL_LOGGER(PsdAxialBias);


#if 0
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_ArcShooter__CLASS_BASE_DOC_ATTRS);
#endif

void AxialBias::postLoad(AxialBias&,void*){
	if(axis<0 || axis>2) throw std::runtime_error("AxialBias.axis: must be in 0..2 (not "+to_string(axis)+").");
}

Vector3r AxialBias::unitPos(const Real& d){
	Vector3r p3=Mathr::UnitRandom3();
	if(axis<0 || axis>2) throw std::runtime_error("AxialBias.axis: must be in 0..2 (not "+to_string(axis));
	Real& p(p3[axis]);
	p=CompUtils::clamped((d-d01[0])/(d01[1]-d01[0])+(p-.5)*fuzz,0,1);
	return p3;
}

void PsdAxialBias::postLoad(PsdAxialBias&,void*){
	PsdSphereGenerator::sanitizePsd(psdPts,"PsdAxialBias.psdPts");
	if(!reorder.empty()){
		if(reorder.size()!=psdPts.size()) throw std::runtime_error("PsdAxialBias.reorder: must have the length of psdPts minus 1 (len(reorder)=="+to_string(reorder.size())+", len(psdPts)="+to_string(psdPts.size())+").");
		for(int i=0; i<(int)psdPts.size()-1; i++){
			size_t c=std::count(reorder.begin(),reorder.end(),i);
			if(c!=1) throw std::runtime_error("PsdAxialBias.reorder: must contain all integers in 0.."+to_string(psdPts.size()-1)+", each exactly once ("+to_string(c)+" occurences of "+to_string(i)+").");
		}
	}
}


Vector3r PsdAxialBias::unitPos(const Real& d){
	if(psdPts.empty()) throw std::runtime_error("AxialBias.psdPts: must not be empty.");
	Vector3r p3=Mathr::UnitRandom3();
	Real& p(p3[axis]);
	size_t pos=0; // contains fraction number in the PSD
	if(!discrete){
		p=linearInterpolate(d,psdPts,pos);
	} else {
		Real f0,f1,t;
		std::tie(f0,f1,t)=linearInterpolateRel(d,psdPts,pos);
		LOG_TRACE("PSD interp for "<<d<<": "<<f0<<".."<<f1<<", pos="<<pos<<", t="<<t);
		if(t==0.){
			// we want the interval to the left of our point
			if(pos==0){ LOG_WARN("PsdAxiaBias.unitPos: discrete PSD interpolation returned point at the beginning for d="<<d<<", which should be zero. No interpolation done, setting 0."); p=0; return p3; }
			f1=f0; f0=psdPts[pos-1].y();
		}
		// pick randomly in our interval
		p=f0+Mathr::UnitRandom()*(f1-f0);
		//cerr<<"Picked from "<<d0<<".."<<d1<<": "<<p<<endl;
	}
	// reorder fractions if desired
	if(!reorder.empty()){
		Real a=0, dp=p-psdPts[pos].y();
		for(size_t i=0; i<reorder.size(); i++){
			if(reorder[i]==(int)pos){ p=a+dp; break; } 
			// cumulate bin sizes of other fractions, except for the very last one which is zero by definition
			if(i<psdPts.size()-1) a+=psdPts[reorder[i]+1].y()-psdPts[reorder[i]].y();
		}
	}
	// apply fuzz
	p=CompUtils::clamped(p+fuzz*(Mathr::UnitRandom()-.5),0,1);
	if(invert) p=1-p;
	return p3;
}



#ifdef WOO_OPENGL
	void Inlet::renderMassAndRate(const Vector3r& pos){
		std::ostringstream oss; oss.precision(4); oss<<mass;
		if(maxMass>0){ oss<<"/"; oss.precision(4); oss<<maxMass; }
		if(!isnan(currRate) && !isinf(currRate)){ oss.precision(3); oss<<"\n("<<currRate<<")"; }
		GLUtils::GLDrawText(oss.str(),pos,CompUtils::mapColor(glColor));
	}
#endif


py::object ParticleGenerator::pyPsd(bool mass, bool cumulative, bool normalize, const Vector2r& dRange, const Vector2r& tRange, int num) const {
	if(!save) throw std::runtime_error("ParticleGenerator.save must be True for calling ParticleGenerator.psd()");
	auto tOk=[&tRange](const Real& t){ return isnan(tRange.minCoeff()) || (tRange[0]<=t && t<tRange[1]); };
	vector<Vector2r> psd=DemFuncs::psd(genDiamMassTime,/*cumulative*/cumulative,/*normalize*/normalize,num,dRange,
		/*diameter getter*/[&tOk](const Vector3r& dmt) ->Real { return tOk(dmt[2])?dmt[0]:NaN; },
		/*weight getter*/[&](const Vector3r& dmt) -> Real{ return mass?dmt[1]:1.; }
	);// 
	return DemFuncs::seqVectorToPy(psd,[](const Vector2r& i)->Vector2r{ return i; },/*zip*/false);
}

py::object ParticleGenerator::pyDiamMass(bool zipped) const {
	return DemFuncs::seqVectorToPy(genDiamMassTime,/*itemGetter*/[](const Vector3r& i)->Vector2r{ return i.head<2>(); },/*zip*/zipped);
}

Real ParticleGenerator::pyMassOfDiam(Real min, Real max) const{
	Real ret=0.;
	for(const Vector3r& vv: genDiamMassTime){
		if(vv[0]>=min && vv[0]<=max) ret+=vv[1];
	}
	return ret;
};



std::tuple<Real,vector<ParticleGenerator::ParticleAndBox>>
MinMaxSphereGenerator::operator()(const shared_ptr<Material>&mat, const Real& time){
	if(isnan(dRange[0]) || isnan(dRange[1]) || dRange[0]>dRange[1]) throw std::runtime_error("MinMaxSphereGenerator: dRange[0]>dRange[1], or they are NaN!");
	Real r=.5*(dRange[0]+Mathr::UnitRandom()*(dRange[1]-dRange[0]));
	auto sphere=DemFuncs::makeSphere(r,mat);
	Real m=sphere->shape->nodes[0]->getData<DemData>().mass;
	if(save) genDiamMassTime.push_back(Vector3r(2*r,m,time));
	return std::make_tuple(2*r,vector<ParticleAndBox>({{sphere,AlignedBox3r(Vector3r(-r,-r,-r),Vector3r(r,r,r))}}));
};

Real MinMaxSphereGenerator::critDt(Real density, Real young) {
	return DemFuncs::spherePWaveDt(dRange[0],density,young);
}


Real RandomInlet::critDt() {
	if(!generator) return Inf;
	Real ret=Inf;
	for(const auto& m: materials){
		const auto em=dynamic_pointer_cast<ElastMat>(m);
		if(!em) continue;
		ret=min(ret,generator->critDt(em->density,em->young));
	}
	return ret;
}


bool Inlet::everythingDone(){
	if((maxMass>0 && mass>=maxMass) || (maxNum>0 && num>=maxNum)){
		LOG_INFO("mass or number reached, making myself dead.");
		dead=true;
		if(zeroRateAtStop) currRate=0.;
		if(!doneHook.empty()){ LOG_DEBUG("Running doneHook: "<<doneHook); Engine::runPy(doneHook); }
		return true;
	}
	return false;
}


void RandomInlet::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	if(!generator) throw std::runtime_error("RandomInlet.generator==None!");
	if(materials.empty()) throw std::runtime_error("RandomInlet.materials is empty!");
	if(collideExisting){
		if(!collider){
		for(const auto& e: scene->engines){ collider=dynamic_pointer_cast<Collider>(e); if(collider) break; }
		if(!collider) throw std::runtime_error("RandomInlet: no Collider found within engines (needed for collisions detection with already existing particles; if you don't need that, set collideExisting=False.)");
		}
		if(dynamic_pointer_cast<InsertionSortCollider>(collider)) static_pointer_cast<InsertionSortCollider>(collider)->forceInitSort=true;
	}
	if(isnan(massRate)) throw std::runtime_error("RandomInlet.massRate must be given (is "+to_string(massRate)+"); if you want to generate as many particles as possible, say massRate=0.");
	if(massRate<=0 && maxAttempts==0) throw std::runtime_error("RandomInlet.massFlowRate<=0 (no massFlowRate prescribed), but RandomInlet.maxAttempts==0. (unlimited number of attempts); this would cause infinite loop.");
	if(maxAttempts<0){
		std::runtime_error("RandomInlet.maxAttempts must be non-negative. Negative value, leading to meaking engine dead, is achieved by setting atMaxAttempts=RandomInlet.maxAttDead now.");
	}
	spheresOnly=generator->isSpheresOnly();
	padDist=generator->padDist();
	if(isnan(padDist) || padDist<0) throw std::runtime_error(generator->pyStr()+".padDist(): returned invalid value "+to_string(padDist));

	// as if some time has already elapsed at the very start
	// otherwise mass flow rate is too big since one particle during Δt exceeds it easily
	// equivalent to not running the first time, but without time waste
	if(stepPrev==-1 && stepPeriod>0) stepPrev=-stepPeriod; 
	long nSteps=scene->step-stepPrev;
	// to be attained in this step;
	stepGoalMass+=massRate*scene->dt*nSteps; // stepLast==-1 if never run, which is OK
	vector<AlignedBox3r> genBoxes; // of particles created in this step
	vector<shared_ptr<Particle>> generated;
	Real stepMass=0.;

	SpherePack spheres;
	//
	if(spheresOnly){
		spheres.pack.reserve(dem->particles->size()*1.2); // something extra for generated particles
		// HACK!!!
		bool isBox=dynamic_cast<BoxInlet*>(this);
		AlignedBox3r box;
		if(isBox){ box=this->cast<BoxInlet>().box; }
		for(const auto& p: *dem->particles){
			if(p->shape->isA<Sphere>() && (!isBox || box.contains(p->shape->nodes[0]->pos))) spheres.pack.push_back(SpherePack::Sph(p->shape->nodes[0]->pos,p->shape->cast<Sphere>().radius));
		}
	}

	while(true){
		// finished forever
		if(everythingDone()) return;

		// finished in this step
		if(massRate>0 && mass>=stepGoalMass) break;

		shared_ptr<Material> mat;
		Vector3r pos=Vector3r::Zero();
		Real diam;
		vector<ParticleGenerator::ParticleAndBox> pee;
		int attempt=-1;
		while(true){
			attempt++;
			/***** too many tries, give up ******/
			if(attempt>=maxAttempts){
				generator->revokeLast(); // last particle could not be placed
				if(massRate<=0){
					LOG_DEBUG("maxAttempts="<<maxAttempts<<" reached; since massRate is not positive, we're done in this step");
					goto stepDone;
				}
				switch(atMaxAttempts){
					case MAXATT_ERROR: throw std::runtime_error("RandomInlet.maxAttempts reached ("+lexical_cast<string>(maxAttempts)+")"); break;
					case MAXATT_DEAD:{
						LOG_INFO("maxAttempts="<<maxAttempts<<" reached, making myself dead.");
						this->dead=true;
						return;
					}
					case MAXATT_WARN: LOG_WARN("maxAttempts "<<maxAttempts<<" reached before required mass amount was generated; continuing, since maxAttemptsError==False"); break;
					case MAXATT_SILENT: break;
					default: throw std::invalid_argument("Invalid value of RandomInlet.atMaxAttempts="+to_string(atMaxAttempts)+".");
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
				std::tie(diam,pee)=(*generator)(mat,scene->time);
				assert(!pee.empty());
				LOG_TRACE("Placing "<<pee.size()<<"-sized particle; first component is a "<<pee[0].par->getClassName()<<", extents from "<<pee[0].extents.min().transpose()<<" to "<<pee[0].extents.max().transpose());
			}

			pos=randomPosition(diam,padDist); // overridden in child classes
			LOG_TRACE("Trying pos="<<pos.transpose());
			for(const auto& pe: pee){
				// make translated copy
				AlignedBox3r peBox(pe.extents); peBox.translate(pos); 
				// box is not entirely within the factory shape
				if(!validateBox(peBox)){ LOG_TRACE("validateBox failed."); goto tryAgain; }

				const shared_ptr<woo::Sphere>& peSphere=dynamic_pointer_cast<woo::Sphere>(pe.par->shape);

				if(spheresOnly){
					if(!peSphere) throw std::runtime_error("RandomInlet.spheresOnly: is true, but a nonspherical particle ("+pe.par->shape->pyStr()+") was returned by the generator.");
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
				} else {
					// see intersection with existing particles
					bool overlap=false;
					if(collideExisting){
						vector<Particle::id_t> ids=collider->probeAabb(peBox.min(),peBox.max());
						for(const auto& id: ids){
							LOG_TRACE("Collider reports intersection with #"<<id);
							if(id>(Particle::id_t)dem->particles->size() || !(*dem->particles)[id]) continue;
							const shared_ptr<Shape>& sh2((*dem->particles)[id]->shape);
							// no spheres, or they are too close
							if(!peSphere || !sh2->isA<woo::Sphere>() || 1.1*(pos-sh2->nodes[0]->pos).squaredNorm()<pow(peSphere->radius+sh2->cast<Sphere>().radius,2)) goto tryAgain;
						}
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
				}
			}
			LOG_DEBUG("No collision (attempt "<<attempt<<"), particle will be created :-) ");
			if(spheresOnly){
				// num will be the same for all spheres within this clump (abuse the *num* counter)
				for(const auto& pe: pee){
					Vector3r subPos=pe.par->shape->nodes[0]->pos;
					Real r=pe.par->shape->cast<Sphere>().radius;
					spheres.pack.push_back(SpherePack::Sph((pos+subPos),r,/*clumpId*/(pee.size()==1?-1:num)));
				}
			}
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
					if(color_>=0) p->shape->color=color_; // otherwise keep generator-assigned color
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
			if(shooter) (*shooter)(clump);
			if(scene->trackEnergy) scene->energy->add(-DemData::getEk_any(clump,true,true,scene),"kinInlet",kinEnergyIx,EnergyTracker::ZeroDontCreate);
			if(dyn.angVel!=Vector3r::Zero()){
				throw std::runtime_error("pkg/dem/RandomInlet.cpp: generated particle has non-zero angular velocity; angular momentum should be computed so that rotation integration is correct, but it was not yet implemented.");
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
				if(color_>=0) p->shape->color=color_;
			#endif
			assert(p->shape->nodes.size()==1); // if this fails, enable the block below
			const auto& node0=p->shape->nodes[0];
			assert(node0->pos==Vector3r::Zero());
			node0->pos+=pos;
			auto& dyn=node0->getData<DemData>();
			if(shooter) (*shooter)(node0);
			if(scene->trackEnergy) scene->energy->add(-DemData::getEk_any(node0,true,true,scene),"kinInlet",kinEnergyIx,EnergyTracker::ZeroDontCreate);
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
				// in case the particle is multinodal, apply the same to all nodes
				if(shooter) shooter(p.shape->nodes[0]);
				const Vector3r& vel(p->shape->nodes[0]->getData<DemData>().vel); const Vector3r& angVel(p->shape->nodes[0]->getData<DemData>().angVel);
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

Vector3r BoxInlet::randomPosition(const Real& rad, const Real& padDist){
	AlignedBox3r box2(box.min()+padDist*Vector3r::Ones(),box.max()-padDist*Vector3r::Ones());
	if(!spatialBias) return box2.sample();
	else return box2.min()+spatialBias->unitPos(rad).cwiseProduct(box2.sizes());
}



#ifdef BOX_FACTORY_PERI
bool BoxInlet::validatePeriodicBox(const AlignedBox3r& b) const {
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

#ifdef WOO_OPENGL
	void CylinderInlet::render(const GLViewInfo&){
		if(isnan(glColor) || !node) return;
		GLUtils::Cylinder(node->loc2glob(Vector3r::Zero()),node->loc2glob(Vector3r(height,0,0)),radius,CompUtils::mapColor(glColor),/*wire*/true,/*caps*/false,/*rad2: use rad1*/-1,/*slices*/glSlices);
		Inlet::renderMassAndRate(node->loc2glob(Vector3r(height/2,0,0)));
	}
#endif


Vector3r CylinderInlet::randomPosition(const Real& rad, const Real& padDist) {
	if(!node) throw std::runtime_error("CylinderInlet.node==None.");
	if(padDist>radius) throw std::runtime_error("CylinderInlet.randomPosition: padDist="+to_string(padDist)+" > radius="+to_string(radius));
	// http://stackoverflow.com/questions/5837572/generate-a-random-point-within-a-circle-uniformly
	Vector3r rand=(spatialBias?spatialBias->unitPos(rad):Mathr::UnitRandom3());
	Real t=2*M_PI*rand[0];
	Real u=2*rand[1];
	Real r=u>1.?2.-u:u;
	Real h=rand[2]*(height-2*padDist)+padDist;
	Vector3r p(h,(radius-padDist)*r*cos(t),(radius-padDist)*r*sin(t));
	return node->loc2glob(p);

};

bool CylinderInlet::validateBox(const AlignedBox3r& b) {
	if(!node) throw std::runtime_error("CylinderInlet.node==None.");
	/* check all corners are inside the cylinder */
	#if 0
		boxesTried.push_back(b);
	#endif
	for(AlignedBox3r::CornerType c:{AlignedBox3r::BottomLeft,AlignedBox3r::BottomRight,AlignedBox3r::TopLeft,AlignedBox3r::TopRight,AlignedBox3r::BottomLeftCeil,AlignedBox3r::BottomRightCeil,AlignedBox3r::TopLeftCeil,AlignedBox3r::TopRightCeil}){
		Vector3r p=node->glob2loc(b.corner(c));
		if(p[0]<0. || p[0]>height) return false;
		if(Vector2r(p[1],p[2]).squaredNorm()>pow(radius,2)) return false;
	}
	return true;
}


void ArcInlet::postLoad(ArcInlet&, void* attr){
	if(!cylBox.isEmpty() && (cylBox.min()[0]<0 || cylBox.max()[0]<0)) throw std::runtime_error("ArcInlet.cylBox: radius bounds (x-component) must be non-negative (not "+to_string(cylBox.min()[0])+".."+to_string(cylBox.max()[0])+").");
	if(!node){ node=make_shared<Node>(); throw std::runtime_error("ArcInlet.node: must not be None (dummy node created)."); }
};



Vector3r ArcInlet::randomPosition(const Real& rad, const Real& padDist) {
	AlignedBox3r b2(cylBox);
	Vector3r pad(padDist,padDist/cylBox.min()[0],padDist); b2.min()+=pad; b2.max()-=pad;
	if(!spatialBias) return node->loc2glob(CompUtils::cylCoordBox_sample_cartesian(b2));
	else return node->loc2glob(CompUtils::cylCoordBox_sample_cartesian(b2,spatialBias->unitPos(rad)));
}

bool ArcInlet::validateBox(const AlignedBox3r& b) {
	for(const auto& c:{AlignedBox3r::BottomLeftFloor,AlignedBox3r::BottomRightFloor,AlignedBox3r::TopLeftFloor,AlignedBox3r::TopRightFloor,AlignedBox3r::BottomLeftCeil,AlignedBox3r::BottomRightCeil,AlignedBox3r::TopLeftCeil,AlignedBox3r::TopRightCeil}){
		// FIXME: all boxes fail?!
		if(!CompUtils::cylCoordBox_contains_cartesian(cylBox,node->glob2loc(b.corner(c)))) return false;
	}
	return true;
}

#ifdef WOO_OPENGL
	void ArcInlet::render(const GLViewInfo&) {
		if(isnan(glColor)) return;
		glPushMatrix();
			GLUtils::setLocalCoords(node->pos,node->ori);
			GLUtils::RevolvedRectangle(cylBox,CompUtils::mapColor(glColor),glSlices);
		glPopMatrix();
		Inlet::renderMassAndRate(node->loc2glob(CompUtils::cyl2cart(cylBox.center())));
	}
#endif

