// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#include<woo/pkg/dem/InsertionSortCollider.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/core/Scene.hpp>

#include<algorithm>
#include<vector>
#include<boost/static_assert.hpp>

WOO_PLUGIN(dem,(InsertionSortCollider));
CREATE_LOGGER(InsertionSortCollider);


// return true if bodies bb overlap in all 3 dimensions
bool InsertionSortCollider::spatialOverlap(Particle::id_t id1, Particle::id_t id2) const {
	assert(!periodic);
	return
		(minima[3*id1+0]<=maxima[3*id2+0]) && (maxima[3*id1+0]>=minima[3*id2+0]) &&
		(minima[3*id1+1]<=maxima[3*id2+1]) && (maxima[3*id1+1]>=minima[3*id2+1]) &&
		(minima[3*id1+2]<=maxima[3*id2+2]) && (maxima[3*id1+2]>=minima[3*id2+2]);
}

// called by the insertion sort if 2 bodies swapped their bounds
void InsertionSortCollider::handleBoundInversion(Particle::id_t id1, Particle::id_t id2){
	assert(!periodic);
	assert(id1!=id2);
	// do bboxes overlap in all 3 dimensions?
	bool overlap=spatialOverlap(id1,id2);
	// existing interaction?
	const shared_ptr<Contact>& C=dem->contacts->find(id1,id2);
	bool hasInter=(bool)C;
	// interaction doesn't exist and shouldn't, or it exists and should
	if(likely(!overlap && !hasInter)) return;
	if(overlap && hasInter){  return; }
	// create interaction if not yet existing
	if(overlap && !hasInter){ // second condition only for readability
		const shared_ptr<Particle>& p1((*particles)[id1]);
		const shared_ptr<Particle>& p2((*particles)[id2]);
		if(!Collider::mayCollide(p1,p2,dem)) return;
		// LOG_TRACE("Creating new interaction #"<<id1<<"+#"<<id2);
		shared_ptr<Contact> newC=make_shared<Contact>();
		// mimick the way clDem::Collider does the job so that results are easily comparable
		if(id1<id2){ newC->pA=p1; newC->pB=p2; }
		else{ newC->pA=p2; newC->pB=p1; }
		dem->contacts->add(newC);
		return;
	}
	if(!overlap && hasInter){ if(!C->isReal()) dem->contacts->remove(C); return; }
	assert(false); // unreachable
}

void InsertionSortCollider::insertionSort(VecBounds& v, bool doCollide, int ax){
	assert(!periodic);
	assert(v.size==(long)v.vec.size());
	for(long i=0; i<v.size; i++){
		const Bounds viInit=v[i]; long j=i-1; /* cache hasBB; otherwise 1% overall performance hit */ const bool viInitBB=viInit.flags.hasBB;
		while(j>=0 && v[j]>viInit){
			v[j+1]=v[j];
			#ifdef PISC_DEBUG
				if(watchIds(v[j].id,viInit.id)) cerr<<"Swapping #"<<v[j].id<<"  with #"<<viInit.id<<" ("<<setprecision(80)<<v[j].coord<<">"<<setprecision(80)<<viInit.coord<<" along axis "<<v.axis<<")"<<endl;
				if(v[j].id==viInit.id){ cerr<<"Inversion of #"<<v[j].id<<" with itself, "<<v[j].flags.isMin<<" & "<<viInit.flags.isMin<<", isGreater "<<(v[j]>viInit)<<", "<<(v[j].coord>viInit.coord)<<endl; j--; continue; }
			#endif
			#ifdef WOO_DEBUG
				stepInvs[ax]++; numInvs[ax]++;
			#endif
			// no collisions without bounding boxes
			// also, do not collide particle with itself; it sometimes happens for facets aligned perpendicular to an axis, for reasons that are not very clear
			// see https://bugs.launchpad.net/woo/+bug/669095
			if(likely(doCollide && viInitBB && v[j].flags.hasBB && (viInit.id!=v[j].id))){
				handleBoundInversion(viInit.id,v[j].id);
			}
			j--;
		}
		v[j+1]=viInit;
	}
}

Vector3i InsertionSortCollider::countInversions(){
	Vector3i ret;
	for(int ax:{0,1,2}){
		int N=0;
		const VecBounds& v(BB[ax]);
		for(long i=0; i<v.size-1; i++){
			for(long j=i+1; j<v.size; j++){
				if(v[i]>v[j]){
					N++;
				}
			}
		}
		ret[ax]=N;
	}
	return ret;
}

