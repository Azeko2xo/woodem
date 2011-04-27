#include<yade/pkg/common/Dispatching.hpp>
#include<yade/pkg/common/VelocityBins.hpp>


YADE_PLUGIN(dem,(BoundFunctor)(IGeomFunctor)(IPhysFunctor)(LawFunctor)(BoundDispatcher)(IGeomDispatcher)(IPhysDispatcher)(LawDispatcher));
BoundFunctor::~BoundFunctor(){};
IGeomFunctor::~IGeomFunctor(){};
IPhysFunctor::~IPhysFunctor(){};
LawFunctor::~LawFunctor(){};


/********************************************************************
                      BoundDispatcher
*********************************************************************/

CREATE_LOGGER(BoundDispatcher);
void BoundDispatcher::action()
{
	updateScenePtr();
	shared_ptr<BodyContainer>& bodies = scene->bodies;
	const long numBodies=(long)bodies->size();
	bool haveBins=(bool)velocityBins;
	if(sweepDist!=0 && haveBins){ LOG_FATAL("Only one of sweepDist or velocityBins can used!"); abort(); }
	//#pragma omp parallel for
	for(int id=0; id<numBodies; id++){
		if(!bodies->exists(id)) continue; // don't delete this check  - Janek
		const shared_ptr<Body>& b=(*bodies)[id];
		shared_ptr<Shape>& shape=b->shape;
		if(!shape || !b->isBounded()) continue;
		#ifdef BV_FUNCTOR_CACHE
			if(!shape->boundFunctor){ shape->boundFunctor=this->getFunctor1D(shape); if(!shape->boundFunctor) continue; }
			// LOG_DEBUG("shape->boundFunctor.get()=="<<shape->boundFunctor.get()<<" for "<<b->shape->getClassName()<<", #"<<id);
			//if(!shape->boundFunctor) throw runtime_error("boundFunctor not found for #"+lexical_cast<string>(id)); assert(shape->boundFunctor);
			shape->boundFunctor->go(shape,b->bound,b->state->se3,b.get());
		#else
			operator()(shape,b->bound,b->state->se3,b.get());
		#endif
		if(!b->bound) continue; // the functor did not create new bound
		if(sweepDist>0){
			Aabb* aabb=YADE_CAST<Aabb*>(b->bound.get());
			aabb->min-=Vector3r(sweepDist,sweepDist,sweepDist);
			aabb->max+=Vector3r(sweepDist,sweepDist,sweepDist);
		}
		if(haveBins){
			Aabb* aabb=YADE_CAST<Aabb*>(b->bound.get());
			Real sweep=velocityBins->bins[velocityBins->bodyBins[b->getId()]].maxDist;
			aabb->min-=Vector3r(sweep,sweep,sweep);
			aabb->max+=Vector3r(sweep,sweep,sweep);
		}
	}
	scene->updateBound();
}


/********************************************************************
                      IGeomDispatcher
*********************************************************************/

CREATE_LOGGER(IGeomDispatcher);

shared_ptr<Interaction> IGeomDispatcher::explicitAction(const shared_ptr<Body>& b1, const shared_ptr<Body>& b2, bool force){
	scene=Omega::instance().getScene().get(); // to make sure if called from outside of the loop
	Vector3i cellDist=Vector3i::Zero();
	if(scene->isPeriodic) {
		//throw logic_error("IGeomDispatcher::explicitAction does not support periodic boundary conditions (O.periodic==True)");
		for(int i=0; i<3; i++) cellDist[i]=-(int)((b2->state->pos[i]-b1->state->pos[i])/scene->cell->getSize()[i]+.5);
	}
	Vector3r shift2=scene->cell->hSize*cellDist.cast<Real>();
	updateScenePtr();
	if(force){
		assert(b1->shape && b2->shape);
		shared_ptr<Interaction> I(new Interaction(b1->getId(),b2->getId()));
		I->cellDist=cellDist;
		// FIXME: this code is more or less duplicated from InteractionLoop :-(
		bool swap=false;
		I->functorCache.geom=getFunctor2D(b1->shape,b2->shape,swap);
		if(!I->functorCache.geom) throw invalid_argument("IGeomDispatcher::explicitAction could not dispatch for given types ("+b1->shape->getClassName()+","+b2->shape->getClassName()+").");
		if(swap){I->swapOrder();}
		const shared_ptr<Body>& b1=Body::byId(I->getId1(),scene);
		const shared_ptr<Body>& b2=Body::byId(I->getId2(),scene);
		bool succ=I->functorCache.geom->go(b1->shape,b2->shape,*b1->state,*b2->state,shift2,/*force*/true,I);
		if(!succ) throw logic_error("Functor "+I->functorCache.geom->getClassName()+"::go returned false, even if asked to force IGeom creation. Please report bug.");
		return I;
	} else {
		shared_ptr<Interaction> I(new Interaction(b1->getId(),b2->getId()));
		I->cellDist=cellDist;
		b1->shape && b2->shape && operator()(b1->shape,b2->shape,*b1->state,*b2->state,shift2,/*force*/ false,I);
		return I;
	}
}

