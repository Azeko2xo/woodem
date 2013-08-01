#include<woo/pkg/dem/GridBound.hpp>
WOO_PLUGIN(dem,(GridBound)(GridBoundFunctor)(GridBoundDispatcher)(Grid1_Sphere)(Grid1_Wall)(Grid1_InfCylinder)(Grid1_Facet))
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

void GridBound::setNodePlay_box0(const shared_ptr<Shape>& s, const AlignedBox3r& box){
	assert(s->nodes.size()==1);
	nodePlay.resize(1);
	nodePlay[0]=box;
}

bool GridBound::insideNodePlay(const shared_ptr<Shape>& s) const {
	if(s->nodes.size()!=nodePlay.size()) return false;
	const size_t N=s->nodes.size();
	for(size_t i=0; i<N; i++) if(!nodePlay[i].contains(s->nodes[i]->pos)) return false;
	return true;
}



#include<woo/pkg/dem/GridCollider.hpp>
#include<woo/pkg/dem/Sphere.hpp>

void Grid1_Sphere::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll, const shared_ptr<GridStore>& grid){
	if(scene->isPeriodic) throw std::logic_error("Grid1_Sphere: PBC not handled (yet)");
	Sphere& s=sh->cast<Sphere>();
	if(!s.bound){ s.bound=make_shared<GridBound>(); }
	assert(dynamic_pointer_cast<GridBound>(s.bound));
	GridBound& gb=s.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	const Vector3r& pos=s.nodes[0]->pos;
	const auto& dyn(s.nodes[0]->getData<DemData>());
	const Real& verletDist(coll->verletDist);
	const int& verletSteps(coll->verletSteps);
	// includes shrinkage (if applicable) and distFactor
	Real radius=max(s.radius-coll->shrink,0.)+(distFactor>1?(distFactor-1):0.)*s.radius;
	// this is the new branch
	bool velSweep=(verletSteps>0 && (dyn.vel*verletSteps*scene->dt).squaredNorm()>pow(verletDist,2));
	if(velSweep){
		AlignedBox3r nodeBox, bbox;
		for(const Vector3r& p:{pos,(pos+dyn.vel*scene->dt*verletSteps).eval()}){
			nodeBox.extend(p);
			bbox.extend(p+Vector3r::Ones()*radius); bbox.extend(p-Vector3r::Ones()*radius);
		}
		AlignedBox3i ijkBox=grid->xxyyzz2iijjkk(bbox);
		// don't go over actual grid dimensions
		grid->clamp(ijkBox);
		Vector3i& ijk0(ijkBox.min()); Vector3i& ijk1(ijkBox.max());
		for(Vector3i ijk=ijk0; ijk[0]<=ijk1[0]; ijk[0]++){
			for(ijk[1]=ijk0[1]; ijk[1]<=ijk1[1]; ijk[1]++){
				for(ijk[2]=ijk0[2]; ijk[2]<=ijk1[2]; ijk[2]++){
					//if((grid->xyzNearXyz(pos,ijk)-pos).squaredNorm()>pow(radEff,2)) continue; 
					if(grid->boxCellDistSq(nodeBox,ijk)>pow(radius,2)) continue; // cell not touched by the sphere
					// add sphere to the cell
					grid->protected_append(ijk,id);
					#ifdef WOO_GRID_BOUND_DEBUG
						gb.cells.push_back(ijk);
					#endif
				}
			}
		}
		gb.setNodePlay_box0(sh,nodeBox);
	} else {
		AlignedBox3r bbox;
		Real radiusVerletDist=radius+verletDist;
		for(int sgn:{-1,+1}) bbox.extend(pos+sgn*Vector3r::Ones()*(radiusVerletDist));
		AlignedBox3i ijkBox=grid->xxyyzz2iijjkk(bbox);
		Vector3i& ijk0(ijkBox.min()); Vector3i& ijk1(ijkBox.max());
		grid->clamp(ijkBox);
		for(Vector3i ijk=ijk0; ijk[0]<=ijk1[0]; ijk[0]++){
			for(ijk[1]=ijk0[1]; ijk[1]<=ijk1[1]; ijk[1]++){
				for(ijk[2]=ijk0[2]; ijk[2]<=ijk1[2]; ijk[2]++){
					if((grid->xyzNearXyz(pos,ijk)-pos).squaredNorm()>pow(radiusVerletDist,2)) continue; 
					// add sphere to the cell
					grid->protected_append(ijk,id);
					#ifdef WOO_GRID_BOUND_DEBUG
						gb.cells.push_back(ijk);
					#endif
				}
			}
		}
		gb.setNodePlay(sh,verletDist);
	}
}

