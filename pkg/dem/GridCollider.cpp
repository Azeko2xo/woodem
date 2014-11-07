#include<woo/pkg/dem/GridCollider.hpp>

WOO_PLUGIN(dem,(GridCollider));
WOO_IMPL_LOGGER(GridCollider);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_GridCollider__CLASS_BASE_DOC_ATTRS);

void GridCollider::pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){
	if(py::len(t)==0) return; // nothing to do
	if(py::len(t)!=1) throw invalid_argument("GridCollider optionally takes exactly one list of GridBoundFunctor's as non-keyword argument for constructor ("+to_string(py::len(t))+" non-keyword ards given instead)");
	if(!boundDispatcher) boundDispatcher=make_shared<GridBoundDispatcher>();
	vector<shared_ptr<GridBoundFunctor>> vf=py::extract<vector<shared_ptr<GridBoundFunctor>>>((t[0]))();
	for(const auto& f: vf) boundDispatcher->add(f);
	t=py::tuple(); // empty the args
}

void GridCollider::getLabeledObjects(const shared_ptr<LabelMapper>& labelMapper){ if(boundDispatcher) boundDispatcher->getLabeledObjects(labelMapper); Engine::getLabeledObjects(labelMapper); }

void GridCollider::postLoad(GridCollider&, void* attr){
	if(domain.isEmpty() || domain.volume()==0) throw std::runtime_error("GridCollider.domain: may not be empty.");
	if(!(minCellSize>0)) throw std::runtime_error("GridCollider.minCellSize: must be positive (not "+to_string(minCellSize));
	dim=(domain.sizes()/minCellSize).cast<int>();
	cellSize=(domain.sizes().array()/dim.cast<Real>().array()).matrix();
	shrink=around?cellSize.minCoeff()/2.:0.;
	if(!boundDispatcher){ boundDispatcher=make_shared<GridBoundDispatcher>(); }
}

void GridCollider::selfTest(){
	if(domain.isEmpty()) throw std::runtime_error("GridCollider.domain: may not be empty.");
	if(!(minCellSize>0)) throw std::runtime_error("GridCollider.minCellSize: must be positive (not "+to_string(minCellSize));
	if(dim.minCoeff()<=0) throw std::logic_error("GridCollider.dim: all components must be positive.");
	if(!(cellSize.minCoeff()>0)) throw std::logic_error("GridCollider.cellSize: all components must be positive.");
	if(!boundDispatcher) throw std::logic_error("GridCollider.boundDispatcher: must not be None.");
}

bool GridCollider::tryAddContact(const Particle::id_t& idA, const Particle::id_t& idB) const {
	// this is rather fast, therefore do it before looking up the contact
	const auto& pA((*dem->particles)[idA]); const auto& pB((*dem->particles)[idB]);
	if(!Collider::mayCollide(dem,pA,pB)) return false;
	// contact lookup
	const shared_ptr<Contact>& C=dem->contacts->find(idA,idB);
	if(C){
		C->stepLastSeen=scene->step; // mark contact as seen by us; unseen will be deleted by ContactLoop automatically
		return false; // already in contact, nothing to do
	}
	LOG_TRACE("Creating new contact ##"<<idA<<"+"<<idB);
	shared_ptr<Contact> newC=make_shared<Contact>();
	// mimick the way clDem::Collider does the job so that results are easily comparable
	if(idA<idB){ newC->pA=pA; newC->pB=pB; }
	else{ newC->pA=pB; newC->pB=pA; }
	newC->stepCreated=scene->step;
	newC->stepLastSeen=scene->step;
	// returns false if the contact exists (added by a different thread meanwhile)
	return dem->contacts->add(newC,/*threadSafe*/true);
}

bool GridCollider::tryDeleteContact(const Particle::id_t& idA, const Particle::id_t& idB) const {
	const shared_ptr<Contact>& C=dem->contacts->find(idA,idB);
	if(!C) return false; // this should not happen at all ?!
	if(C->isReal()) C->setNotColliding(); // contact still exists, set stepCreated to -1 (will be deleted by ContactLoop)
	else return dem->contacts->remove(C,/*threadSafe*/true);
	return true;
}

template<bool sameGridCell, bool addContacts>
void GridCollider::processCell(const shared_ptr<GridStore>& gridA, const Vector3i& ijkA, const shared_ptr<GridStore>& gridB, const Vector3i& ijkB) const {
	assert(!sameGridCell || (gridA==gridB && ijkA==ijkB));
	const int aMin=0; const int aMax=gridA->size(ijkA);
	// if checking the same cell against itself, set bMin to -1,
	// that will make it start from the value of (a+1) at every iteration
	const int bMax=gridB->size(ijkB);
	for(int a=aMin; a<aMax; a++){
		for(int b=(sameGridCell?a+1:0); b<bMax; b++){
			const Particle::id_t& idA=gridA->get(ijkA,a); const Particle::id_t& idB=gridB->get(ijkB,b);
			if(!sameGridCell && idA==idB) continue;
			if(addContacts) tryAddContact(idA,idB);
			else tryDeleteContact(idA,idB);
		}
	}
}

