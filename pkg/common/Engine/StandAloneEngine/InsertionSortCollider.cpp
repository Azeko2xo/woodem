// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#include"InsertionSortCollider.hpp"
#include<yade/core/World.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/core/InteractionContainer.hpp>
#include<yade/pkg-common/BoundingVolumeDispatcher.hpp>
#include<yade/pkg-common/VelocityBins.hpp>
#include<yade/pkg-dem/NewtonsDampedLaw.hpp>

#include<algorithm>
#include<vector>
#include<boost/static_assert.hpp>

using namespace std;

YADE_PLUGIN((InsertionSortCollider))
CREATE_LOGGER(InsertionSortCollider);

// return true if bodies bb overlap in all 3 dimensions
bool InsertionSortCollider::spatialOverlap(body_id_t id1, body_id_t id2) const {
	assert(!periodic);
	return
		(minima[3*id1+0]<=maxima[3*id2+0]) && (maxima[3*id1+0]>=minima[3*id2+0]) &&
		(minima[3*id1+1]<=maxima[3*id2+1]) && (maxima[3*id1+1]>=minima[3*id2+1]) &&
		(minima[3*id1+2]<=maxima[3*id2+2]) && (maxima[3*id1+2]>=minima[3*id2+2]);
}

// called by the insertion sort if 2 bodies swapped their bounds
void InsertionSortCollider::handleBoundInversion(body_id_t id1, body_id_t id2, InteractionContainer* interactions, World* rb){
	assert(!periodic);
	assert(id1!=id2);
	// do bboxes overlap in all 3 dimensions?
	bool overlap=spatialOverlap(id1,id2);
	// existing interaction?
	const shared_ptr<Interaction>& I=interactions->find(id1,id2);
	bool hasInter=(bool)I;
	// interaction doesn't exist and shouldn't, or it exists and should
	if(!overlap && !hasInter) return;
	if(overlap && hasInter){  return; }
	// create interaction if not yet existing
	if(overlap && !hasInter){ // second condition only for readability
		if(!Collider::mayCollide(Body::byId(id1,rb).get(),Body::byId(id2,rb).get())) return;
		// LOG_TRACE("Creating new interaction #"<<id1<<"+#"<<id2);
		shared_ptr<Interaction> newI=shared_ptr<Interaction>(new Interaction(id1,id2));
		interactions->insert(newI);
		return;
	}
	if(!overlap && hasInter){ if(!I->isReal()) interactions->erase(id1,id2); return; }
	assert(false); // unreachable
}

void InsertionSortCollider::insertionSort(VecBounds& v, InteractionContainer* interactions, World* rb, bool doCollide){
	assert(!periodic);
	assert(v.size==(long)v.vec.size());
	for(long i=0; i<v.size; i++){
		const Bound viInit=v[i]; long j=i-1; /* cache hasBB; otherwise 1% overall performance hit */ const bool viInitBB=viInit.flags.hasBB;
		while(j>=0 && v[j]>viInit){
			v[j+1]=v[j];
			// no collisions without bounding boxes
			if(doCollide && viInitBB && v[j].flags.hasBB) handleBoundInversion(viInit.id,v[j].id,interactions,rb);
			j--;
		}
		v[j+1]=viInit;
	}
}

vector<body_id_t> InsertionSortCollider::probeBoundingVolume(const BoundingVolume& bv){
	if(periodic){ LOG_FATAL("Probing with periodic boundary not implemented."); abort(); }
	vector<body_id_t> ret;
	for( vector<Bound>::iterator 
			it=BB[0].vec.begin(),et=BB[0].vec.end(); it < et; ++it)
	{
		if (it->coord > bv.max[0]) break;
		if (!it->flags.isMin || !it->flags.hasBB) continue;
		int offset = 3*it->id;
		if (!(maxima[offset] < bv.min[0] ||
				minima[offset+1] > bv.max[1] ||
				maxima[offset+1] < bv.min[1] ||
				minima[offset+2] > bv.max[2] ||
				maxima[offset+2] < bv.min[2] )) 
		{
			ret.push_back(it->id);
		}
	}
	return ret;
}

