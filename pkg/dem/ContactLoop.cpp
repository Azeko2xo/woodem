#include<woo/pkg/dem/ContactLoop.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>

// temporary
#include<woo/pkg/dem/G3Geom.hpp>

WOO_PLUGIN(dem,(CGeomFunctor)(CGeomDispatcher)(CPhysFunctor)(CPhysDispatcher)(LawFunctor)(LawDispatcher)(ContactLoop));
WOO_IMPL_LOGGER(ContactLoop);

WOO_IMPL__CLASS_BASE_DOC_PY(woo_dem_CGeomFunctor__CLASS_BASE_DOC_PY);
WOO_IMPL__CLASS_BASE_DOC_PY(woo_dem_CPhysFunctor__CLASS_BASE_DOC_PY);
WOO_IMPL__CLASS_BASE_DOC_PY(woo_dem_LawFunctor__CLASS_BASE_DOC_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_ContactLoop__CLASS_BASE_DOC_ATTRS_CTOR);


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
	const shared_ptr<CGeomFunctor> geoF=getFunctor2D(p1->shape,p2->shape,swap);
	if(!geoF) throw invalid_argument("IGeomDispatcher::explicitAction could not dispatch for given types ("+p1->shape->getClassName()+", "+p2->shape->getClassName()+").");
	if(swap){C->swapOrder();}
	// possibly swapped here, don't use p1, p2 anymore
	bool succ=geoF->go(C->leakPA()->shape,C->leakPB()->shape,shift2,/*force*/true,C);
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
	for(shared_ptr<CGeomFunctor> gf: vg) this->geoDisp->add(gf);
	for(shared_ptr<CPhysFunctor> pf: vp) this->phyDisp->add(pf);
	for(shared_ptr<LawFunctor> cf: vl)   this->lawDisp->add(cf);
	t=py::tuple(); // empty the args; not sure if this is OK, as there is some refcounting in raw_constructor code
}

void ContactLoop::getLabeledObjects(const shared_ptr<LabelMapper>& labelMapper){
	geoDisp->getLabeledObjects(labelMapper);
	phyDisp->getLabeledObjects(labelMapper);
	lawDisp->getLabeledObjects(labelMapper);
	Engine::getLabeledObjects(labelMapper);
}

void ContactLoop::reorderContacts(){
	// traverse contacts, move real onces towards the beginning
	ContactContainer& cc=*(field->cast<DemField>().contacts);
	size_t N=cc.size();
	if(N<2) return;
	size_t lowestReal=0;
	// traverse from the end, and put to the beginning
	for(size_t i=N-1; i>lowestReal; i--){
		const auto& C=cc[i];
		if(!C->isReal()) continue;
		while(lowestReal<i && cc[lowestReal]->isReal()) lowestReal++;
		if(lowestReal==i) return; // nothing to do, already reached the part of real contacts
		std::swap(cc[i],cc[lowestReal]);
		cc[i]->linIx=i;
		cc[lowestReal]->linIx=lowestReal;
	}
}

