#include<woo/pkg/dem/GridCollider.hpp>

WOO_PLUGIN(dem,(GridCollider));
CREATE_LOGGER(GridCollider);

void GridCollider::postLoad(GridCollider&, void* attr){
	if(domain.isEmpty() || domain.volume()==0) throw std::runtime_error("GridCollider.domain: may not be empty.");
	if(!(minCellSize>0)) throw std::runtime_error("GridCollider.minCellSize: must be positive (not "+to_string(minCellSize));
	dim=(domain.sizes()/minCellSize).cast<int>();
	cellSize=(domain.sizes().array()/dim.cast<Real>().array()).matrix();
	shrink=around?cellSize.minCoeff()/2.:0.;
	if(!gridBoundDispatcher){ gridBoundDispatcher=make_shared<GridBoundDispatcher>(); }
}

void GridCollider::selfTest(){
	if(domain.isEmpty()) throw std::runtime_error("GridCollider.domain: may not be empty.");
	if(!(minCellSize>0)) throw std::runtime_error("GridCollider.minCellSize: must be positive (not "+to_string(minCellSize));
	if(dim.minCoeff()<=0) throw std::logic_error("GridCollider.dim: all components must be positive.");
	if(!(cellSize.minCoeff()>0)) throw std::logic_error("GridCollider.cellSize: all components must be positive.");
	if(!gridBoundDispatcher) throw std::logic_error("GridCollider.gridBoundDispatcher: must not be None.");
}

bool GridCollider::tryAddNewContact(const Particle::id_t& idA, const Particle::id_t& idB) const {
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
	dem->contacts->add(newC);
	newC->stepCreated=scene->step;
	newC->stepLastSeen=scene->step;
	return true;
}

template<int what>
void GridCollider::processCell(const shared_ptr<GridStore>& gridA, const Vector3i& ijkA, const shared_ptr<GridStore>& gridB, const Vector3i& ijkB) const {
	static_assert((what==PROCESS_FULL) /* || (what==...) */,"Invalid template parameter to processCell<what>.");
	int aMin=0, aMax=gridA->size(ijkA);
	// if checking the same cell against itself, set bMin to -1,
	// that will make it start from the value of (a+1) at every iteration
	int bMin=((gridA.get()==gridB.get() && ijkA==ijkB)?-1:0); int bMax=gridB->size(ijkB);
	for(int a=aMin; a<aMax; a++){
		for(int b=(bMin<0?a+1:0); b<bMax; b++){
			Particle::id_t idA=gridA->get(ijkA,a), idB=gridB->get(ijkB,b);
			switch(what){
				case PROCESS_FULL: tryAddNewContact(idA,idB); break;
			}
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
	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)
	#endif
	for(size_t parId=0; parId<nPar; parId++){
		const shared_ptr<Particle>& p((*dem->particles)[parId]);
		if(!p || !p->shape) continue;
		// when not in diffStep, add all particles, even if within their nodePlay boxes
		gridBoundDispatcher->operator()(p->shape,p->id,shared_this,gridCurr,/*force*/true);
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
	GC_CHECKPOINT("check-play");
	if(allOk) return;
	dem->contacts->stepColliderLastRun=scene->step;

	prepareGridCurr(); GC_CHECKPOINT("prepare-grid");

	/* update grid bounding boxes */
	fillGridCurr(); GC_CHECKPOINT("fill-grid");

	// this is not supported yet
	bool diffStep=(false && gridPrev && gridPrev->isCompatible(gridCurr));

	/* contact search with history */
	if(diffStep){
		// do only relative computation of appeared/disappeared potential contacts
		// gridNew, gridOld will be created if needed inside computeRelativeComplements
		gridCurr->computeRelativeComplements(*gridPrev,gridNew,gridOld);
		GC_CHECKPOINT("complements");
	} else {
		/* contact search without history */
		// 
		size_t N=gridCurr->linSize();
		if(around){
			// throw std::runtime_error("GridCollider.around==true: not yet implemented!");
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
				for(const Vector3i& dIjk: {
					Vector3i(0,0,0),
					Vector3i(-1,0,0),Vector3i(0,-1,0),Vector3i(0,0,-1),
					Vector3i(0,-1,-1),Vector3i(-1,0,-1),Vector3i(-1,-1,0),
					// Vector3i(1,-1,-1),Vector3i(-1,1,-1),Vector3i(-1,-1,1),
					Vector3i(-1,-1,-1)
				}){
					Vector3i ijk2(ijk+dIjk);
					if(!gridCurr->ijkOk(ijk2)) continue;
					// GC_CHECKPOINT("process-cell (around)");
					processCell<PROCESS_FULL>(gridCurr,ijk,gridCurr,ijk2);
				}
			}
		} else {
			#ifdef WOO_OPENMP
				#pragma omp parallel for schedule(guided,1000)
			#endif
			for(size_t lin=0; lin<N; lin++){
				GC_CHECKPOINT("process-cells prologue (!around)");
				Vector3i ijk=gridCurr->lin2ijk(lin);
				// check contacts between all particles in this cell
				if(gridCurr->size(ijk)<=1) continue; // avoid function call
				GC_CHECKPOINT("process-cells call (!around)");
				processCell<PROCESS_FULL>(gridCurr,ijk,gridCurr,ijk);
			}
		}
	}
	GC_CHECKPOINT("rest");
}

#ifdef WOO_OPENGL
#include<woo/lib/opengl/GLUtils.hpp>
	void GridCollider::render(const GLViewInfo&){
		// show domain with tics along all sides of the box
		GLUtils::AlignedBox(domain,color);
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
							size_t occupancy=grid->size(ijk);
							if(occupancy==0) continue;
							GLUtils::AlignedBox(grid->ijk2boxShrink(ijk,.1),occupancyRange->color(occupancy));
						}
					}
				}
			}
		}
	};
#endif