// if(verletDist>0){ mn-=verletDist*Vector3r::Ones(); mx+=verletDist*Vector3r::Ones(); }
vector<Particle::id_t> InsertionSortCollider::probeAabb(const Vector3r& mn, const Vector3r& mx){
	vector<Particle::id_t> ret;
	const short ax0=0; // use the x-axis for the traversal
	const VecBounds& v(BB[ax0]);
	if(!periodic){
		#if 0
			auto I=std::lower_bound(v.vec.begin(),v.vec.end(),mn[ax0],[](const Bounds& b, const Real& c)->bool{ return b.coord<c; } );
			#ifdef WOO_DEBUG
				long i=I-v.vec.begin();
				long pi=max(0L,i-1), ppi=max(0L,i-2), ni=min((long)v.vec.size(),i+1), nni=min((long)v.vec.size(),i+2);
				cerr<<"x="<<mn[ax0]<<":: b["<<ppi<<"]="<<v.vec[ppi].coord<<", b["<<pi<<"]="<<v.vec[pi].coord<<" || "<<v.vec[i].coord<<" || b["<<ni<<"]="<<v.vec[ni].coord<<", b["<<nni<<"]="<<v.vec[nni].coord<<endl;
			#endif
		#endif
		for(long i=0; i<v.size; i++){
			const Bounds& b(v.vec[i]);
			if(!b.flags.isMin || !b.flags.hasBB) continue;
			if(b.coord>mx[ax0]) break;
			long off=3*b.id;
			//Particle::id_t id2=I->id;
			bool overlap=
				(mn[0]<=maxima[off+0]) && (mx[0]>=minima[off+0]) &&
				(mn[1]<=maxima[off+1]) && (mx[1]>=minima[off+1]) &&
				(mn[2]<=maxima[off+2]) && (mx[2]>=minima[off+2]);
			if(overlap) ret.push_back(b.id);
		}
		return ret;
	} else {
		// for the periodic case, go through all particles
		// algorithmically wasteful :|
		// copied from handleBoundInversionPeri, with adjustments
		for(long i=0; i<v.size; i++){
			// safely skip deleted particles to avoid spurious overlaps
			const Bounds& b(v.vec[i]);
			if(!b.flags.isMin || !b.flags.hasBB) continue;
			long id1=b.id;
			for(int axis=0; axis<3; axis++){
				Real dim=scene->cell->getSize()[axis];
				// find particle of which minimum when taken as period start will make the gap smaller
				Real m1=minima[3*id1+axis],m2=mn[axis];
				Real wMn=(cellWrapRel(m1,m2,m2+dim)<cellWrapRel(m2,m1,m1+dim)) ? m2 : m1;
				int pmn1,pmx1,pmn2,pmx2;
				Real mn1=cellWrap(minima[3*id1+axis],wMn,wMn+dim,pmn1), mx1=cellWrap(maxima[3*id1+axis],wMn,wMn+dim,pmx1);
				Real mn2=cellWrap(mn[axis],wMn,wMn+dim,pmn2), mx2=cellWrap(mx[axis],wMn,wMn+dim,pmx2);
				if(unlikely((pmn1!=pmx1) || (pmn2!=pmx2))){
					Real span=(pmn1!=pmx1?mx1-mn1:mx2-mn2); if(span<0) span=dim-span;
					LOG_FATAL("Particle #"<<(pmn1!=pmx1?id1:-1)<<" spans over half of the cell size "<<dim<<" (axis="<<axis<<", min="<<(pmn1!=pmx1?mn1:mn2)<<", max="<<(pmn1!=pmx1?mx1:mx2)<<", span="<<span<<")");
					throw runtime_error(__FILE__ ": Particle larger than half of the cell size encountered.");
				}
				if(!(mn1<=mx2 && mx1 >= mn2)) goto noOverlap;
			}
			ret.push_back(id1);
			noOverlap: ;
		}
		return ret;
	}
};

// STRIDE
	bool InsertionSortCollider::isActivated(){
		// we wouldn't run in this step; in that case, just delete pending interactions
		// this is done in ::action normally, but it would make the call counters not reflect the stride
		return true;
	}


