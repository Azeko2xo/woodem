#include<woo/pkg/dem/Outlet.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Funcs.hpp>
WOO_PLUGIN(dem,(Outlet)(BoxOutlet)(ArcOutlet));
CREATE_LOGGER(Outlet);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Outlet__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_BoxOutlet__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_ArcOutlet__CLASS_BASE_DOC_ATTRS);


void Outlet::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	Real stepMass=0.;
	std::set<Particle::id_t> delParIds;
	std::set<Particle::id_t> delClumpIxs;
	bool deleting=(markMask==0);
	for(size_t i=0; i<dem->nodes.size(); i++){
		const auto& n=dem->nodes[i];
		if(inside!=isInside(n->pos)) continue; // node inside, do nothing
		if(!n->hasData<DemData>()) continue;
		const auto& dyn=n->getData<DemData>();
		// check all particles attached to this node
		for(const Particle* p: dyn.parRef){
			if(!p || !p->shape) continue;
			// mask is checked for both deleting and marking
			if(mask && !(mask & p->mask)) continue;
			// marking, but mask has the markMask already
			if(!deleting && (markMask&p->mask)==markMask) continue;
			// check that all other nodes of that particle may also be deleted
			bool otherOk=true;
			for(const auto& nn: p->shape->nodes){
				// useless to check n again
				if(nn.get()!=n.get() && !(inside!=isInside(nn->pos))){ otherOk=false; break; }
			}
			if(!otherOk) continue;
			LOG_TRACE("DemField.par["<<i<<"] marked for deletion.");
			delParIds.insert(p->id);
		}
		// if this is a clump, check positions of all attached nodes, and masks of their particles
		if(dyn.isClump()){
			assert(dynamic_pointer_cast<ClumpData>(n->getDataPtr<DemData>()));
			const auto& cd=n->getDataPtr<DemData>()->cast<ClumpData>();
			for(const auto& nn: cd.nodes){
				if(inside!=isInside(nn->pos)) goto otherNotOk;
				for(const Particle* p: nn->getData<DemData>().parRef){
					assert(p);
					if(mask && !(mask & p->mask)) goto otherNotOk;
					if(!deleting && (markMask&p->mask)==markMask) goto otherNotOk;
					// don't check positions of all nodes of each particles, just assume that
					// all nodes of that particle are in the clump
				}
					
			}
			LOG_TRACE("DemField.nodes["<<i<<"]: clump marked for deletion, with all its particles.");
			delClumpIxs.insert(i);
			otherNotOk: ;
		}
	}
	for(const auto& id: delParIds){
		const shared_ptr<Particle>& p((*dem->particles)[id]);
		if(deleting && scene->trackEnergy) scene->energy->add(DemData::getEk_any(p->shape->nodes[0],true,true,scene),"kinOutlet",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		const Real& m=p->shape->nodes[0]->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		// handle radius recovery with spheres
		if(recoverRadius && p->shape->isA<Sphere>()){
			auto& s=p->shape->cast<Sphere>();
			Real r=s.radius;
			if(recoverRadius){
				r=cbrt(3*m/(4*M_PI*p->material->density));
				rDivR0.push_back(s.radius/r);
			}
			if(save) diamMassTime.push_back(Vector3r(2*r,m,scene->time));
		} else{
			// all other cases
			if(save) diamMassTime.push_back(Vector3r(2*p->shape->equivRadius(),m,scene->time));
		}
		LOG_TRACE("DemField.par["<<id<<"] will be "<<(deleting?"deleted.":"marked."));
		if(deleting) dem->removeParticle(id);
		else p->mask|=markMask;
		LOG_DEBUG("DemField.par["<<id<<"] "<<(deleting?"deleted.":"marked."));
	}
	for(const auto& ix: delClumpIxs){
		const shared_ptr<Node>& n(dem->nodes[ix]);
		if(deleting && scene->trackEnergy) scene->energy->add(DemData::getEk_any(n,true,true,scene),"kinOutlet",kinEnergyIx,EnergyTracker::ZeroDontCreate);
		Real m=n->getData<DemData>().mass;
		num++;
		mass+=m;
		stepMass+=m;
		if(save) diamMassTime.push_back(Vector3r(2*n->getData<DemData>().cast<ClumpData>().equivRad,m,scene->time));
		LOG_TRACE("DemField.nodes["<<ix<<"] (clump) will be "<<(deleting?"deleted":"marked")<<", with all its particles.");
		if(deleting) dem->removeClump(ix);
		else {
			// apply markMask on all clumps (all particles attached to all nodes in this clump)
			const auto& cd=n->getData<DemData>().cast<ClumpData>();
			for(const auto& nn: cd.nodes) for(Particle* p: nn->getData<DemData>().parRef) p->mask|=markMask;
		}
		LOG_TRACE("DemField.nodes["<<ix<<"] (clump) "<<(deleting?"deleted.":"marked."));
	}

	// use the whole stepPeriod for the first time (might be residuum from previous packing), if specified
	// otherwise the rate might be artificially high at the beginning
	Real currRateNoSmooth=stepMass/((((stepPrev<0 && stepPeriod>0)?stepPeriod:scene->step-stepPrev))*scene->dt); 
	if(isnan(currRate)||stepPrev<0) currRate=currRateNoSmooth;
	else currRate=(1-currRateSmooth)*currRate+currRateSmooth*currRateNoSmooth;
}
py::object Outlet::pyDiamMass(bool zipped) const {
	return DemFuncs::seqVectorToPy(diamMassTime,/*itemGetter*/[](const Vector3r& i)->Vector2r{ return i.head<2>(); },/*zip*/zipped);
}
py::object Outlet::pyDiamMassTime(bool zipped) const {
	return DemFuncs::seqVectorToPy(diamMassTime,/*itemGetter*/[](const Vector3r& i)->Vector3r{ return i; },/*zip*/zipped);
}

