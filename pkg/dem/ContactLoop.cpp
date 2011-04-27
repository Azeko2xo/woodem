#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>

YADE_PLUGIN(dem,(CGeom)(CPhys)(CGeomFunctor)(CGeomDispatcher)(CPhysFunctor)(CPhysDispatcher)(LawFunctor)(LawDispatcher)(ContactLoop));
CREATE_LOGGER(ContactLoop);

void ContactLoop::pyHandleCustomCtorArgs(python::tuple& t, python::dict& d){
	if(python::len(t)==0) return; // nothing to do
	if(python::len(t)!=3) throw invalid_argument("Exactly 3 lists of functors must be given");
	// parse custom arguments (3 lists) and do in-place modification of args
	typedef std::vector<shared_ptr<CGeomFunctor> > vecGeom;
	typedef std::vector<shared_ptr<CPhysFunctor> > vecPhys;
	typedef std::vector<shared_ptr<LawFunctor> > vecLaw;
	vecGeom vg=python::extract<vecGeom>(t[0])();
	vecPhys vp=python::extract<vecPhys>(t[1])();
	vecLaw vl=python::extract<vecLaw>(t[2])();
	FOREACH(shared_ptr<CGeomFunctor> gf, vg) this->geoDisp->add(gf);
	FOREACH(shared_ptr<CPhysFunctor> pf, vp) this->phyDisp->add(pf);
	FOREACH(shared_ptr<LawFunctor> cf, vl)   this->lawDisp->add(cf);
	t=python::tuple(); // empty the args; not sure if this is OK, as there is some refcounting in raw_constructor code
}

// #define IDISP_TIMING

#ifdef IDISP_TIMING
	#define IDISP_CHECKPOINT(cpt) timingDeltas->checkpoint(cpt)
#else
	#define IDISP_CHECKPOINT(cpt)
#endif

void ContactLoop::action(){
	#ifdef IDISP_TIMING
		timingDeltas->start();
	#endif

	field=dynamic_pointer_cast<DemField>(scene->field);
	if(!field) throw runtime_error("scene->field is not a DemField?!");

	/*
	if(scene->interactions->unconditionalErasePending()>0 && !alreadyWarnedNoCollider){
		LOG_WARN("Interactions pending erase found (erased), no collider being used?");
		alreadyWarnedNoCollider=true;
	}
	*/

	/*
	if(field->contactsDirty){
		throw std::logic_error("InteractionContainer::dirty is true; the collider should re-initialize in such case and clear the dirty flag.");
	}
	*/
	// update Scene* of the dispatchers
	geoDisp->scene=phyDisp->scene=lawDisp->scene=scene;
	// ask dispatchers to update Scene* of their functors
	geoDisp->updateScenePtr(); phyDisp->updateScenePtr(); lawDisp->updateScenePtr();

	// cache transformed cell size
	Matrix3r cellHsize; if(scene->isPeriodic) cellHsize=scene->cell->hSize;

	#if 0
	// force removal of interactions that were not encountered by the collider
	// (only for some kinds of colliders; see comment for InteractionContainer::iterColliderLastRun)
	bool removeUnseenIntrs=(scene->interactions->iterColliderLastRun>=0 && scene->interactions->iterColliderLastRun==scene->iter);
	#endif
		
	// TODO: REPLACE BY SOME MORE INTELLIGENT (and parallelizable) loop later
	FOREACH(const shared_ptr<Particle>&p, *(field->particles)){
		FOREACH(const Particle::MapParticleContact::value_type idC, p->contacts){
			const shared_ptr<Contact> C(idC.second);
			// skip the interaction where this particle is "B"
			// does not work well with swapping particles to the order the functor expects them
			if(C->pB.get()==p.get()) continue; 
			// therefore we check stepMadeReal as well
			// in the worst case, one contact will be uselessly processed twice, but only if there is no real contact
			if(C->stepMadeReal==scene->step) continue;

			#if 0
				if(unlikely(removeUnseenIntrs && !I->isReal() && I->iterLastSeen<scene->iter)) {
					eraseAfterLoop(I->getId1(),I->getId2());
					continue;
				}
			#endif

			shared_ptr<Shape>& sA(C->pA->shape);
			shared_ptr<Shape>& sB(C->pB->shape);

			bool swap=false;
			if(!C->isReal()){
				const shared_ptr<CGeomFunctor> cgf=geoDisp->getFunctor2D(sA,sB,swap);
				if(swap){ C->swapOrder(); }
				if(!cgf) continue;
			}
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
		}
	}
#if 0
	// process eraseAfterLoop
	#ifdef YADE_OPENMP
		FOREACH(list<idPair>& l, eraseAfterLoopIds){
			FOREACH(idPair p,l) scene->interactions->erase(p.first,p.second);
			l.clear();
		}
	#else
		FOREACH(idPair p, eraseAfterLoopIds) scene->interactions->erase(p.first,p.second);
		eraseAfterLoopIds.clear();
	#endif
#endif
}