bool InsertionSortCollider::updateBboxes_doFullRun(){
	// update bounds via boundDispatcher
	boundDispatcher->scene=scene;
	boundDispatcher->field=field;
	boundDispatcher->updateScenePtr();
	// boundDispatcher->run();

	// automatically initialize from min sphere size; if no spheres, disable stride
	if(verletDist<0){
		Real minR=std::numeric_limits<Real>::infinity();
		FOREACH(const shared_ptr<Particle>& p, *dem->particles){
			if(!p || !p->shape) continue;
			if(!dynamic_pointer_cast<Sphere>(p->shape))continue;
			minR=min(p->shape->cast<Sphere>().radius,minR);
		}
		verletDist=isinf(minR) ? 0 : abs(verletDist)*minR;
	}

	bool recomputeBounds=false;
	// first loop only checks if there something is our
	FOREACH(const shared_ptr<Particle>& p, *dem->particles){
		if(!p->shape) continue;
		const int nNodes=p->shape->nodes.size();
		// below we throw exception for particle that has no functor afer the dispatcher has been called
		// that would prevent mistakenly boundless particless triggering collisions every time
		if(!p->shape->bound){
			LOG_TRACE("recomputeBounds because of #"<<p->id<<" without bound");
			recomputeBounds=true;
			break;
		}
		// existing bound, do we need to update it?
		const Aabb& aabb=p->shape->bound->cast<Aabb>();
		assert(aabb.nodeLastPos.size()==p->shape->nodes.size());
		if(isnan(aabb.min.maxCoeff())||isnan(aabb.max.maxCoeff())) { recomputeBounds=true; break; } 
		Real d2=0; 
		for(int i=0; i<nNodes; i++){
			d2=max(d2,(aabb.nodeLastPos[i]-p->shape->nodes[i]->pos).squaredNorm());
			// maxVel2b=max(maxVel2b,p->shape->nodes[i]->getData<DemData>().vel.squaredNorm());
		}
		if(d2>aabb.maxD2){
			LOG_TRACE("recomputeBounds because of #"<<p->id<<" moved too far");
			recomputeBounds=true;
			break;
		}
		// fine, particle doesn't need to be updated
	}
	// bounds don't need update, collision neither
	if(!recomputeBounds) return false;

	FOREACH(const shared_ptr<Particle>& p, *dem->particles){
		if(!p->shape) continue;
		// call dispatcher now
		boundDispatcher->operator()(p->shape);
		if(!p->shape->bound){
			throw std::runtime_error("InsertionSortCollider: No bound was created for #"+lexical_cast<string>(p->id)+", provide a Bo1_*_Aabb functor for it. (Particle without Aabb are not supported yet, and perhaps will never be (what is such a particle good for?!)");
		}
		Aabb& aabb=p->shape->bound->cast<Aabb>();
		const int nNodes=p->shape->nodes.size();
		// save reference node positions
		aabb.nodeLastPos.resize(nNodes);
		for(int i=0; i<nNodes; i++) aabb.nodeLastPos[i]=p->shape->nodes[i]->pos;
		aabb.maxD2=pow(verletDist,2);
		#if 0
			// proportionally to relVel, shift bbox margin in the direction of velocity
			// take velocity of nodes[0] as representative
			const Vector3r& v0=p->shape->nodes[0]->getData<DemData>().vel;
			Real vNorm=v0.norm();
			Real relVel=max(maxVel2b,maxVel2)==0?0:vNorm/sqrt(max(maxVel2b,maxVel2));
		#endif
		if(verletDist>0){
			aabb.max+=verletDist*Vector3r::Ones();
			aabb.min-=verletDist*Vector3r::Ones();
		}
	};
	return true;
}

bool InsertionSortCollider::prologue_doFullRun(){
	dem=dynamic_cast<DemField*>(field.get());
	assert(dem);
	particles=dem->particles.get();

	// scene->interactions->iterColliderLastRun=-1;

	// conditions when we need to run a full pass
	bool fullRun=false;

	// contacts are dirty and must be detected anew
	if(dem->contacts->dirty || forceInitSort){ fullRun=true; dem->contacts->dirty=false; }

	// number of particles changed
	if((size_t)BB[0].size!=2*particles->size()) fullRun=true;
	//redundant: if(minima.size()!=3*nPar || maxima.size()!=3*nPar) fullRun=true;

	// periodicity changed
	if(scene->isPeriodic != periodic){
		for(int i=0; i<3; i++) BB[i].vec.clear();
		periodic=scene->isPeriodic;
		fullRun=true;
	}
	return fullRun;
}

