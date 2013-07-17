#include<woo/pkg/dem/GridBound.hpp>
WOO_PLUGIN(dem,(GridBound)(GridBoundFunctor)(GridBoundDispatcher)(Grid1_Sphere)(Grid1_Wall))
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_GridBound));
#endif


void GridBound::setNodePlay(const shared_ptr<Shape>& s, const Real& verletDist){
	const size_t N=s->nodes.size();
	nodePlay.resize(N);
	for(size_t i=0; i<N; i++){
		nodePlay[i]=AlignedBox3r(s->nodes[i]->pos-verletDist*Vector3r::Ones(),s->nodes[i]->pos+verletDist*Vector3r::Ones());
	}
};

bool GridBound::insideNodePlay(const shared_ptr<Shape>& s) const {
	if(s->nodes.size()!=nodePlay.size()) return false;
	const size_t N=s->nodes.size();
	for(size_t i=0; i<N; i++) if(!nodePlay[i].contains(s->nodes[i]->pos)) return false;
	return true;
}



#include<woo/pkg/dem/GridCollider.hpp>
#include<woo/pkg/dem/Sphere.hpp>

bool Grid1_Sphere::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll, const shared_ptr<GridStore>& grid, const bool& force){
	Sphere& s=sh->cast<Sphere>();
	if(scene->isPeriodic) throw std::logic_error("Grid1_Sphere: PBC not handled (yet)");
	if(!s.bound){ s.bound=make_shared<GridBound>(); }
	else if(!force && s.bound->cast<GridBound>().insideNodePlay(sh)) return false;
	assert(dynamic_pointer_cast<GridBound>(s.bound));
	GridBound& gb=s.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	//assert(s.numNodesOk());
	const Vector3r& pos=s.nodes[0]->pos;
	//Vector3r halfSize=(distFactor>0?distFactor:1.)*s.radius*Vector3r::Ones();
	Real radEff=max(s.radius-coll->shrink,0.)+(distFactor>1?(distFactor-1):0.)*s.radius+coll->verletDist;
	//if(!scene->isPeriodic){ gb.min=pos-halfSize; gb.max=pos+halfSize; return; }
	Vector3i ijk0=grid->xyz2ijk(pos-radEff*Vector3r::Ones());
	Vector3i ijk1=grid->xyz2ijk(pos+radEff*Vector3r::Ones());
	// don't go over actual grid dimensions
	ijk0=ijk0.cwiseMax(Vector3i::Zero());
	ijk1=ijk1.cwiseMin(grid->sizes()-Vector3i::Ones()); // the upper bound is inclusive
	// it is possible that shape is entirely outside of the grid -- keep track of whether we modified any cell at all
	//bool hit=false;
	for(Vector3i ijk=ijk0; ijk[0]<=ijk1[0]; ijk[0]++){
		for(ijk[1]=ijk0[1]; ijk[1]<=ijk1[1]; ijk[1]++){
			for(ijk[2]=ijk0[2]; ijk[2]<=ijk1[2]; ijk[2]++){
				if((grid->xyzNearXyz(pos,ijk)-pos).squaredNorm()>pow(radEff,2)) continue; // cell not touched by the sphere
				// add sphere to the cell
				grid->protected_append(ijk,id);
				//hit=true;
				#ifdef WOO_GRID_BOUND_DEBUG
					gb.cells.push_back(ijk);
				#endif
			}
		}
	}
	//if(!hit) return false;
	gb.setNodePlay(sh,coll->verletDist);
	return true;
}

bool Grid1_Wall::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll, const shared_ptr<GridStore>& grid, const bool& force){
	Wall& w=sh->cast<Wall>();
	const int& ax(w.axis); int ax1=(ax+1)%3, ax2=(ax+2)%3;
	if(scene->isPeriodic) throw std::logic_error("Grid1_Wall: PBC not handled (yet)");
	if(!w.bound){ w.bound=make_shared<GridBound>(); }
	else if(!force && w.bound->cast<GridBound>().insideNodePlay(sh)) return false;
	assert(dynamic_pointer_cast<GridBound>(w.bound));
	GridBound& gb=w.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	const Vector3r& pos=w.nodes[0]->pos;
	Vector3r posLo=pos, posHi=pos;
	Real verletDist=(movable?coll->verletDist:0.);
	if(!movable && sh->nodes[0]->getData<DemData>().vel[ax]!=0.) throw std::runtime_error("Grid1_Wall: #"+to_string(id)+" has non-zero ("+to_string(sh->nodes[0]->getData<DemData>().vel[ax])+") but Grid1_Wall.movable==False (set to True to have verletDist added to Wall's grid).");
	posLo[ax]-=verletDist; posHi[ax]+=verletDist;
	Vector3i ijkLo=grid->xyz2ijk(posLo); Vector3i ijkHi=grid->xyz2ijk(posHi);
	Vector3i ijk;
	Vector3i sizes=grid->sizes();
	// it is possible that shape is entirely outside of the grid -- keep track of whether we modified any cell at all
	//bool hit=false;
	for(int c0=max(ijkLo[ax],0); c0<=min(ijkHi[ax],sizes[ax]-1); c0++){
		for(int c1=0; c1<sizes[ax1]; c1++){
			for(int c2=0; c2<sizes[ax2]; c2++){
				ijk[ax]=c0; ijk[ax1]=c1; ijk[ax2]=c2;
				grid->protected_append(ijk,id);
				//hit=true;
				#ifdef WOO_GRID_BOUND_DEBUG
					gb.cells.push_back(ijk);
				#endif
			}
		}
	}
	//if(!hit) return false;
	gb.setNodePlay(sh,verletDist); 
	// can move as much as it wants in-plane
	gb.nodePlay[0].min()[ax1]=gb.nodePlay[0].min()[ax2]=-Inf;
	gb.nodePlay[0].max()[ax1]=gb.nodePlay[0].max()[ax2]=Inf;
	return true;
}