void ContactLoop::run(){
	#ifdef CONTACTLOOP_TIMING
		timingDeltas->start();
	#endif

	DemField& dem=field->cast<DemField>();

	if(dem.contacts->removeAllPending()>0 && !alreadyWarnedNoCollider){
		LOG_WARN("Contacts pending removal found (and were removed); no collider being used?");
		alreadyWarnedNoCollider=true;
	}

	if(dem.contacts->dirty){
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
	bool removeUnseen=(dem.contacts->stepColliderLastRun>=0 && dem.contacts->stepColliderLastRun==scene->step);

	const bool doStress=(evalStress && scene->isPeriodic);
	const bool deterministic(scene->deterministic);

	if(reorderEvery>0 && (scene->step%reorderEvery==0)) reorderContacts();

	size_t size=dem.contacts->size();

	CONTACTLOOP_CHECKPOINT("prologue");

	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)
	#endif
	for(size_t i=0; i<size; i++){
		CONTACTLOOP_CHECKPOINT("loop-begin");
		const shared_ptr<Contact>& C=(*dem.contacts)[i];

		if(unlikely(removeUnseen && !C->isReal() && C->stepLastSeen<scene->step)) { removeAfterLoop(C); continue; }
		if(unlikely(!C->isReal() && !C->isColliding())){ removeAfterLoop(C); continue; }

		/* this block is called exactly once for every potential contact created; it should check whether shapes
			should be swapped, and also set minDist00Sq if used (Sphere-Sphere only)
		*/
		if(unlikely(!C->isReal() && C->isFresh(scene))){
			bool swap=false;
			const shared_ptr<CGeomFunctor>& cgf=geoDisp->getFunctor2D(C->leakPA()->shape,C->leakPB()->shape,swap);
			if(!cgf) continue;
			if(swap){ C->swapOrder(); }
			cgf->setMinDist00Sq(C->pA.lock()->shape,C->pB.lock()->shape,C);
			CONTACTLOOP_CHECKPOINT("swap-check");
		}
		Particle *pA=C->leakPA(), *pB=C->leakPB();
		Vector3r shift2=(scene->isPeriodic?scene->cell->intrShiftPos(C->cellDist):Vector3r::Zero());
		// the order is as the geometry functor expects it
		shared_ptr<Shape>& sA(pA->shape); shared_ptr<Shape>& sB(pB->shape);

		// if minDist00Sq is defined, we might see that there is no contact without ever calling the functor
		// saving quite a few calls for sphere-sphere contacts
		if(likely(dist00 && !C->isReal() && !C->isFresh(scene) && C->minDist00Sq>0 && (sA->nodes[0]->pos-(sB->nodes[0]->pos+shift2)).squaredNorm()>C->minDist00Sq)){
			CONTACTLOOP_CHECKPOINT("dist00Sq-too-far");
			continue;
		}

		CONTACTLOOP_CHECKPOINT("pre-geom");

		bool geomCreated=geoDisp->operator()(sA,sB,shift2,/*force*/false,C);

		CONTACTLOOP_CHECKPOINT("geom");
		if(!geomCreated){
			if(/* has both geo and phy */C->isReal()) LOG_ERROR("CGeomFunctor "<<geoDisp->getClassName()<<" did not update existing contact ##"<<pA->id<<"+"<<pB->id);
			continue;
		}


		// CPhys
		if(!C->phys) C->stepCreated=scene->step;
		if(!C->phys || updatePhys) phyDisp->operator()(pA->material,pB->material,C);
		if(!C->phys) throw std::runtime_error("ContactLoop: ##"+to_string(pA->id)+"+"+to_string(pB->id)+": con Contact.phys created from materials "+pA->material->getClassName()+" and "+pB->material->getClassName()+" (a CPhysFunctor must be available for every contacting material combination).");

		CONTACTLOOP_CHECKPOINT("phys");

		// CLaw
		bool keepContact=lawDisp->operator()(C->geom,C->phys,C);
		if(!keepContact) dem.contacts->requestRemoval(C);
		CONTACTLOOP_CHECKPOINT("law");

		if(applyForces && C->isReal() && likely(!deterministic)){
			applyForceUninodal(C,pA);
			applyForceUninodal(C,pB);
			#if  0
			for(const Particle* particle:{pA,pB}){
				// remove once tested thoroughly
					const shared_ptr<Shape>& sh(particle->shape);
					if(!sh || sh->nodes.size()!=1) continue;
					// if(sh->nodes.size()!=1) continue;
					#if 0
						for(size_t i=0; i<sh->nodes.size(); i++){
							if((sh->nodes[i]->getData<DemData>().flags&DemData::DOF_ALL)!=DemData::DOF_ALL) LOG_WARN("Multinodal #"<<particle->id<<" has free DOFs, but force will not be applied; set ContactLoop.applyForces=False and use IntraForce(...) dispatcher instead.");
						}
					#endif
					Vector3r F,T,xc;
					std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/0,scene);
					sh->nodes[0]->getData<DemData>().addForceTorque(F,xc.cross(F)+T);
			}
			#endif
		}

		// track gradV work
		/* this is meant to avoid calling extra loop at every step, since the work must be evaluated incrementally */
		if(doStress && /*contact law deleted the contact?*/ C->isReal()){
			const auto& nnA(pA->shape->nodes); const auto& nnB(pB->shape->nodes);
			if(nnA.size()!=1 || nnB.size()!=1) throw std::runtime_error("ContactLoop.trackWork not allowed with multi-nodal particles in contact (##"+lexical_cast<string>(pA->id)+"+"+lexical_cast<string>(pB->id)+")");
			Vector3r branch=C->dPos(scene); // (nnB[0]->pos-nnA[0]->pos+scene->cell->intrShiftPos(C->cellDist));
			Vector3r F=C->geom->node->ori*C->phys->force; // force in global coords
			#ifdef WOO_OPENMP
				#pragma omp critical
			#endif
			{
				stress.noalias()+=F*branch.transpose();
			}
		}
		CONTACTLOOP_CHECKPOINT("force+stress");
	}
	// process removeAfterLoop
	#ifdef WOO_OPENMP
		for(list<shared_ptr<Contact>>& l: removeAfterLoopRefs){
			for(const shared_ptr<Contact>& c: l) dem.contacts->remove(c);
			l.clear();
		}
	#else
		for(const shared_ptr<Contact>& c: removeAfterLoopRefs) dem.contacts->remove(c);
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
	// apply forces deterministically, after the parallel loop
	// this could be also loop over particles in parallel (since per-particle loop would still be deterministic) but time per particle is very small here
	if(unlikely(deterministic) && applyForces){
		// non-paralell loop here
		for(const auto& C: *dem.contacts){
			if(!C->isReal()) continue;
			applyForceUninodal(C,C->leakPA());
			applyForceUninodal(C,C->leakPB());
		}
	}
	CONTACTLOOP_CHECKPOINT("epilogue");
}

void ContactLoop::applyForceUninodal(const shared_ptr<Contact>& C, const Particle* particle){
	const auto& sh(particle->shape);
	if(!sh || sh->nodes.size()!=1) return;
	Vector3r F,T,xc;
	std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/0,scene);
	sh->nodes[0]->getData<DemData>().addForceTorque(F,xc.cross(F)+T);
}
