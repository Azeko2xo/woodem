#include<woo/pkg/dem/GridCollider.hpp>

WOO_PLUGIN(dem,(GridCollider));
CREATE_LOGGER(GridCollider);

void GridCollider::postLoad(GridCollider&, void* attr){
	if(domain.isEmpty() || domain.volume()==0) throw std::runtime_error("GridCollider.domain: may not be empty.");
	if(!(minCellSize>0)) throw std::runtime_error("GridCollider.minCellSize: must be positive (not "+to_string(minCellSize));
	dim=(domain.sizes()/minCellSize).cast<int>();
	cellSize=(domain.sizes().array()/dim.cast<Real>().array()).matrix();
	if(!gridBoundDispatcher){ gridBoundDispatcher=make_shared<GridBoundDispatcher>(); }
}

void GridCollider::selfTest(){
	if(domain.isEmpty()) throw std::runtime_error("GridCollider.domain: may not be empty.");
	if(!(minCellSize>0)) throw std::runtime_error("GridCollider.minCellSize: must be positive (not "+to_string(minCellSize));
	if(dim.minCoeff()<=0) throw std::logic_error("GridCollider.dim: all components must be positive.");
	if(!(cellSize.minCoeff()>0)) throw std::logic_error("GridCollider.cellSize: all components must be positive.");
	if(!gridBoundDispatcher) throw std::logic_error("GridCollider.gridBoundDispatcher: must not be None.");
}

Vector3i GridCollider::xyz2ijk(const Vector3r& xyz) const {
	// cast rounds down properly
	return ((xyz-domain.min()).array()/cellSize.array()).cast<int>().matrix();
};

Vector3r GridCollider::ijk2boxMin(const Vector3i& ijk) const{
	return domain.min()+(ijk.cast<Real>().array()*cellSize.array()).matrix();
}

AlignedBox3r GridCollider::ijk2box(const Vector3i& ijk) const{
	AlignedBox3r ret; ret.min()=ijk2boxMin(ijk); ret.max()=ret.min()+cellSize;
	return ret;
};

AlignedBox3r GridCollider::ijk2boxShrink(const Vector3i& ijk, const Real& shrink) const{
	AlignedBox3r box=ijk2box(ijk); Vector3r s=box.sizes();
	box.min()=box.min()+.5*shrink*s;
	box.max()=box.max()-.5*shrink*s;
	return box;
}

Vector3r GridCollider::xyzNearXyz(const Vector3r& xyz, const Vector3i& ijk) const{
	AlignedBox3r box=ijk2box(ijk);
	Vector3r ret;
	for(int ax:{0,1,2}){
		ret[ax]=(xyz[ax]<box.min()[ax]?box.min()[ax]:(xyz[ax]>box.max()[ax]?box.max()[ax]:xyz[ax]));
	}
	return ret;
}

Vector3r GridCollider::xyzNearIjk(const Vector3i& from, const Vector3i& ijk) const{
	AlignedBox3r box=ijk2box(ijk);
	Vector3r ret;
	for(int ax:{0,1,2}){
		ret[ax]=(from[ax]<=ijk[ax]?box.min()[ax]:box.max()[ax]);
	}
	return ret;
}

bool GridCollider::tryAddNewContact(const Particle::id_t& idA, const Particle::id_t& idB){
	// this is rather fast, therefore do it before looking up the contact
	const auto& pA((*dem->particles)[idA]); const auto& pB((*dem->particles)[idB]);
	if(!Collider::mayCollide(dem,pA,pB)) return false;
	// contact lookup
	bool hasC=(bool)dem->contacts->find(idA,idB);
	if(hasC) return false; // already in contact, nothing to do
	LOG_TRACE("Creating new contact ##"<<idA<<"+"<<idB);
	shared_ptr<Contact> newC=make_shared<Contact>();
	// mimick the way clDem::Collider does the job so that results are easily comparable
	if(idA<idB){ newC->pA=pA; newC->pB=pB; }
	else{ newC->pA=pB; newC->pB=pA; }
	dem->contacts->add(newC);
	return true;
}