#ifdef COLLIDE_STRIDED
	bool InsertionSortCollider::isActivated(World* rb){
		// activated if number of bodies changes (hence need to refresh collision information)
		// or the time of scheduled run already came, or we were never scheduled yet
		if(!strideActive) return true;
		if(!newton || (nBins>=1 && !newton->velocityBins)) return true;
		if(nBins>=1 && newton->velocityBins->incrementDists_shouldCollide(rb->dt)) return true;
		if(nBins<=0){
			if(fastestBodyMaxDist<0){fastestBodyMaxDist=0; return true;}
			fastestBodyMaxDist+=sqrt(newton->maxVelocitySq)*rb->dt;
			if(fastestBodyMaxDist>=sweepLength) return true;
		}
		if(BB[0].size!=2*rb->bodies->size()) return true;
		// we wouldn't run in this step; in that case, just delete pending interactions
		// this is done in ::action normally, but it would make the call counters not reflect the stride
		rb->interactions->erasePending(*this,rb);
		return false;
	}
#endif

void InsertionSortCollider::action(World* rb){
	#ifdef ISC_TIMING
		timingDeltas->start();
	#endif

	long nBodies=(long)rb->bodies->size();
	InteractionContainer* interactions=rb->interactions.get();
	rb->interactions->iterColliderLastRun=-1;

	// periodicity changed, force reinit
	if(rb->isPeriodic != periodic){
		for(int i=0; i<3; i++) BB[i].vec.clear();
		periodic=rb->isPeriodic;
	}


	// pre-conditions
		// adjust storage size
		bool doInitSort=false;
		if(BB[0].size!=2*nBodies){
			long BBsize=BB[0].size;
			LOG_DEBUG("Resize bounds containers from "<<BBsize<<" to "<<nBodies*2<<", will std::sort.");
			// bodies deleted; clear the container completely, and do as if all bodies were added (rather slow…)
			// future possibility: insertion sort with such operator that deleted bodies would all go to the end, then just trim bounds
			if(2*nBodies<BBsize){ for(int i=0; i<3; i++) BB[i].vec.clear(); }
			// more than 100 bodies was added, do initial sort again
			// maybe: should rather depend on ratio of added bodies to those already present...?
			if(2*nBodies-BBsize>200 || BBsize==0) doInitSort=true;
			assert((BBsize%2)==0);
			for(int i=0; i<3; i++){
				BB[i].vec.reserve(2*nBodies);
				// add lower and upper bounds; coord is not important, will be updated from bb shortly
				for(long id=BBsize/2; id<nBodies; id++){ BB[i].vec.push_back(Bound(0,id,/*isMin=*/true)); BB[i].vec.push_back(Bound(0,id,/*isMin=*/false)); }
				BB[i].size=BB[i].vec.size();
			}
		}
		if(minima.size()!=(size_t)3*nBodies){ minima.resize(3*nBodies); maxima.resize(3*nBodies); }
		assert(BB[0].size==2*rb->bodies->size());

		// update periodicity
		assert(BB[0].axis==0); assert(BB[1].axis==1); assert(BB[2].axis==2);
		if(periodic) for(int i=0; i<3; i++) BB[i].updatePeriodicity(rb); 

		#ifdef COLLIDE_STRIDED
			// get the BoundingVolumeDispatcher and turn it off; we will call it ourselves
			if(!boundDispatcher){
				FOREACH(shared_ptr<Engine>& e, rb->engines){ boundDispatcher=dynamic_pointer_cast<BoundingVolumeDispatcher>(e); if(boundDispatcher) break; }
				if(!boundDispatcher){ LOG_FATAL("Unable to locate BoundingVolumeDispatcher within engines, aborting."); abort(); }
				boundDispatcher->activated=false; // deactive the engine, we will call it ourselves from now (just when needed)
			}
			if(sweepLength>0){
				// get NewtonsDampedLaw, to ask for the maximum velocity value
				if(!newton){
					FOREACH(shared_ptr<Engine>& e, rb->engines){ newton=dynamic_pointer_cast<NewtonsDampedLaw>(e); if(newton) break; }
					if(!newton){ LOG_FATAL("Unable to locate NewtonsDampedLaw within engines, aborting."); abort(); }
				}
			}
		#endif
	ISC_CHECKPOINT("init");

		#ifdef COLLIDE_STRIDED
			// get us ready for strides, if they were deactivated
			if(!strideActive && sweepLength>0 && newton->maxVelocitySq>=0){ // maxVelocitySq is a really computed value
				strideActive=true;
			}
			if(strideActive){
				assert(sweepLength>0);
				if(nBins<=0){
					// reset bins, in case they were active but are not anymore
					if(newton->velocityBins) newton->velocityBins=shared_ptr<VelocityBins>(); if(boundDispatcher->velocityBins) boundDispatcher->velocityBins=shared_ptr<VelocityBins>();
					assert(strideActive); assert(newton->maxVelocitySq>=0); assert(sweepFactor>1.);
					Real sweepVelocity=sqrt(newton->maxVelocitySq)*sweepFactor; int stride=-1;
					if(sweepVelocity>0) {
						stride=max(1,int((sweepLength/sweepVelocity)/rb->dt));
						boundDispatcher->sweepDist=rb->dt*(stride-1)*sweepVelocity;
					} else { // no motion
						boundDispatcher->sweepDist=0; // nothing moves, no need to make bboxes larger
					}
					LOG_DEBUG(rb->simulationTime<<"s: stride ≈"<<stride<<"; maxVelocity="<<sqrt(newton->maxVelocitySq)<<", sweepDist="<<boundDispatcher->sweepDist);
					fastestBodyMaxDist=0; // reset
				} else { // nBins>=1
					if(!newton->velocityBins){ newton->velocityBins=shared_ptr<VelocityBins>(new VelocityBins(nBins,newton->maxVelocitySq,binCoeff,binOverlap)); }
					if(!boundDispatcher->velocityBins) boundDispatcher->velocityBins=newton->velocityBins;
					// update things:
					newton->velocityBins->nBins=nBins; newton->velocityBins->binCoeff=binCoeff; newton->velocityBins->binOverlap=binOverlap; newton->velocityBins->maxRefRelStep=maxRefRelStep; newton->velocityBins->histInterval=histInterval;  
					boundDispatcher->sweepDist=0; // not used with bins at all
					// re-bin bodies
					newton->velocityBins->setBins(rb,newton->maxVelocitySq,sweepLength);
				}
			}
			boundDispatcher->action(rb);
		#endif

	ISC_CHECKPOINT("bound");

	// copy bounds along given axis into our arrays
		for(long i=0; i<2*nBodies; i++){
			for(int j=0; j<3; j++){
				VecBounds& BBj=BB[j];
				const body_id_t id=BBj[i].id;
				const shared_ptr<Body>& b=Body::byId(id,rb);
				if(b){
					const shared_ptr<BoundingVolume>& bv=b->boundingVolume;
					// coordinate is min/max if has bounding volume, otherwise both are the position. Add periodic shift so that we are inside the cell
					// watch out for the parentheses around ?: within ?: (there was unwanted conversion of the Reals to bools!)
					BBj[i].coord=((BBj[i].flags.hasBB=((bool)bv)) ? (BBj[i].flags.isMin ? bv->min[j] : bv->max[j]) : (b->state->pos[j])) - (periodic ? BBj.cellDim*BBj[i].period : 0.);
				} else { BBj[i].flags.hasBB=false; /* for vanished body, keep the coordinate as-is, to minimize inversions. */ }
				// if initializing periodic, shift coords & record the period into BBj[i].period
				if(doInitSort && periodic) BBj[i].coord=cellWrap(BBj[i].coord,BBj.cellMin,BBj.cellMax,BBj[i].period);
			}	
		}
	// for each body, copy its minima and maxima, for quick checks of overlaps later
	for(body_id_t id=0; id<nBodies; id++){
		BOOST_STATIC_ASSERT(sizeof(Vector3r)==3*sizeof(Real));
		const shared_ptr<Body>& b=Body::byId(id,rb);
		if(b){
			const shared_ptr<BoundingVolume>& bv=b->boundingVolume;
			if(bv) { memcpy(&minima[3*id],&bv->min,3*sizeof(Real)); memcpy(&maxima[3*id],&bv->max,3*sizeof(Real)); } // ⇐ faster than 6 assignments 
			else{ const Vector3r& pos=b->state->pos; memcpy(&minima[3*id],pos,3*sizeof(Real)); memcpy(&maxima[3*id],pos,3*sizeof(Real)); }
		} else { memset(&minima[3*id],0,3*sizeof(Real)); memset(&maxima[3*id],0,3*sizeof(Real)); }
	}

	ISC_CHECKPOINT("copy");

	// process interactions that the constitutive law asked to be erased
	interactions->erasePending(*this,rb);
	ISC_CHECKPOINT("erase");

	// sort
		// the regular case
		if(!doInitSort && !sortThenCollide){
			/* each inversion in insertionSort calls handleBoundInversion, which in turns may add/remove interaction */
			if(!periodic) for(int i=0; i<3; i++) insertionSort(BB[i],interactions,rb); 
			else for(int i=0; i<3; i++) insertionSortPeri(BB[i],interactions,rb);
		}
		// create initial interactions (much slower)
		else {
			if(doInitSort){
				// the initial sort is in independent in 3 dimensions, may be run in parallel; it seems that there is no time gain running in parallel, though
				for(int i=0; i<3; i++) 	std::sort(BB[i].vec.begin(),BB[i].vec.end());
			} else { // sortThenCollide
				if(!periodic) for(int i=0; i<3; i++) insertionSort(BB[i],interactions,rb,false);
				else for(int i=0; i<3; i++) insertionSortPeri(BB[i],interactions,rb,false);
			}
			// traverse the container along requested axis
			assert(sortAxis==0 || sortAxis==1 || sortAxis==2);
			VecBounds& V=BB[sortAxis];
			// go through potential aabb collisions, create interactions as necessary
			if(!periodic){
				for(long i=0; i<2*nBodies; i++){
					// start from the lower bound (i.e. skipping upper bounds)
					// skip bodies without bbox, because they don't collide
					if(!(V[i].flags.isMin && V[i].flags.hasBB)) continue;
					const body_id_t& iid=V[i].id;
					// go up until we meet the upper bound
					for(long j=i+1; /* handle case 2. of swapped min/max */ j<2*nBodies && V[j].id!=iid; j++){
						const body_id_t& jid=V[j].id;
						// take 2 of the same condition (only handle collision [min_i..max_i]+min_j, not [min_i..max_i]+min_i (symmetric)
						if(!V[j].flags.isMin) continue;
						/* abuse the same function here; since it does spatial overlap check first, it is OK to use it */
						handleBoundInversion(iid,jid,interactions,rb);
						assert(j<2*nBodies-1);
					}
				}
			} else { // periodic case: see comments above
				for(long i=0; i<2*nBodies; i++){
					if(!(V[i].flags.isMin && V[i].flags.hasBB)) continue;
					const body_id_t& iid=V[i].id;
					long cnt=0;
					// we might wrap over the periodi boundary here; that's why the condition is difference from the aperiodic case
					for(long j=V.norm(i+1); V[j].id!=iid; j=V.norm(j+1)){
						const body_id_t& jid=V[j].id;
						if(!V[j].flags.isMin) continue;
						handleBoundInversionPeri(iid,jid,interactions,rb);
						if(cnt++>2*(long)nBodies){ LOG_FATAL("Uninterrupted loop in the initial sort?"); throw std::logic_error("loop??"); }
					}
				}
			}
		}
	ISC_CHECKPOINT("sort&collide");
}