bool GridCollider::allParticlesWithinPlay() const {
	for(const shared_ptr<Particle>& p: *dem->particles){
		if(!p->shape) continue;
		if(!p->shape->bound) return false;
		assert(dynamic_cast<GridBound*>(p->shape->bound.get()));
		const auto& gb(p->shape->bound->cast<GridBound>());
		if(!gb.insideNodePlay(p->shape)) return false;
	}
	return true;
}

void GridCollider::prepareGridCurr(){
	// recycle to avoid re-allocations (expensive)
	std::swap(gridPrev,gridCurr);
	// if some parameters differ, make a new one
	if(!gridCurr || gridCurr->sizes()!=dim || gridCurr->cellLen!=gridDense || gridCurr->exIniSize!=exIniSize || gridCurr->exNumMaps!=exNumMaps){
		gridCurr=make_shared<GridStore>(dim,gridDense,/*locking*/true,exIniSize,exNumMaps);
		LOG_WARN("Allocated new GridStore.");
	} else {
		gridCurr->clear();
	}
	gridCurr->lo=domain.min();
	gridCurr->cellSize=cellSize;
}

void GridCollider::fillGridCurr(){
	shared_ptr<GridCollider> shared_this=static_pointer_cast<GridCollider>(shared_from_this());
	size_t nPar=dem->particles->size();
	boundDispatcher->scene=scene;
	boundDispatcher->field=field;
	boundDispatcher->updateScenePtr();
	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)
	#endif
	for(size_t parId=0; parId<nPar; parId++){
		const shared_ptr<Particle>& p((*dem->particles)[parId]);
		if(!p || !p->shape) continue;
		// when not in diffStep, add all particles, even if within their nodePlay boxes
		boundDispatcher->operator()(p->shape,p->id,shared_this,gridCurr);
	}
}

void GridCollider::run(){
	#ifdef GC_TIMING
		if(!timingDeltas) timingDeltas=make_shared<TimingDeltas>();
		timingDeltas->start();
	#endif
	dem=static_cast<DemField*>(field.get());

	// pending contacts will be kept as potential (uselessly), as there is no way to quickly check for overlap
	// of two particles given their ids. After the next full run, ContactLoop will remove those based on stepLastSeen.
	dem->contacts->clearPending();

	bool allOk=allParticlesWithinPlay();
	// if dirty, clear the dirty flag and do a full run
	if(dem->contacts->dirty){ allOk=false; dem->contacts->dirty=false; }
	GC_CHECKPOINT("check-play");
	if(allOk) return;

	/*** FULL COLLIDER RUN ***/
	nFullRuns++;

	prepareGridCurr(); GC_CHECKPOINT("prepare-grid");
	bool diffStep=(useDiff && gridPrev && gridPrev->isCompatible(gridCurr));
	
	// with diffStep, we handle contact deletion ourselves
	if(diffStep){ dem->contacts->stepColliderLastRun=-1; }
	// without diffStep, ContactLoop deletes overdue contacts
	else { dem->contacts->stepColliderLastRun=scene->step; }

	/* update grid bounding boxes */
	fillGridCurr(); GC_CHECKPOINT("fill-grid");

	if(diffStep){
		/* contact search with history */
		// do only relative computation of appeared potential contacts
		// gridNew will be created if needed inside the complement function
		gridPrev->complements(gridCurr,gridOld,gridNew,complSetMinSize,timingDeltas);
		GC_CHECKPOINT("complements");
		size_t N=gridCurr->linSize();
		#ifdef WOO_OPENMP
			#pragma omp parallel for schedule(static,1000)
		#endif
		for(size_t lin=0; lin<N; lin++){
			GC_CHECKPOINT(": start");
			Vector3i ijk=gridCurr->lin2ijk(lin);
			assert(ijk==gridNew->lin2ijk(lin));
			size_t sizeNew=gridNew->size(ijk), sizeOld=gridNew->size(ijk);
			size_t sizePrev=gridPrev->size(ijk), sizeCurr=gridCurr->size(ijk);
			// new-new potential contacts
			if(sizeNew>0){
				if(sizeNew>1) processCell</*sameGridCell*/true>(gridNew,ijk,gridNew,ijk);
				GC_CHECKPOINT(": new+new");
				// curr-new potential contacts
				if(sizePrev>0) processCell</*sameGridCell*/false>(gridPrev,ijk,gridNew,ijk);
				GC_CHECKPOINT(": prev+new");
				// old-old delete
			}
			if(sizeOld>0){
				if(sizeOld>1) processCell</*sameGridCell*/true,/*addContacts*/false>(gridOld,ijk,gridOld,ijk);
				GC_CHECKPOINT(": old+old");
				// old-curr delete
				if(sizeCurr>0) processCell<true,/*addContacts*/false>(gridCurr,ijk,gridOld,ijk);
				GC_CHECKPOINT(": curr+old");
			}
		}
	} else {
		/* contact search without history */
		// 
		size_t N=gridCurr->linSize();
		if(around){
			throw std::runtime_error("GridCollider.around==true: implementation is broken and very likely weak performance-wise; forbidden.");
			#if 0
				/* traverse the grid with 2-stride along all axes, and check for every strided cell:
					* contacts within itself
					* contacts with the central cell and all cells around
					* contacts of all cells "lower" between themselves
				*/
				#ifdef WOO_OPENMP
					#pragma omp parallel for
				#endif
				for(size_t lin=0; lin<N; lin++){
					Vector3i ijk=gridCurr->lin2ijk(lin);
					processCell</*sameGridCell*/true>(gridCurr,ijk,gridCurr,ijk);
					for(const Vector3i& dIjk: {
						Vector3i(-1,0,0),Vector3i(0,-1,0),Vector3i(0,0,-1),
						Vector3i(0,-1,-1),Vector3i(-1,0,-1),Vector3i(-1,-1,0),
						// Vector3i(1,-1,-1),Vector3i(-1,1,-1),Vector3i(-1,-1,1),
						Vector3i(-1,-1,-1)
					}){
						Vector3i ijk2(ijk+dIjk);
						if(!gridCurr->ijkOk(ijk2)) continue;
						// GC_CHECKPOINT("process-cell (around)");
						processCell</*sameGridCell*/false>(gridCurr,ijk,gridCurr,ijk2);
					}
				}
			#endif
		} else {
			GC_CHECKPOINT("pre-loop");
			#ifdef WOO_OPENMP
				#pragma omp parallel for schedule(static,1000)
			#endif
			for(size_t lin=0; lin<N; lin++){
				GC_CHECKPOINT(": start");
				Vector3i ijk=gridCurr->lin2ijk(lin);
				// avoid useless function call
				if(gridCurr->size(ijk)<=1){ GC_CHECKPOINT(": empty"); continue;  }
				// check contacts between all particles in this cell
				processCell</*sameGridCell*/true>(gridCurr,ijk,gridCurr,ijk);
				GC_CHECKPOINT(": curr+curr");
			}
		}
	}
	GC_CHECKPOINT("end");
}