void IGeomDispatcher::action(){
	// Erase interaction that were requested for erase, but not processed by the collider, if any (and warn once about that, as it is suspicious)
	if(scene->interactions->unconditionalErasePending()>0 && !alreadyWarnedNoCollider){
		LOG_WARN("Interactions pending erase found, no collider being used?");
		alreadyWarnedNoCollider=true;
	}
	updateScenePtr();

	shared_ptr<BodyContainer>& bodies = scene->bodies;
	const bool isPeriodic(scene->isPeriodic);
	Matrix3r cellHsize; if(isPeriodic) cellHsize=scene->cell->hSize;
	bool removeUnseenIntrs=(scene->interactions->iterColliderLastRun>=0 && scene->interactions->iterColliderLastRun==scene->iter);
	#ifdef YADE_OPENMP
		const long size=scene->interactions->size();
		#pragma omp parallel for
		for(long i=0; i<size; i++){
			const shared_ptr<Interaction>& I=(*scene->interactions)[i];
	#else
		FOREACH(const shared_ptr<Interaction>& I, *scene->interactions){
	#endif
			if(removeUnseenIntrs && !I->isReal() && I->iterLastSeen<scene->iter) {
				scene->interactions->requestErase(I->getId1(),I->getId2());
				continue;
			}
			const shared_ptr<Body>& b1=(*bodies)[I->getId1()];
			const shared_ptr<Body>& b2=(*bodies)[I->getId2()];

			if(!b1 || !b2){ LOG_DEBUG("Body #"<<(b1?I->getId2():I->getId1())<<" vanished, erasing intr #"<<I->getId1()<<"+#"<<I->getId2()<<"!"); scene->interactions->requestErase(I->getId1(),I->getId2(),/*force*/true); continue; }

			bool wasReal=I->isReal();
			if (!b1->shape || !b2->shape) { assert(!wasReal); continue; } // some bodies do not have shape
			bool geomCreated;
			if(!isPeriodic){
				geomCreated=operator()(b1->shape, b2->shape, *b1->state, *b2->state, Vector3r::Zero(), /*force*/ false, I);
			} else{
				Vector3r shift2=cellHsize*I->cellDist.cast<Real>();
				geomCreated=operator()(b1->shape, b2->shape, *b1->state, *b2->state, shift2, /*force*/ false, I);
			}
			// reset && erase interaction that existed but now has no geometry anymore
			if(wasReal && !geomCreated){ scene->interactions->requestErase(I->getId1(),I->getId2()); }
	}
}

/********************************************************************
                      IPhysDispatcher
*********************************************************************/


void IPhysDispatcher::explicitAction(shared_ptr<Material>& pp1, shared_ptr<Material>& pp2, shared_ptr<Interaction>& I){
	updateScenePtr();
	if(!I->geom) throw invalid_argument(string(__FILE__)+": explicitAction received interaction without geom.");
	if(!I->functorCache.phys){
		bool dummy;
		I->functorCache.phys=getFunctor2D(pp1,pp2,dummy);
		if(!I->functorCache.phys) throw invalid_argument("IPhysDispatcher::explicitAction did not find a suitable dispatch for types "+pp1->getClassName()+" and "+pp2->getClassName());
		I->functorCache.phys->go(pp1,pp2,I);
	}
}

void IPhysDispatcher::action()
{
	updateScenePtr();
	shared_ptr<BodyContainer>& bodies = scene->bodies;
	#ifdef YADE_OPENMP
		const long size=scene->interactions->size();
		#pragma omp parallel for
		for(long i=0; i<size; i++){
			const shared_ptr<Interaction>& interaction=(*scene->interactions)[i];
	#else
		FOREACH(const shared_ptr<Interaction>& interaction, *scene->interactions){
	#endif
			if(interaction->geom){
				shared_ptr<Body>& b1 = (*bodies)[interaction->getId1()];
				shared_ptr<Body>& b2 = (*bodies)[interaction->getId2()];
				bool hadPhys=interaction->phys;
				operator()(b1->material, b2->material, interaction);
				assert(interaction->phys);
				if(!hadPhys) interaction->iterMadeReal=scene->iter;
			}
		}
}


/********************************************************************
                      LawDispatcher
*********************************************************************/

CREATE_LOGGER(LawDispatcher);
void LawDispatcher::action(){
	updateScenePtr();
	#ifdef YADE_OPENMP
		const long size=scene->interactions->size();
		#pragma omp parallel for
		for(long i=0; i<size; i++){
			const shared_ptr<Interaction>& I=(*scene->interactions)[i];
	#else
		FOREACH(shared_ptr<Interaction> I, *scene->interactions){
	#endif
		if(I->isReal()){
			assert(I->geom); assert(I->phys);
			operator()(I->geom,I->phys,I.get());
			if(!I->isReal() && I->isFresh(scene)) LOG_ERROR("Law functor deleted interaction that was just created. Please report bug: either this message is spurious, or the functor (or something else) is buggy.");
		}
	}
}

