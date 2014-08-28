#include<woo/pkg/dem/DynDt.hpp>

#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/Clump.hpp>

WOO_PLUGIN(dem,(DynDt));
CREATE_LOGGER(DynDt);

void DynDt::postLoad(DynDt&,void*){
	if(1.+maxRelInc==1.) throw std::runtime_error("DynDt: maxRelInc too small (1.0+maxRelInc==1.0)");
}

void DynDt::nodalStiffAdd(const shared_ptr<Node>& n, Vector3r& ktrans, Vector3r& krot) const {
	const DemData& dyn=n->getData<DemData>();
	// for every particle with this node, traverse its contacts
	for(auto& p: dyn.parRef){
		for(const auto& idC: p->contacts){
			const auto& C(idC.second);
			if(!C->isReal()) continue;
			assert(C->geom && C->phys);
			assert(dynamic_pointer_cast<L6Geom>(C->geom));
			assert(dynamic_pointer_cast<FrictPhys>(C->phys));
			short ix=C->pIndex(p);
			const auto& ph=C->phys->cast<FrictPhys>();
			const auto& g=C->geom->cast<L6Geom>();
			Vector3r n=C->geom->node->ori*Vector3r::UnitX(); // contact normal in global coords
			Vector3r n2=n.array().pow(2).matrix();
			ktrans+=n2*(ph.kn-ph.kt)+Vector3r::Constant(ph.kt);
			// rotational stiffness only due to translation
			krot+=pow(g.lens[ix],2)*ph.kt*Vector3r(n2[1]+n2[2],n2[2]+n2[0],n2[0]+n2[1]);
		}
		if(p->shape->nodes.size()>1 && intraForce){ intraForce->addIntraStiffness(shared_ptr<Particle>(p,woo::Object::null_deleter()),n,ktrans,krot); }
	}
}

// return square of critical timestep for given node
Real DynDt::nodalCritDtSq(const shared_ptr<Node>& n) const {
	const DemData& dyn=n->getData<DemData>();
	// completely blocked particles are always stable
	if(dyn.isBlockedAll()) return Inf;
	// rotational and translational stiffness
	Vector3r ktrans=Vector3r::Zero(), krot=Vector3r::Zero();
	// add stiffnesses of contacts of all particles belonging to this node
	nodalStiffAdd(n,ktrans,krot);
	// for clump, add stiffnesses of contacts of all particles of all clump nodes
	if(dyn.isClump()){
		const auto& clump=dyn.cast<ClumpData>();
		for(const auto& cn: clump.nodes) nodalStiffAdd(cn,ktrans,krot);
	};
	Real ret=Inf;
	LOG_TRACE("ktrans="<<ktrans.transpose()<<", krot="<<krot.transpose()<<", mass="<<dyn.mass<<", inertia="<<dyn.inertia.transpose());
	for(int i:{0,1,2}){ if(ktrans[i]!=0 && dyn.mass>0.) ret=min(ret,dyn.mass/abs(ktrans[i])); }
	for(int i:{0,1,2}){ if(krot[i]!=0 && dyn.inertia[i]>0.) ret=min(ret,dyn.inertia[i]/abs(krot[i])); }
	return 2*ret; // (sqrt(2)*sqrt(ret))^2
}


Real DynDt::critDt_stiffness() const {
	// traverse nodes, find critical timestep for each of them
	Real ret=Inf;
	for(const auto& n: field->cast<DemField>().nodes){
		ret=min(ret,nodalCritDtSq(n));
		if(ret==0){ LOG_ERROR("DynDt::nodalCriDtSq returning 0 for node at "<<n->pos<<"??"); }
		if(isnan(ret)){ LOG_ERROR("DynDt::nodalCritDtSq returning nan for node at "<<n->pos<<"??"); }
		assert(!isnan(ret));
	}
	return sqrt(ret);
}


Real DynDt::critDt_compute() {
	// just for the case of unitialized finite elements, find the functor if present and store the pointer to it
	// this way it can be called to compute their stiffness matrices on-demand
	intraForce.reset();
	for(const auto& e: scene->engines){
		if(!e->isA<IntraForce>()) continue;
		intraForce=static_pointer_cast<IntraForce>(e); break;
	}

	// compute timestep from contact stiffnesses
	// and from internal stiffnesses of membranes
	Real cdt=critDt_stiffness();
	intraForce.reset();
	return cdt;	
}

void DynDt::run(){
	// apply critical timestep times safety factor
	// prevent too fast changes, so cap the value with maxRelInc
	Real crDt=critDt_compute();

	if(isinf(crDt)){
		if(!dryRun) LOG_INFO("No timestep computed, keeping the current value "<<scene->dt);
		return;
	}
	int nSteps=scene->step-stepPrev;
	Real maxNewDt=scene->dt*pow(1.+maxRelInc,nSteps);
	LOG_DEBUG("dt="<<scene->dt<<", crDt="<<crDt<<" ("<<crDt*scene->dtSafety<<" with dtSafety="<<scene->dtSafety<<"), maxNewDt="<<maxNewDt<<" with exponent "<<1.+maxRelInc<<"^"<<nSteps<<"="<<pow(1.+maxRelInc,nSteps));
	Real nextDt=min(crDt*scene->dtSafety,maxNewDt);
	if(!dryRun){
		// don't bother with diagnostics if there is no change
		if(nextDt!=scene->dt){
			LOG_INFO("Timestep "<<scene->dt<<" -> "<<nextDt);
			this->dt=NaN; // only meaningful with dryRun
			scene->nextDt=nextDt;
		}
	} else {
		this->dt=nextDt;
	}
}
