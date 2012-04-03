#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>

// temporary
#include<yade/pkg/dem/G3Geom.hpp>

YADE_PLUGIN(dem,(CGeomFunctor)(CGeomDispatcher)(CPhysFunctor)(CPhysDispatcher)(LawFunctor)(LawDispatcher)(ContactLoop));
CREATE_LOGGER(ContactLoop);

shared_ptr<Contact> CGeomDispatcher::explicitAction(Scene* _scene, const shared_ptr<Particle>& p1, const shared_ptr<Particle>& p2, bool force){
	scene=_scene;
	updateScenePtr();
	assert(p1->shape && p2->shape); assert(!p1->shape->nodes.empty() && !p2->shape->nodes.empty());

	// compute shift2
	Vector3i cellDist=Vector3i::Zero();
	if(scene->isPeriodic){
		for(int i=0;i<3;i++) cellDist[i]=-(int)((p2->shape->nodes[0]->pos[i]-p1->shape->nodes[0]->pos[i])/scene->cell->getSize()[i]+.5);
	}
	Vector3r shift2=(scene->isPeriodic?scene->cell->intrShiftPos(cellDist):Vector3r::Zero());

	shared_ptr<Contact> C(new Contact); C->pA=p1; C->pB=p2;
	C->cellDist=cellDist;
	// FIXME: this code is more or less duplicated from ContactLoop :-(
	bool swap=false;
	const shared_ptr<CGeomFunctor> geoF=getFunctor2D(C->pA->shape,C->pB->shape,swap);
	if(!geoF) throw invalid_argument("IGeomDispatcher::explicitAction could not dispatch for given types ("+p1->shape->getClassName()+", "+p2->shape->getClassName()+").");
	if(swap){C->swapOrder();}
	bool succ=geoF->go(C->pA->shape,C->pB->shape,shift2,/*force*/true,C);
	if(!succ) throw logic_error("Functor "+geoF->getClassName()+"::go returned false, even if asked to force CGeom creation. Please report bug.");
	return C;
}

void CPhysDispatcher::explicitAction(Scene* _scene, shared_ptr<Material>& mA, shared_ptr<Material>& mB, shared_ptr<Contact>& C){
	scene=_scene;
	updateScenePtr();
	if(!C->geom) throw invalid_argument("CPhysDispatcher::explicitAction received contact without Contact.geom.");
	bool dummy;
	const shared_ptr<CPhysFunctor> phyF=getFunctor2D(mA,mB,dummy);
	if(!phyF) throw invalid_argument("CPhysDispatcher::explicitAction could not dispatch for given types ("+mA->getClassName()+", "+mB->getClassName()+")");
	phyF->go(mA,mB,C);
};


void ContactLoop::pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){
	if(py::len(t)==0) return; // nothing to do
	if(py::len(t)!=3) throw invalid_argument("Exactly 3 lists of functors must be given");
	// parse custom arguments (3 lists) and do in-place modification of args
	typedef std::vector<shared_ptr<CGeomFunctor> > vecGeom;
	typedef std::vector<shared_ptr<CPhysFunctor> > vecPhys;
	typedef std::vector<shared_ptr<LawFunctor> > vecLaw;
	vecGeom vg=py::extract<vecGeom>(t[0])();
	vecPhys vp=py::extract<vecPhys>(t[1])();
	vecLaw vl=py::extract<vecLaw>(t[2])();
	FOREACH(shared_ptr<CGeomFunctor> gf, vg) this->geoDisp->add(gf);
	FOREACH(shared_ptr<CPhysFunctor> pf, vp) this->phyDisp->add(pf);
	FOREACH(shared_ptr<LawFunctor> cf, vl)   this->lawDisp->add(cf);
	t=py::tuple(); // empty the args; not sure if this is OK, as there is some refcounting in raw_constructor code
}

void ContactLoop::getLabeledObjects(std::map<std::string, py::object>& m){
	geoDisp->getLabeledObjects(m);
	phyDisp->getLabeledObjects(m);
	lawDisp->getLabeledObjects(m);
	GlobalEngine::getLabeledObjects(m);
}


// #define IDISP_TIMING

#ifdef IDISP_TIMING
	#define IDISP_CHECKPOINT(cpt) timingDeltas->checkpoint(cpt)
#else
	#define IDISP_CHECKPOINT(cpt)
#endif