void Grid1_Wall::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll, const shared_ptr<GridStore>& grid){
	if(scene->isPeriodic) throw std::logic_error("Grid1_Wall: PBC not supported.");
	Wall& w=sh->cast<Wall>();
	const int& ax(w.axis); int ax1=(ax+1)%3, ax2=(ax+2)%3;
	if(!w.bound){ w.bound=make_shared<GridBound>(); }
	assert(dynamic_pointer_cast<GridBound>(w.bound));
	GridBound& gb=w.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	const Vector3r& pos=w.nodes[0]->pos;
	Vector3r posLo=pos, posHi=pos;
	Real verletDist=(movable?coll->verletDist:0.);
	if(!movable && sh->nodes[0]->getData<DemData>().vel[ax]!=0.) throw std::runtime_error("Grid1_Wall: #"+to_string(id)+" has non-zero ("+to_string(sh->nodes[0]->getData<DemData>().vel[ax])+") velocity but Grid1_Wall.movable==False (set to True to have verletDist added to Wall's grid).");
	posLo[ax]-=verletDist; posHi[ax]+=verletDist;
	Vector3i ijkLo=grid->xyz2ijk(posLo); Vector3i ijkHi=grid->xyz2ijk(posHi);
	Vector3i ijk;
	Vector3i sizes=grid->sizes();
	for(int c0=max(ijkLo[ax],0); c0<=min(ijkHi[ax],sizes[ax]-1); c0++){
		for(int c1=0; c1<sizes[ax1]; c1++){
			for(int c2=0; c2<sizes[ax2]; c2++){
				ijk[ax]=c0; ijk[ax1]=c1; ijk[ax2]=c2;
				grid->protected_append(ijk,id);
				#ifdef WOO_GRID_BOUND_DEBUG
					gb.cells.push_back(ijk);
				#endif
			}
		}
	}
	gb.setNodePlay(sh,verletDist); 
	// can move as much as it wants in-plane
	gb.nodePlay[0].min()[ax1]=gb.nodePlay[0].min()[ax2]=-Inf;
	gb.nodePlay[0].max()[ax1]=gb.nodePlay[0].max()[ax2]=Inf;
}


