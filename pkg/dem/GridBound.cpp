#include<woo/pkg/dem/GridBound.hpp>
WOO_PLUGIN(dem,(GridBound)(GridBoundFunctor)(GridBoundDispatcher)(Grid1_Sphere)(Grid1_Wall))
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_GridBound));
#endif


void GridBound::setNodePlay(const shared_ptr<Shape>& s, const Real& verletDist){
	const size_t N=s->nodes.size();
	nodePlay.resize(N);
	for(size_t i=0; i<N; i++){
		nodePlay[i].min()=s->nodes[i]->pos-verletDist*Vector3r::Ones();
		nodePlay[i].max()=s->nodes[i]->pos-verletDist*Vector3r::Ones();
	}
};

bool GridBound::insideNodePlay(const shared_ptr<Shape>& s){
	if(s->nodes.size()!=nodePlay.size()) return false;
	const size_t N=s->nodes.size();
	for(size_t i=0; i<N; i++) if(!nodePlay[i].contains(s->nodes[i]->pos)) return false;
	return true;
}



#include<woo/pkg/dem/GridCollider.hpp>
#include<woo/pkg/dem/Sphere.hpp>

void Grid1_Sphere::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll){
	Sphere& s=sh->cast<Sphere>();
	if(scene->isPeriodic) throw std::logic_error("Grid1_Sphere: PBC not handled (yet)");
	assert(dynamic_pointer_cast<GridBound>(s.bound));
	if(!s.bound){ s.bound=make_shared<GridBound>(); }
	GridBound& gb=s.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	//assert(s.numNodesOk());
	const Vector3r& pos=s.nodes[0]->pos;
	//Vector3r halfSize=(distFactor>0?distFactor:1.)*s.radius*Vector3r::Ones();
	Real radEff=max(s.radius-coll->shrink,0.)+(distFactor>1?(distFactor-1):0.)*s.radius+coll->verletDist;
	//if(!scene->isPeriodic){ gb.min=pos-halfSize; gb.max=pos+halfSize; return; }
	Vector3i ijk0=coll->xyz2ijk(pos-radEff*Vector3r::Ones());
	Vector3i ijk1=coll->xyz2ijk(pos+radEff*Vector3r::Ones());
	// don't go over actual grid dimensions
	ijk0=ijk0.cwiseMax(Vector3i::Zero());
	ijk1=ijk1.cwiseMin(coll->dim-Vector3i::Ones()); // the upper bound is inclusive
	for(Vector3i ijk=ijk0; ijk[0]<=ijk1[0]; ijk[0]++){
		for(ijk[1]=ijk0[1]; ijk[1]<=ijk1[1]; ijk[1]++){
			for(ijk[2]=ijk0[2]; ijk[2]<=ijk1[2]; ijk[2]++){
				if((coll->xyzNearXyz(pos,ijk)-pos).squaredNorm()>pow(radEff,2)) continue; // cell not touched by the sphere
				// add sphere to the cell
				coll->gridCurr->protected_append(ijk,id);
				#ifdef WOO_GRID_BOUND_DEBUG
					gb.cells.push_back(ijk);
				#endif
			}
		}
	}
	gb.setNodePlay(sh,coll->verletDist);
}

void Grid1_Wall::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll){
	Wall& w=sh->cast<Wall>();
	const int& ax(w.axis); int ax1=(ax+1)%3, ax2=(ax+2)%3;
	if(scene->isPeriodic) throw std::logic_error("Grid1_Wall: PBC not handled (yet)");
	assert(dynamic_pointer_cast<GridBound>(w.bound));
	if(!w.bound){ w.bound=make_shared<GridBound>(); }
	GridBound& gb=w.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	const Vector3r& pos=w.nodes[0]->pos;
	Vector3r posLo=pos, posHi=pos;
	posLo[ax]-=coll->verletDist; posHi[ax]+=coll->verletDist;
	Vector3i ijkLo=coll->xyz2ijk(posLo); Vector3i ijkHi=coll->xyz2ijk(posHi);
	Vector3i ijk;
	for(int c0=max(ijkLo[ax],0); c0<=min(ijkHi[ax],coll->dim[ax]-1); c0++){
		for(int c1=0; c1<coll->dim[ax1]; c1++){
			for(int c2=0; c2<coll->dim[ax2]; c2++){
				ijk[ax]=c0; ijk[ax1]=c1; ijk[ax2]=c2;
				coll->gridCurr->protected_append(ijk,id);
				#ifdef WOO_GRID_BOUND_DEBUG
					gb.cells.push_back(ijk);
				#endif
			}
		}
	}
	gb.setNodePlay(sh,coll->verletDist); 
	// can move as much as it wants in-plane
	gb.nodePlay[0].min()[ax1]=gb.nodePlay[0].min()[ax2]=-Inf;
	gb.nodePlay[0].max()[ax1]=gb.nodePlay[0].max()[ax2]=Inf;
}