Real Outlet::pyMassOfDiam(Real min, Real max) const {
	Real ret=0.;
	for(const auto& dm: diamMassTime){ if(dm[0]>=min && dm[0]<=max) ret+=dm[1]; }
	return ret;
}

py::object Outlet::pyPsd(bool _mass, bool cumulative, bool normalize, int _num, const Vector2r& dRange, const Vector2r& tRange, bool zip, bool emptyOk){
	if(!save) throw std::runtime_error("Outlet.psd(): Outlet.save must be True.");
	auto tOk=[&tRange](const Real& t){ return isnan(tRange.minCoeff()) || (tRange[0]<=t && t<tRange[1]); };
	vector<Vector2r> psd=DemFuncs::psd(diamMassTime,cumulative,normalize,_num,dRange,
		/*diameter getter*/[&tOk](const Vector3r& dmt)->Real{ return tOk(dmt[2])?dmt[0]:NaN; },
		/*weight getter*/[&_mass](const Vector3r& dmt)->Real{ return _mass?dmt[1]:1.; },
		/*emptyOk*/ emptyOk
	);
	return DemFuncs::seqVectorToPy(psd,[](const Vector2r& i)->Vector2r{ return i; },/*zip*/zip);
}


#ifdef WOO_OPENGL
	void Outlet::renderMassAndRate(const Vector3r& pos){
		if(glHideZero && (isnan(currRate) || currRate==0) && mass!=0) return;
		std::ostringstream oss;
		oss.precision(4); oss<<mass;
		if(!isnan(currRate)){ oss.precision(3); oss<<"\n("<<currRate<<")"; }
		GLUtils::GLDrawText(oss.str(),pos,CompUtils::mapColor(glColor));
	}
#endif


#ifdef WOO_OPENGL
void BoxOutlet::render(const GLViewInfo&){
	if(isnan(glColor)) return;
	Vector3r center;
	if(!node){
		GLUtils::AlignedBox(box,CompUtils::mapColor(glColor));
		center=box.center();
	} else {
		glPushMatrix();
			GLUtils::setLocalCoords(node->pos,node->ori);
			GLUtils::AlignedBox(box,CompUtils::mapColor(glColor));
		glPopMatrix();
		center=node->loc2glob(box.center());
	}
	Outlet::renderMassAndRate(center);
}
#endif

void ArcOutlet::postLoad(ArcOutlet&, void* attr){
	if(!cylBox.isEmpty() && (cylBox.min()[0]<0 || cylBox.max()[0]<0)) throw std::runtime_error("ArcOutlet.cylBox: radius bounds (x-component) must be non-negative (not "+to_string(cylBox.min()[0])+".."+to_string(cylBox.max()[0])+").");
	if(!node){ node=make_shared<Node>(); throw std::runtime_error("ArcOutlet.node: must not be None (dummy node created)."); }
};

bool ArcOutlet::isInside(const Vector3r& p){ return CompUtils::cylCoordBox_contains_cartesian(cylBox,node->glob2loc(p)); }
#ifdef WOO_OPENGL
	void ArcOutlet::render(const GLViewInfo&){
		if(isnan(glColor)) return;
		glPushMatrix();
			GLUtils::setLocalCoords(node->pos,node->ori);
			GLUtils::RevolvedRectangle(cylBox,CompUtils::mapColor(glColor),glSlices);
		glPopMatrix();
		Outlet::renderMassAndRate(node->loc2glob(CompUtils::cyl2cart(cylBox.center())));
	}
#endif