void Grid1_InfCylinder::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll, const shared_ptr<GridStore>& grid){
	if(scene->isPeriodic) throw std::logic_error("Grid1_InfCylinder: PBC not supported.");
	InfCylinder& c=sh->cast<InfCylinder>();
	const int& ax(c.axis); int ax1=(ax+1)%3, ax2=(ax+2)%3;
	if(!c.bound){ c.bound=make_shared<GridBound>(); }
	assert(dynamic_pointer_cast<GridBound>(c.bound));
	GridBound& gb=c.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	const Vector3r& pos=c.nodes[0]->pos;
	Vector3r posLo=pos, posHi=pos;
	Real verletDist=(movable?coll->verletDist:0.);
	if(!movable && sh->nodes[0]->getData<DemData>().vel.squaredNorm()!=0.) throw std::runtime_error("Grid1_InfCylinder: #"+to_string(id)+" has non-zero ("+to_string(sh->nodes[0]->getData<DemData>().vel.norm())+") velocity but Grid1_InfCylinder.movable==False (set to True to have verletDist added to InfCylinder's grid).");
	posLo[ax1]-=c.radius+verletDist; posLo[ax2]-=c.radius+verletDist;
	posHi[ax1]+=c.radius+verletDist; posHi[ax2]+=c.radius+verletDist;
	posLo[ax]-=verletDist; posHi[ax]+=verletDist;
	Vector3i ijkLo=grid->xyz2ijk(posLo); Vector3i ijkHi=grid->xyz2ijk(posHi);
	Vector3i ijk;
	Vector3i sizes=grid->sizes();
	for(int c0=0; c0<sizes[ax]; c0++){
		for(int c1=max(ijkLo[ax1],0); c1<=min(ijkHi[ax1],sizes[ax1]-1); c1++){
			for(int c2=max(ijkLo[ax2],0); c2<min(ijkHi[ax2],sizes[ax2]-1); c2++){
				ijk[ax]=c0; ijk[ax1]=c1; ijk[ax2]=c2;
				Vector3r dr=grid->xyzNearXyz(pos,ijk)-pos; // pos[ax] won't influence the result
				if(Vector2r(dr[ax1],dr[ax2]).squaredNorm()>pow(c.radius+verletDist,2)) continue; // cell not touched by the cylinder
				grid->protected_append(ijk,id);
				#ifdef WOO_GRID_BOUND_DEBUG
					gb.cells.push_back(ijk);
				#endif
			}
		}
	}
	gb.setNodePlay(sh,verletDist); 
	// can move as much as it wants along the axis
	gb.nodePlay[0].min()[ax]=-Inf;
	gb.nodePlay[0].max()[ax]=Inf;
}


void Grid1_Facet::go(const shared_ptr<Shape>& sh, const Particle::id_t& id, const shared_ptr<GridCollider>& coll, const shared_ptr<GridStore>& grid){
	if(scene->isPeriodic) throw std::logic_error("Grid1_Facet: PBC not supported.");
	Facet& f=sh->cast<Facet>();
	if(!f.bound){ f.bound=make_shared<GridBound>(); }
	assert(dynamic_pointer_cast<GridBound>(f.bound));
	GridBound& gb=f.bound->cast<GridBound>();
	#ifdef WOO_GRID_BOUND_DEBUG
		gb.cells.clear();
	#endif
	// smuggle halfThick into verletDist
	Real verletDist=(movable?coll->verletDist:0.)+f.halfThick;
	for(int i:{0,1,2}) if(!movable && sh->nodes[i]->getData<DemData>().vel.squaredNorm()!=0.) throw std::runtime_error("Grid1_Facet: #"+to_string(id)+" has non-zero ("+to_string(sh->nodes[i]->getData<DemData>().vel.norm())+") velocity but Grid1_Facet.movable==False (set to True to have verletDist added to Facet's grid).");
	Eigen::AlignedBox<int,3> box;
	for(int i:{0,1,2}){
		box.extend(grid->xyz2ijk(f.nodes[i]->pos+(verletDist*Vector3r::Ones())));
		box.extend(grid->xyz2ijk(f.nodes[i]->pos-(verletDist*Vector3r::Ones())));
	}
	box.clamp(Eigen::AlignedBox<int,3>(Vector3i(0,0,0),grid->sizes()-Vector3i::Ones()));
	for(int i=box.min()[0]; i<=box.max()[0]; i++){
		for(int j=box.min()[1]; j<=box.max()[1]; j++){
			for(int k=box.min()[2]; k<=box.max()[2]; k++){
				// this is quite imprecise
				Vector3i ijk(i,j,k);
				Vector3r mid=grid->ijk2boxMiddle(ijk);
				Vector3r p=f.getNearestPt(mid);
				Real diag_2=.5*grid->cellSize.norm();
				if((p-mid).norm()>(diag_2+verletDist)) continue; // cell center too far from the facet
				grid->protected_append(ijk,id);
				#ifdef WOO_GRID_BOUND_DEBUG
					gb.cells.push_back(ijk);
				#endif
			}
		}
	}
	gb.setNodePlay(sh,verletDist); 
}