// return floating value wrapped between x0 and x1 and saving period number to period
Real InsertionSortCollider::cellWrap(const Real x, const Real x0, const Real x1, int& period){
	Real xNorm=(x-x0)/(x1-x0);
	period=(int)floor(xNorm); // some people say this is very slow; probably optimized by gcc, however (google suggests)
	return x0+(xNorm-period)*(x1-x0);
}

// return coordinate wrapped to x0…x1, relative to x0; don't care about period
Real InsertionSortCollider::cellWrapRel(const Real x, const Real x0, const Real x1){
	Real xNorm=(x-x0)/(x1-x0);
	return (xNorm-floor(xNorm))*(x1-x0);
}

void InsertionSortCollider::insertionSortPeri(VecBounds& v, InteractionContainer* interactions, World*rb, bool doCollide){
	assert(periodic);
	long &loIdx=v.loIdx; const long &size=v.size;
	for(long _i=0; _i<size; _i++){
		const long i=v.norm(_i);
		const long i_1=v.norm(i-1);
		//switch period of (i) if the coord is below the lower edge cooridnate-wise and just above the split
		if(i==loIdx && v[i].coord<v.cellMin){ v[i].period-=1; v[i].coord+=v.cellDim; loIdx=v.norm(loIdx+1); }
		// coordinate of v[i] used to check inversions
		// if crossing the split, adjust by cellDim;
		// if we get below the loIdx however, the v[i].coord will have been adjusted already, no need to do that here
		const Real iCmpCoord=v[i].coord+(i==loIdx ? v.cellDim : 0); 
		// no inversion
		if(v[i_1].coord<=iCmpCoord) continue;
		// vi is the copy that will travel down the list, while other elts go up
		// if will be placed in the list only at the end, to avoid extra copying
		int j=i_1; Bound vi=v[i];  const bool viHasBB=vi.flags.hasBB;
		while(v[j].coord>vi.coord + /* wrap for elt just below split */ (v.norm(j+1)==loIdx ? v.cellDim : 0)){
			long j1=v.norm(j+1);
			// OK, now if many bodies move at the same pace through the cell and at one point, there is inversion,
			// this can happen without any side-effects
			if (false && v[j].coord>v.cellMax+v.cellDim){
				// this condition is not strictly necessary, but the loop of insertionSort would have to run more times.
				// Since size of particle is required to be < .5*cellDim, this would mean simulation explosion anyway
				LOG_FATAL("Body #"<<v[j].id<<" going faster than 1 cell in one step? Not handled.");
				throw runtime_error(__FILE__ ": body mmoving too fast (skipped 1 cell).");
			}
			Bound& vNew(v[j1]); // elt at j+1 being overwritten by the one at j and adjusted
			vNew=v[j];
			// inversions close the the split need special care
			if(j==loIdx && vi.coord<v.cellMin) { vi.period-=1; vi.coord+=v.cellDim; loIdx=v.norm(loIdx+1); }
			else if(j1==loIdx) { vNew.period+=1; vNew.coord-=v.cellDim; loIdx=v.norm(loIdx-1); }
			if(doCollide && viHasBB && v[j].flags.hasBB){
				if(vi.id==vNew.id){ // BUG!!
					LOG_FATAL("Inversion of body's #"<<vi.id<<" boundary with its other boundary, "<<v[j].coord<<" meets "<<vi.coord);
					throw runtime_error(__FILE__ ": Body's boundary metting its opposite boundary.");
				}
				handleBoundInversionPeri(vi.id,vNew.id,interactions,rb);
			}
			j=v.norm(j-1);
		}
		v[v.norm(j+1)]=vi;
	}
}

