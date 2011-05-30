#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>

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

	// force removal of interactions that were not encountered by the collider
	// (only for some kinds of colliders; see comment for InteractionContainer::iterColliderLastRun)
	bool removeUnseen=(dem.contacts.stepColliderLastRun>=0 && dem.contacts.stepColliderLastRun==scene->step);
		
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
}