void ContactLoop::run(){
	#ifdef IDISP_TIMING
		timingDeltas->start();
	#endif

	if(!applyForces && !_forceApplyChecked){
		shared_ptr<IntraForce> intra;
		for(const auto& e: scene->engines){ intra=dynamic_pointer_cast<IntraForce>(e); if(intra) break; }
		if(!intra) LOG_WARN("ContactLoop.applyForce==False and no IntraForce in Scene.engines! Are you sure this is ok? (proceeding)");
		_forceApplyChecked=true;
	}

	DemField& dem=field->cast<DemField>();

	if(dem.contacts.removeAllPending()>0 && !alreadyWarnedNoCollider){
		LOG_WARN("Contacts pending removal found (and were removed); no collider being used?");
		alreadyWarnedNoCollider=true;
	}

	if(dem.contacts.dirty){
		throw std::logic_error("ContactContainer::dirty is true; the collider should re-initialize in such case and clear the dirty flag.");
	}
	// update Scene* of the dispatchers
	geoDisp->scene=phyDisp->scene=lawDisp->scene=scene;
	geoDisp->field=phyDisp->field=lawDisp->field=field;
	// ask dispatchers to update Scene* of their functors
	geoDisp->updateScenePtr(); phyDisp->updateScenePtr(); lawDisp->updateScenePtr();

	// cache transformed cell size
	Matrix3r cellHsize; if(scene->isPeriodic) cellHsize=scene->cell->hSize;

	stress=Matrix3r::Zero();

	// force removal of interactions that were not encountered by the collider
	// (only for some kinds of colliders; see comment for InteractionContainer::iterColliderLastRun)
	bool removeUnseen=(dem.contacts.stepColliderLastRun>=0 && dem.contacts.stepColliderLastRun==scene->step);

	const bool doStress=(evalStress && scene->isPeriodic);
		
	size_t size=dem.contacts.size();
	#ifdef YADE_OPENMP
		#pragma omp parallel for schedule(guided)	
	#endif
	for(size_t i=0; i<size; i++){
		const shared_ptr<Contact>& C=dem.contacts[i];

		if(unlikely(removeUnseen && !C->isReal() && C->stepLastSeen<scene->step)) { removeAfterLoop(C); continue; }


		bool swap=false;
		if(!C->isReal()){
			const shared_ptr<CGeomFunctor> cgf=geoDisp->getFunctor2D(C->pA->shape,C->pB->shape,swap);
			if(swap){ C->swapOrder(); }
			if(!cgf) continue;
		}
		// the order is as the geometry functor expects it
		shared_ptr<Shape>& sA(C->pA->shape); shared_ptr<Shape>& sB(C->pB->shape);

		bool geomCreated=geoDisp->operator()(sA,sB,(scene->isPeriodic?scene->cell->intrShiftPos(C->cellDist):Vector3r::Zero()),/*force*/false,C);
		if(!geomCreated){
			if(/* has both geo and phy */C->isReal()) LOG_ERROR("CGeomFunctor "<<geoDisp->getClassName()<<" did not update existing contact ##"<<C->pA->id<<"+"<<C->pB->id);
			continue;
		}

		// CPhy
		if(!C->phys) C->stepMadeReal=scene->step;
		phyDisp->operator()(C->pA->material,C->pB->material,C);

		// CLaw
		lawDisp->operator()(C->geom,C->phys,C);

		if(applyForces && C->isReal()){
			for(const shared_ptr<Particle>& particle:{C->pA,C->pB}){
				const shared_ptr<Shape>& sh(particle->shape);
				if(!sh) continue;
				if(sh->nodes.size()!=1){
					for(size_t i=0; i<sh->nodes.size(); i++){
						if((sh->nodes[i]->getData<DemData>().flags&DemData::DOF_ALL)!=DemData::DOF_ALL) LOG_WARN("Multinodal #"<<particle->id<<" has free DOFs, but force will not be applied; set ContactLoop.applyForces=False and use IntraForce(...) dispatcher instead.");
					}
					continue;
				}
				Vector3r F,T,xc;
				std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/0,scene);
				sh->nodes[0]->getData<DemData>().addForceTorque(F,xc.cross(F)+T);
			}
		}

		// track gradV work
		/* this is meant to avoid calling extra loop at every step, since the work must be evaluated incrementally */
		if(doStress && /*contact law deleted the contact?*/ C->isReal()){
			if(C->pA->shape->nodes.size()!=1 || C->pB->shape->nodes.size()!=1) throw std::runtime_error("ContactLoop.trackWork not allowed with multi-nodal particles in contact (##"+lexical_cast<string>(C->pA->id)+"+"+lexical_cast<string>(C->pB->id)+")");
			#if 0
				const Real d0=(C->pB->shape->nodes[0]->pos-C->pA->shape->nodes[0]->pos+scene->cell->intrShiftPos(C->cellDist)).norm();
				Vector3r n=C->geom->node->ori.conjugate()*Vector3r::UnitX(); // normal in global coords
				#if 1
					// g3geom doesn't set local x axis propertly, use its internal data instead
					G3Geom* g3g=dynamic_cast<G3Geom*>(C->geom.get());
					if(g3g) n=g3g->normal;
				#endif
				Vector3r F=C->geom->node->ori.conjugate()*C->phys->force;
				Real fN=F.dot(n); Vector3r fT=F-n*fN;
				// this could be done better using reductions, but those don't allow overloaded operators :-|
				#pragma omp critical
				{ for(int i:{0,1,2}) for(int j:{0,1,2}) stress(i,j)+=d0*(fN*n[i]*n[j]+.5*(fT[i]*n[j]+fT[j]*n[i])); }
			#endif
			Vector3r branch=(C->pB->shape->nodes[0]->pos-C->pA->shape->nodes[0]->pos+scene->cell->intrShiftPos(C->cellDist));
			Vector3r F=C->geom->node->ori.conjugate()*C->phys->force; // force in global coords
			#pragma omp critical
			stress.noalias()+=F*branch.transpose();
		}
	}
	// process removeAfterLoop
	#ifdef YADE_OPENMP
		FOREACH(list<shared_ptr<Contact> >& l, removeAfterLoopRefs){
			FOREACH(const shared_ptr<Contact>& c,l) dem.contacts.remove(c);
			l.clear();
		}
	#else
		FOREACH(const shared_ptr<Contact>& c, removeAfterLoopRefs) dem.contacts.remove(c);
		removeAfterLoopRefs.clear();
	#endif
	// compute gradVWork eventually
	if(doStress){
		stress/=scene->cell->getVolume();
		if(scene->trackEnergy){
			Matrix3r midStress=.5*(stress+prevStress);
			Real midVol=(!isnan(prevVol)?.5*(prevVol+scene->cell->getVolume()):scene->cell->getVolume());
			Real dW=-(scene->cell->gradV*midStress).trace()*scene->dt*midVol;
			scene->energy->add(dW,"gradV",gradVIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
		}
		//prevTrGradVStress=trGradVStress;
		prevVol=scene->cell->getVolume();
		prevStress=stress;
	}
}