// called by the insertion sort if 2 bodies swapped their bounds
void InsertionSortCollider::handleBoundInversionPeri(body_id_t id1, body_id_t id2, InteractionContainer* interactions, World* rb){
	assert(periodic);
	// do bboxes overlap in all 3 dimensions?
	Vector3<int> periods;
	bool overlap=spatialOverlapPeri(id1,id2,rb,periods);
	// existing interaction?
	const shared_ptr<Interaction>& I=interactions->find(id1,id2);
	bool hasInter=(bool)I;
	#ifdef PISC_DEBUG
		if(watchIds(id1,id2)) LOG_INFO("Inversion #"<<id1<<"+#"<<id2<<", overlap=="<<overlap<<", hasInter=="<<hasInter);
	#endif
	// interaction doesn't exist and shouldn't, or it exists and should
	if(!overlap && !hasInter) return;
	if(overlap && hasInter){  return; }
	// create interaction if not yet existing
	if(overlap && !hasInter){ // second condition only for readability
		#ifdef PISC_DEBUG
			if(watchIds(id1,id2)) LOG_INFO("Attemtping collision of #"<<id1<<"+#"<<id2);
		#endif
		if(!Collider::mayCollide(Body::byId(id1,rb).get(),Body::byId(id2,rb).get())) return;
		// LOG_TRACE("Creating new interaction #"<<id1<<"+#"<<id2);
		shared_ptr<Interaction> newI=shared_ptr<Interaction>(new Interaction(id1,id2));
		newI->cellDist=periods;
		#ifdef PISC_DEBUG
			if(watchIds(id1,id2)) LOG_INFO("Created intr #"<<id1<<"+#"<<id2<<", periods="<<periods);
		#endif
		interactions->insert(newI);
		return;
	}
	if(!overlap && hasInter){ if(!I->isReal()) interactions->erase(id1,id2); return; }
	assert(false); // unreachable
}

