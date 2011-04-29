#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>

YADE_PLUGIN(dem,(CGeomFunctor)(CGeomDispatcher)(CPhysFunctor)(CPhysDispatcher)(LawFunctor)(LawDispatcher)(ContactLoop));
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

	if(field->contacts.removeAllPending()>0 && !alreadyWarnedNoCollider){
		LOG_WARN("Interactions pending removal found (and were removed); no collider being used?");
		alreadyWarnedNoCollider=true;
	}

	if(field->contacts.dirty){
		throw std::logic_error("InteractionContainer::dirty is true; the collider should re-initialize in such case and clear the dirty flag.");
	}
	// update Scene* of the dispatchers
	geoDisp->scene=phyDisp->scene=lawDisp->scene=scene;
	// ask dispatchers to update Scene* of their functors
	geoDisp->updateScenePtr(); phyDisp->updateScenePtr(); lawDisp->updateScenePtr();

	// cache transformed cell size
	Matrix3r cellHsize; if(scene->isPeriodic) cellHsize=scene->cell->hSize;

	// force removal of interactions that were not encountered by the collider
	// (only for some kinds of colliders; see comment for InteractionContainer::iterColliderLastRun)
	bool removeUnseen=(field->contacts.stepColliderLastRun>=0 && field->contacts.stepColliderLastRun==scene->step);
		
	size_t size=field->contacts.size();
	#ifdef YADE_OPENMP
		#pragma omp parallel for schedule(guided)
	#endif
	for(size_t i=0; i<size; i++){
		const shared_ptr<Contact>& C=field->contacts[i];

		if(unlikely(removeUnseen && !C->isReal() && C->stepLastSeen<scene->step)) { removeAfterLoop(C); continue; }

		shared_ptr<Shape>& sA(C->pA->shape); shared_ptr<Shape>& sB(C->pB->shape);

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
	// process removeAfterLoop
	#ifdef YADE_OPENMP
		FOREACH(list<shared_ptr<Contact> >& l, removeAfterLoopRefs){
			FOREACH(const shared_ptr<Contact>& c,l) field->contacts.remove(c);
			l.clear();
		}
	#else
		FOREACH(const shared_ptr<Contact>& c, removeAfterLoopRefs) field->contacts.remove(c);
		removeAfterLoopRefs.clear();
	#endif
}