void InsertionSortCollider::run(){
	#ifdef ISC_TIMING
		timingDeltas->start();
	#endif
	bool fullRun=prologue_doFullRun();

	ISC_CHECKPOINT("aabb");
	bool runBboxes=updateBboxes_doFullRun();
	if(runBboxes) fullRun=true;

	stepInvs=Vector3i::Zero();

	if(!fullRun){
		// done in every step; with fullRun, number of particles might have changed
		// in that case, this is called below, not here
		field->cast<DemField>().contacts->removePending(*this,scene);
		return;
	}
	
	nFullRuns++;

	long nPar=(long)particles->size();

	// pre-conditions
		// adjust storage size
		bool doInitSort=false;
		if(forceInitSort){ doInitSort=true; forceInitSort=false; }
		if(BB[0].size!=2*nPar){
			long BBsize=BB[0].size;
			LOG_DEBUG("Resize bounds containers from "<<BBsize<<" to "<<nPar*2<<", will std::sort.");
			// bodies deleted; clear the container completely, and do as if all bodies were added (rather slow…)
			// future possibility: insertion sort with such operator that deleted bodies would all go to the end, then just trim bounds
			if(2*nPar<BBsize){ for(int i=0; i<3; i++) BB[i].vec.clear(); }
			// more than 100 bodies was added, do initial sort again
			// maybe: should rather depend on ratio of added bodies to those already present...?
			if(2*nPar-BBsize>200 || BBsize==0) doInitSort=true;
			assert((BBsize%2)==0);
			for(int i=0; i<3; i++){
				BB[i].vec.reserve(2*nPar);
				// add lower and upper bounds; coord is not important, will be updated from bb shortly
				for(long id=BBsize/2; id<nPar; id++){ BB[i].vec.push_back(Bounds(0,id,/*isMin=*/true)); BB[i].vec.push_back(Bounds(0,id,/*isMin=*/false)); }
				BB[i].size=BB[i].vec.size();
				assert(BB[i].size==2*nPar);
				assert(nPar==(long)particles->size());
			}
		}
		if(minima.size()!=(size_t)3*nPar){ minima.resize(3*nPar); maxima.resize(3*nPar); }
		assert((size_t)BB[0].size==2*particles->size());

		// update periodicity
		assert(BB[0].axis==0); assert(BB[1].axis==1); assert(BB[2].axis==2);
		if(periodic) for(int i=0; i<3; i++) BB[i].updatePeriodicity(scene);

	#if 0
		// if interactions are dirty, force reinitialization
		if(scene->interactions->dirty){
			doInitSort=true;
			scene->interactions->dirty=false;
		}
	#endif




	ISC_CHECKPOINT("bound");

	// copy bounds along given axis into our arrays
		for(long i=0; i<2*nPar; i++){
			for(int j=0; j<3; j++){
				VecBounds& BBj=BB[j];
				const Particle::id_t id=BBj[i].id;
				const shared_ptr<Particle>& b=(*particles)[id];
				if(likely(b)){
					const shared_ptr<Bound>& bv=b->shape->bound;
					// coordinate is min/max if has bounding volume, otherwise both are the position. Add periodic shift so that we are inside the cell
					// watch out for the parentheses around ?: within ?: (there was unwanted conversion of the Reals to bools!)
					BBj[i].coord=((BBj[i].flags.hasBB=((bool)bv)) ? (BBj[i].flags.isMin ? bv->min[j] : bv->max[j]) : (b->shape->nodes[0]->pos[j])) - (periodic ? BBj.cellDim*BBj[i].period : 0.);
				} else { // vanished particle
					BBj[i].flags.hasBB=false;
					// when doing initial sort, set to -inf so that nonexistent particles don't generate inversions later
					if(doInitSort) BBj[i].coord=-Inf;
					// otherwise keep the coordinate as-is, to minimize inversions
				}
				// if initializing periodic, shift coords & record the period into BBj[i].period
				if(doInitSort && periodic) {
					BBj[i].coord=cellWrap(BBj[i].coord,0,BBj.cellDim,BBj[i].period);
				}
			}	
		}
	// for each body, copy its minima and maxima, for quick checks of overlaps later
	for(Particle::id_t id=0; id<nPar; id++){
		BOOST_STATIC_ASSERT(sizeof(Vector3r)==3*sizeof(Real));
		const shared_ptr<Particle>& b=(*particles)[id];
		if(likely(b)){
			const shared_ptr<Bound>& bv=b->shape->bound;
			if(likely(bv)) { memcpy(&minima[3*id],&bv->min,3*sizeof(Real)); memcpy(&maxima[3*id],&bv->max,3*sizeof(Real)); } // ⇐ faster than 6 assignments 
			else{ const Vector3r& pos=b->shape->nodes[0]->pos; memcpy(&minima[3*id],&pos,3*sizeof(Real)); memcpy(&maxima[3*id],&pos,3*sizeof(Real)); }
		} else { memset(&minima[3*id],0,3*sizeof(Real)); memset(&maxima[3*id],0,3*sizeof(Real)); }
	}

	ISC_CHECKPOINT("copy");

	// process interactions that the constitutive law asked to be erased
	field->cast<DemField>().contacts->removePending(*this,scene);
	
	ISC_CHECKPOINT("erase");

	// sort
		// the regular case
		if(!doInitSort && !sortThenCollide){
			/* each inversion in insertionSort calls handleBoundInversion, which in turns may add/remove interaction */
			if(!periodic) for(int i=0; i<3; i++){
				//Vector3i invs=countInversions();
				insertionSort(BB[i],/*collide*/true,i); 
				//LOG_INFO(invs.sum()<<"/"<<stepInvs<<" invs (counted/insertion sort)");
			}
			else for(int i=0; i<3; i++) insertionSortPeri(BB[i],/*collide*/true,i);
		}
		// create initial interactions (much slower)
		else {
			if(doInitSort){
				// the initial sort is in independent in 3 dimensions, may be run in parallel; it seems that there is no time gain running in parallel, though
				// important to reset loInx for periodic simulation (!!)
				LOG_DEBUG("Initial std::sort over all axes");
				for(int i=0; i<3; i++) { BB[i].loIdx=0; std::sort(BB[i].vec.begin(),BB[i].vec.end()); }
				numReinit++;
			} else { // sortThenCollide
				if(!periodic) for(int i=0; i<3; i++) insertionSort(BB[i],false,i);
				else for(int i=0; i<3; i++) insertionSortPeri(BB[i],false,i);
			}
			// traverse the container along requested axis
			assert(sortAxis==0 || sortAxis==1 || sortAxis==2);
			VecBounds& V=BB[sortAxis];
			// go through potential aabb collisions, create interactions as necessary
			if(!periodic){
				for(long i=0; i<2*nPar; i++){
					// start from the lower bound (i.e. skipping upper bounds)
					// skip bodies without bbox, because they don't collide
					if(unlikely(!(V[i].flags.isMin && V[i].flags.hasBB))) continue;
					const Particle::id_t& iid=V[i].id;
					// go up until we meet the upper bound
					for(long j=i+1; /* handle case 2. of swapped min/max */ j<2*nPar && V[j].id!=iid; j++){
						const Particle::id_t& jid=V[j].id;
						// take 2 of the same condition (only handle collision [min_i..max_i]+min_j, not [min_i..max_i]+min_i (symmetric)
						if(!V[j].flags.isMin) continue;
						/* abuse the same function here; since it does spatial overlap check first, it is OK to use it */
						handleBoundInversion(iid,jid);
						assert(j<2*nPar-1);
					}
				}
			} else { // periodic case: see comments above
				for(long i=0; i<2*nPar; i++){
					if(unlikely(!(V[i].flags.isMin && V[i].flags.hasBB))) continue;
					const Particle::id_t& iid=V[i].id;
					long cnt=0;
					// we might wrap over the periodic boundary here; that's why the condition is different from the aperiodic case
					for(long j=V.norm(i+1); V[j].id!=iid; j=V.norm(j+1)){
						const Particle::id_t& jid=V[j].id;
						if(!V[j].flags.isMin) continue;
						handleBoundInversionPeri(iid,jid);
						if(cnt++>2*(long)nPar){ LOG_FATAL("Uninterrupted loop in the initial sort?"); throw std::logic_error("loop??"); }
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

// return coordinate wrapped to 0…x1, relative to x0; don't care about period
Real InsertionSortCollider::cellWrapRel(const Real x, const Real x0, const Real x1){
	Real xNorm=(x-x0)/(x1-x0);
	return (xNorm-floor(xNorm))*(x1-x0);
}

void InsertionSortCollider::insertionSortPeri(VecBounds& v, bool doCollide, int ax){
	assert(periodic);
	long &loIdx=v.loIdx; const long &size=v.size;
	for(long _i=0; _i<size; _i++){
		const long i=v.norm(_i);
		const long i_1=v.norm(i-1);
		//switch period of (i) if the coord is below the lower edge cooridnate-wise and just above the split
		if(i==loIdx && v[i].coord<0){ v[i].period-=1; v[i].coord+=v.cellDim; loIdx=v.norm(loIdx+1); }
		// coordinate of v[i] used to check inversions
		// if crossing the split, adjust by cellDim;
		// if we get below the loIdx however, the v[i].coord will have been adjusted already, no need to do that here
		const Real iCmpCoord=v[i].coord+(i==loIdx ? v.cellDim : 0); 
		// no inversion
		if(v[i_1].coord<=iCmpCoord) continue;
		// vi is the copy that will travel down the list, while other elts go up
		// if will be placed in the list only at the end, to avoid extra copying
		int j=i_1; Bounds vi=v[i];  const bool viHasBB=vi.flags.hasBB;
		while(v[j].coord>vi.coord + /* wrap for elt just below split */ (v.norm(j+1)==loIdx ? v.cellDim : 0)){
			long j1=v.norm(j+1);
			// OK, now if many bodies move at the same pace through the cell and at one point, there is inversion,
			// this can happen without any side-effects
			if (false && v[j].coord>2*v.cellDim){
				// this condition is not strictly necessary, but the loop of insertionSort would have to run more times.
				// Since size of particle is required to be < .5*cellDim, this would mean simulation explosion anyway
				LOG_FATAL("Body #"<<v[j].id<<" going faster than 1 cell in one step? Not handled.");
				throw runtime_error(__FILE__ ": body mmoving too fast (skipped 1 cell).");
			}
			Bounds& vNew(v[j1]); // elt at j+1 being overwritten by the one at j and adjusted
			vNew=v[j];
			// inversions close the the split need special care
			if(unlikely(j==loIdx && vi.coord<0)) { vi.period-=1; vi.coord+=v.cellDim; loIdx=v.norm(loIdx+1); }
			else if(unlikely(j1==loIdx)) { vNew.period+=1; vNew.coord-=v.cellDim; loIdx=v.norm(loIdx-1); }
			if(likely(doCollide && viHasBB && v[j].flags.hasBB)){
				// see https://bugs.launchpad.net/woo/+bug/669095 and similar problem in aperiodic insertionSort
				#if 0
				if(vi.id==vNew.id){
					LOG_FATAL("Inversion of body's #"<<vi.id<<" boundary with its other boundary, "<<v[j].coord<<" meets "<<vi.coord);
					throw runtime_error(__FILE__ ": Body's boundary metting its opposite boundary.");
				}
				#endif
				if(likely(vi.id!=vNew.id)){
					#ifdef WOO_DEBUG
						stepInvs[ax]++; numInvs[ax]++;
					#endif
					handleBoundInversionPeri(vi.id,vNew.id);
				}
			}
			j=v.norm(j-1);
		}
		v[v.norm(j+1)]=vi;
	}
}

// called by the insertion sort if 2 bodies swapped their bounds
void InsertionSortCollider::handleBoundInversionPeri(Particle::id_t id1, Particle::id_t id2){
	assert(periodic);
	// do bboxes overlap in all 3 dimensions?
	Vector3i periods;
	bool overlap=spatialOverlapPeri(id1,id2,scene,periods);
	// existing interaction?
	const shared_ptr<Contact>& C=dem->contacts->find(id1,id2);
	bool hasInter=(bool)C;
	#ifdef PISC_DEBUG
		if(watchIds(id1,id2)) LOG_DEBUG("Inversion #"<<id1<<"+#"<<id2<<", overlap=="<<overlap<<", hasInter=="<<hasInter);
	#endif
	// interaction doesn't exist and shouldn't, or it exists and should
	if(likely(!overlap && !hasInter)) return;
	if(overlap && hasInter){  return; }
	// create interaction if not yet existing
	if(overlap && !hasInter){ // second condition only for readability
		#ifdef PISC_DEBUG
			if(watchIds(id1,id2)) LOG_DEBUG("Attemtping collision of #"<<id1<<"+#"<<id2);
		#endif
		const shared_ptr<Particle>& pA((*particles)[id1]);
		const shared_ptr<Particle>& pB((*particles)[id2]);
		if(!Collider::mayCollide(pA,pB,dem)) return;
		// LOG_TRACE("Creating new interaction #"<<id1<<"+#"<<id2);
		shared_ptr<Contact> newC=make_shared<Contact>();
		newC->pA=pA; newC->pB=pB;
		newC->cellDist=periods;
		#ifdef PISC_DEBUG
			if(watchIds(id1,id2)) LOG_DEBUG("Created intr #"<<id1<<"+#"<<id2<<", periods="<<periods);
		#endif
		dem->contacts->add(newC);
		return;
	}
	if(!overlap && hasInter){
		if(!C->isReal()) {
			dem->contacts->remove(C);
			#ifdef PISC_DEBUG
				if(watchIds(id1,id2)) LOG_DEBUG("Erased intr #"<<id1<<"+#"<<id2);
			#endif
		}
		return;
	}
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
bool InsertionSortCollider::spatialOverlapPeri(Particle::id_t id1, Particle::id_t id2, Scene* scene, Vector3i& periods) const {
	assert(periodic);
	assert(id1!=id2); // programming error, or weird bodies (too large?)
	for(int axis=0; axis<3; axis++){
		Real dim=scene->cell->getSize()[axis];
		// LOG_DEBUG("dim["<<axis<<"]="<<dim);
		// too big bodies in interaction
		#if 0
		if(!(maxima[3*id1+axis]-minima[3*id1+axis]<.99*dim)){
			LOG_ERROR("##"<<id1<<"+"<<id2<<": min["<<axis<<"]="<<minima[3*id1+axis]<<" max["<<axis<<"]="<<maxima[3*id1+axis]<<", dim="<<dim<<", ignored!!")
			return false;
		}
		#endif
		assert(!isnan(minima[3*id1+axis])); assert(!isnan(maxima[3*id1+axis]));
		assert(!isnan(minima[3*id2+axis])); assert(!isnan(maxima[3*id2+axis]));
		assert(maxima[3*id1+axis]-minima[3*id1+axis]<.99*dim); assert(maxima[3*id2+axis]-minima[3*id2+axis]<.99*dim);
		// find particle of which minimum when taken as period start will make the gap smaller
		Real m1=minima[3*id1+axis],m2=minima[3*id2+axis];
		Real wMn=(cellWrapRel(m1,m2,m2+dim)<cellWrapRel(m2,m1,m1+dim)) ? m2 : m1;
		#ifdef PISC_DEBUG
		if(watchIds(id1,id2)){
			TRVAR4(id1,id2,axis,dim);
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
		if(unlikely((pmn1!=pmx1) || (pmn2!=pmx2))){
			Real span=(pmn1!=pmx1?mx1-mn1:mx2-mn2); if(span<0) span=dim-span;
			LOG_FATAL("Particle #"<<(pmn1!=pmx1?id1:id2)<<" spans over half of the cell size "<<dim<<" (axis="<<axis<<", min="<<(pmn1!=pmx1?mn1:mn2)<<", max="<<(pmn1!=pmx1?mx1:mx2)<<", span="<<span<<")");
			throw runtime_error(__FILE__ ": Particle larger than half of the cell size encountered.");
		}
		periods[axis]=(int)(pmn1-pmn2);
		if(!(mn1<=mx2 && mx1 >= mn2)) return false;
	}
	#ifdef PISC_DEBUG
		if(watchIds(id1,id2)) LOG_DEBUG("Overlap #"<<id1<<"+#"<<id2<<", periods "<<periods);
	#endif
	return true;
}

py::tuple InsertionSortCollider::dumpBounds(){
	py::list bl[3]; // 3 bound lists, inserted into the tuple at the end
	for(int axis=0; axis<3; axis++){
		VecBounds& V=BB[axis];
		if(periodic){
			for(long i=0; i<V.size; i++){
				long ii=V.norm(i); // start from the period boundary
				bl[axis].append(py::make_tuple(V[ii].coord,(V[ii].flags.isMin?-1:1)*V[ii].id,V[ii].period));
			}
		} else {
			for(long i=0; i<V.size; i++){
				bl[axis].append(py::make_tuple(V[i].coord,(V[i].flags.isMin?-1:1)*V[i].id));
			}
		}
	}
	return py::make_tuple(bl[0],bl[1],bl[2]);
}

py::object InsertionSortCollider::dbgInfo(){
	py::dict ret;
	return ret;
}