/* Performance hint
	================

	Since this function is called (most the time) from insertionSort,
	we actually already know what is the overlap status in that one dimension, including
	periodicity information; both values could be passed down as a parameters, avoiding 1 of 3 loops here.
	We do some floats math here, so the speedup could noticeable; doesn't concertn the non-periodic variant,
	where it is only plain comparisons taking place.

	In the same way, handleBoundInversion is passed only id1 and id2, but again insertionSort already knows in which sense
	the inversion happens; if the boundaries get apart (min getting up over max), it wouldn't have to check for overlap
	at all, for instance.
*/
//! return true if bodies bb overlap in all 3 dimensions
bool InsertionSortCollider::spatialOverlapPeri(body_id_t id1, body_id_t id2,World* rb, Vector3<int>& periods) const {
	assert(periodic);
	assert(id1!=id2); // programming error, or weird bodies (too large?)
	for(int axis=0; axis<3; axis++){
		Real dim=rb->cellMax[axis]-rb->cellMin[axis];
		// too big bodies in interaction
		assert(maxima[3*id1+axis]-minima[3*id1+axis]<.99*dim); assert(maxima[3*id2+axis]-minima[3*id2+axis]<.99*dim);
		// find body of which when taken as period start will make the gap smaller
		Real m1=minima[3*id1+axis],m2=minima[3*id2+axis];
		Real wMn=(cellWrapRel(m1,m2,m2+dim)<cellWrapRel(m2,m1,m1+dim)) ? m2 : m1;
		#ifdef PISC_DEBUG
		if(watchIds(id1,id2)){
			TRVAR3(id1,id2,axis);
			TRVAR4(minima[3*id1+axis],maxima[3*id1+axis],minima[3*id2+axis],maxima[3*id2+axis]);
			TRVAR2(cellWrapRel(m1,m2,m2+dim),cellWrapRel(m2,m1,m1+dim));
			TRVAR3(m1,m2,wMn);
		}
		#endif
		int pmn1,pmx1,pmn2,pmx2;
		Real mn1=cellWrap(minima[3*id1+axis],wMn,wMn+dim,pmn1), mx1=cellWrap(maxima[3*id1+axis],wMn,wMn+dim,pmx1);
		Real mn2=cellWrap(minima[3*id2+axis],wMn,wMn+dim,pmn2), mx2=cellWrap(maxima[3*id2+axis],wMn,wMn+dim,pmx2);
		#ifdef PISC_DEBUG
			if(watchIds(id1,id2)){
				TRVAR4(mn1,mx1,mn2,mx2);
				TRVAR4(pmn1,pmx1,pmn2,pmx2);
			}
		#endif
		if((pmn1!=pmx1) || (pmn2!=pmx2)){
			LOG_FATAL("Body #"<<(pmn1!=pmx1?id1:id2)<<" spans over half of the cell size "<<dim<<" (axis="<<axis<<", min="<<(pmn1!=pmx1?mn1:mn2)<<", max="<<(pmn1!=mn1?mx1:mx2)<<")");
			throw runtime_error(__FILE__ ": Body larger than half of the cell size encountered.");
		}
		periods[axis]=(int)(pmn1-pmn2);
		if(!(mn1<=mx2 && mx1 >= mn2)) return false;
	}
	#ifdef PISC_DEBUG
		if(watchIds(id1,id2)) LOG_INFO("Overlap #"<<id1<<"+#"<<id2<<", periods "<<periods);
	#endif
	return true;
}