void GridCollider::processCell(const int& what, const shared_ptr<GridStore>& gridA, const Vector3i& ijkA, const shared_ptr<GridStore>& gridB, const Vector3i& ijkB){
	size_t aMin=0, aMax=gridA->size(ijkA);
	// if checking the same cell against itself, set bMin to -1,
	// that will make it start from the value of (a+1) at every iteration
	int bMin=((gridA.get()==gridB.get() && ijkA==ijkB)?-1:0); size_t bMax=gridB->size(ijkB);
	for(int a=aMin; a<aMax; a++){
		for(size_t b=(bMin<0?a+1:0); b<bMax; b++){
			Particle::id_t idA=gridA->get(ijkA,a), idB=gridB->get(ijkB,b);
			switch(what){
				case PROCESS_FULL: tryAddNewContact(idA,idB); break;
				default: throw std::logic_error("GridCollider::processCell: unhandled value of *what* ("+to_string(what)+")");
			}
		}
	}
}

void GridCollider::run(){
	dem=static_cast<DemField*>(field.get());
	// previous gridPrev is deleted automatically here
	gridPrev=gridCurr; 
	// create new gridCurr
	gridCurr=make_shared<GridStore>(dim,gridDense,/*locking*/true,exIniSize,exNumMaps);

	/* update grid bounding boxes */
	shared_ptr<GridCollider> shared_this=static_pointer_cast<GridCollider>(shared_from_this());
	if(around) shrink=around?cellSize.minCoeff()/2.:0.;
	for(const shared_ptr<Particle>& p: *(dem->particles)){
		assert(p);
		if(!p->shape) continue;
		gridBoundDispatcher->operator()(p->shape,p->id,shared_this);
	}
	/* contact search with history */
	if(false && gridPrev && gridCurr->isCompatible(gridPrev)){
		// do only relative computation of appeared/disappeared potential contacts
		if(!gridOld) gridOld=make_shared<GridStore>();
		if(!gridNew) gridNew=make_shared<GridStore>();
		gridCurr->computeRelativeComplements(*gridPrev,gridNew,gridOld);
	} else {
		/* contact search without history */
		// 
		size_t N=gridCurr->linSize();
		if(around){
			throw std::runtime_error("GridCollider.around==true: not yet implemented!");
			/* traverse the grid with 2-stride along all axes, and check for every strided cell:
				* contacts within itself
				* contacts with the central cell and all cells around
				* contacts of all cells "lower" between themselves
			*/
			//#ifdef WOO_OPENMP
			//	#pragma omp parallel for
			//#endif
		} else {
			#ifdef WOO_OPENMP
				#pragma omp parallel for
			#endif
			for(size_t lin=0; lin<N; lin++){
				Vector3i ijk=gridCurr->lin2ijk(lin);
				// check contacts between all particles in this cell
				processCell(PROCESS_FULL,gridCurr,ijk,gridCurr,ijk);
			}
		}
	}
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

		if(!occupancyRange){
			occupancyRange=make_shared<ScalarRange>();
			occupancyRange->label="occupancy";
		}
		if(gridCurr && gridCurr->sizes()==dim){
			// show cells which are used, with some color based on the number of entries in there
			shared_ptr<GridStore> store(gridCurr); // copy shared_ptr so that we don't crash if changed meanwhile
			if(store){
				Vector3i maxIjk=store->gridSize;
				for(Vector3i ijk=Vector3i::Zero(); ijk[0]<maxIjk[0]; ijk[0]++){
					if(ijk[0]>=store->gridSize[0]) break;
					for(ijk[1]=0; ijk[1]<maxIjk[1]; ijk[1]++){
						if(ijk[1]>=store->gridSize[1]) break;
						for(ijk[2]=0; ijk[2]<maxIjk[2]; ijk[2]++){
							if(ijk[2]>=store->gridSize[2]) break;
							size_t occupancy=store->size(ijk);
							if(occupancy==0) continue;
							GLUtils::AlignedBox(ijk2boxShrink(ijk,.1),occupancyRange->color(occupancy));
						}
					}
				}
			}
		}
	};
#endif