#ifdef WOO_OPENGL
#include<woo/lib/opengl/GLUtils.hpp>
	void GridCollider::render(const GLViewInfo&){
		// show domain with tics along all sides of the box
		// GLUtils::AlignedBox(domain,color);
		GLUtils::AlignedBoxWithTicks(domain,cellSize,cellSize,color);
	#if 0
		glColor3v(color);
		glBegin(GL_LINES);
			// for each axis (along which the sides go), there are found corners
			for(int ax:{0,1,2}){
				int ax1=(ax+1)%3, ax2=(ax+2)%3;
				// coordinates of the corner
				Real c1[]={domain.min()[ax1],domain.max()[ax1],domain.min()[ax1],domain.max()[ax1]};
				Real c2[]={domain.min()[ax2],domain.min()[ax2],domain.max()[ax2],domain.max()[ax2]};
				// tics direction (endpoint offsets from the point at the edge)
				Real dc1[]={cellSize[ax1],-cellSize[ax1],cellSize[ax1],-cellSize[ax1]};
				Real dc2[]={cellSize[ax2],cellSize[ax2],-cellSize[ax2],-cellSize[ax2]};
				for(int corner:{0,1,2,3}){
					Vector3r pt; pt[ax1]=c1[corner]; pt[ax2]=c2[corner];
					Vector3r dx1; dx1[ax]=0; dx1[ax1]=dc1[corner]; dx1[ax2]=0;
					Vector3r dx2; dx2[ax]=0; dx2[ax1]=0; dx2[ax2]=dc2[corner];
					for(pt[ax]=domain.min()[ax]+cellSize[ax]; pt[ax]<domain.max()[ax]; pt[ax]+=cellSize[ax]){
						glVertex3v(pt); glVertex3v((pt+dx1).eval());
						glVertex3v(pt); glVertex3v((pt+dx2).eval());
					}
				}
			}
		glEnd();
	#endif
		if(!renderCells) return;

		if(!occupancyRange){
			occupancyRange=make_shared<ScalarRange>();
			occupancyRange->label="occupancy";
		}
		if(gridCurr && gridCurr->sizes()==dim){
			// show cells which are used, with some color based on the number of entries in there
			shared_ptr<GridStore> grid(gridCurr); // copy shared_ptr so that we don't crash if changed meanwhile
			if(grid){
				Vector3i maxIjk=grid->gridSize;
				for(Vector3i ijk=Vector3i::Zero(); ijk[0]<maxIjk[0]; ijk[0]++){
					if(ijk[0]>=grid->gridSize[0]) break;
					for(ijk[1]=0; ijk[1]<maxIjk[1]; ijk[1]++){
						if(ijk[1]>=grid->gridSize[1]) break;
						for(ijk[2]=0; ijk[2]<maxIjk[2]; ijk[2]++){
							if(ijk[2]>=grid->gridSize[2]) break;
							int occupancy=grid->size(ijk);
							if(occupancy==0 || occupancy<minOccup) continue;
							GLUtils::AlignedBox(grid->ijk2boxShrink(ijk,.1),occupancyRange->color(occupancy));
						}
					}
				}
			}
		}
	};
#endif
