#ifndef WOO_NOCAPSULE

#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/dem/Sphere.hpp>

WOO_PLUGIN(dem,(Capsule)(Bo1_Capsule_Aabb)(Cg2_Sphere_Capsule_L6Geom)(Cg2_Wall_Capsule_L6Geom)(Cg2_Facet_Capsule_L6Geom)(Cg2_InfCylinder_Capsule_L6Geom)(Cg2_Capsule_Capsule_L6Geom));

WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Capsule_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_InfCylinder_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Sphere_Capsule_L6Geom__CLASS_BASE_DOC);
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Capsule));
#endif


void Capsule::selfTest(const shared_ptr<Particle>& p){
	if(!(radius>0.) || !(shaft>=0.)) throw std::runtime_error("Capsule #"+to_string(p->id)+": radius must be positive and shaft non-negative (current: radius="+to_string(radius)+", shaft="+to_string(shaft)+").");
	if(!numNodesOk()) throw std::runtime_error("Capsule #"+to_string(p->id)+": numNodesOk() failed: must be 1, not "+to_string(nodes.size())+".");
}

Real Capsule::volume() const {
	return (4/3.)*M_PI*pow(radius,3)+M_PI*pow(radius,2)*shaft;
}

Real Capsule::equivRadius() const {
	return cbrt(this->Capsule::volume()*3/(4*M_PI));
}

bool Capsule::isInside(const Vector3r& pt) const{
	Vector3r l(nodes[0]->glob2loc(pt));
	// cylinder test
	if(abs(l[0])<shaft/2.) return pow(l[1],2)+pow(l[2],2)<pow(radius,2);
	// cap test (distance to endpoint)
	return (Vector3r(l[0]<0?-shaft/2.:shaft/2.,0,0)-l).squaredNorm()<pow(radius,2);
}

void Capsule::applyScale(Real scale){ radius*=scale; shaft*=scale; }


void Capsule::lumpMassInertia(const shared_ptr<Node>& n, Real density, Real& mass, Matrix3r& I, bool& rotateOk){
	if(n.get()!=nodes[0].get()) return; // not our node
	rotateOk=false; // node may not be rotated without geometry change
	checkNodesHaveDemData();
	Real r2(pow(radius,2)), r3(pow(radius,3));
	Real mCaps=(4/3.)*M_PI*r3*density;
	Real mShaft=M_PI*r2*shaft*density;
	Real distCap=.5*shaft+(3/8.)*radius; // distance between centroid and the cap's centroid
	Real Ix=2*mCaps*r2/5.+.5*mShaft*r2;
	Real Iy=(83/320.)*mCaps*r2+mCaps*pow(distCap,2)+(1/12.)*mShaft*(3*r2+pow(shaft,2));
	I.diagonal()+=Vector3r(Ix,Iy,Iy);
	mass+=mCaps+mShaft;

}

#if 0
void Capsule::updateMassInertia(const Real& density) {
	Real m(0.); Matrix3r I(Matrix3r::Zero()); bool rotateOk;
	lumpMassInertia(nodes[0],density,m,I,rotateOk);
	// FIXME: only correct if this is the only particle in the node
}
#endif

AlignedBox3r Capsule::alignedBox() const {
	const auto& pos(nodes[0]->pos); const auto& ori(nodes[0]->ori);
	Vector3r dShaft=ori*Vector3r(.5*shaft,0,0);
	AlignedBox3r ret;
	for(int a:{-1,1}) for(int b:{-1,1}) ret.extend(pos+a*dShaft+b*radius*Vector3r::Ones());
	return ret;
}

void Capsule::asRaw(Vector3r& _center, Real& _radius, vector<shared_ptr<Node>>&nn, vector<Real>& raw) const {
	_center=nodes[0]->pos;
	_radius=radius+.5*shaft;
	// store as half-shaft vector
	// radius will be bounding sphere radius minus shaft/2
	raw.resize(3);
	Eigen::Map<Vector3r> hShaft(raw.data());
	hShaft=nodes[0]->ori*Vector3r(.5*shaft,0,0);
}

void Capsule::setFromRaw(const Vector3r& _center, const Real& _radius, vector<shared_ptr<Node>>& nn, const vector<Real>& raw) {
	Shape::setFromRaw_helper_checkRaw_makeNodes(raw,3);
	Eigen::Map<const Vector3r> hShaft(raw.data());
	nodes[0]->pos=_center;
	shaft=2*hShaft.norm();
	radius=_radius-.5*shaft;
	if(shaft>0.) nodes[0]->ori.setFromTwoVectors(Vector3r::UnitX(),hShaft.normalized());
	else nodes[0]->ori=Quaternionr::Identity();
	nn.push_back(nodes[0]);
}


void Bo1_Capsule_Aabb::go(const shared_ptr<Shape>& sh){
	if(!sh->bound){ sh->bound=make_shared<Aabb>(); /* consider node rotation*/ sh->bound->cast<Aabb>().maxRot=0.; }
	Aabb& aabb=sh->bound->cast<Aabb>();
	const auto& c(sh->cast<Capsule>());
	if(!scene->isPeriodic || !scene->cell->hasShear()){
		aabb.box=c.Capsule::alignedBox(); // non-virtual call
	} else {
		aabb.box.setEmpty();
		Vector3r extents=scene->cell->shearAlignedExtents(Vector3r::Constant(c.radius));
		// as if sphere for both endpoints
		for(int e:{0,1}){
			Vector3r ue=scene->cell->unshearPt(c.endPt(e));
			aabb.box.extend(ue+extents);
			aabb.box.extend(ue-extents);
		}
	}
}


bool Cg2_Wall_Capsule_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	if(scene->isPeriodic && scene->cell->hasShear()) throw std::logic_error("Cg2_Wall_Capsule_L6Geom does not handle periodic boundary conditions with skew (Scene.cell.trsf is not diagonal).");
	const auto& wall(s1->cast<Wall>()); const auto& cap(s2->cast<Capsule>());
	const auto& wallPos(wall.nodes[0]->pos);
	const auto& capNode(cap.nodes[0]);
	const Vector3r capPos(capNode->pos+shift2); const auto& capOri(capNode->ori);
	const DemData& wallDyn(wall.nodes[0]->getData<DemData>()); const DemData& capDyn(capNode->getData<DemData>());
	const int& ax=wall.axis; const int& sense=wall.sense;
	Vector3r AB[2]={capPos-capOri*Vector3r(cap.shaft/2.,0,0),capPos+capOri*Vector3r(cap.shaft/2.,0,0)};
	Vector2r sgnAxDiff(AB[0][ax]-wallPos[ax],AB[1][ax]-wallPos[ax]);
	// too far from each other
	if(!C->isReal() && (abs(sgnAxDiff[0])>cap.radius && abs(sgnAxDiff[1])>cap.radius) && !force) return false;
	Real cDist=capPos[ax]-wallPos[ax]; // centroid position decides which way to fly
	// normal vector
	Vector3r normal(Vector3r::Zero());
	if(sense==0){
		if(!C->geom){
			normal[ax]=cDist>0?1.:-1.;  // new contacts (depending on current position)
		}
		else normal[ax]=C->geom->cast<L6Geom>().trsf.col(0)[ax]; // existing contacts (preserve previous) // QQQ
	}
	else normal[ax]=(sense==1?1.:-1);

	// contact point
	Vector3r contPt;
	Real uN;
	// overlaps for both sides of the capsule
	Vector2r unun=sgnAxDiff*normal[ax]-Vector2r(cap.radius,cap.radius); 
	if(unun.prod()<0){
		// points on opposite sides of the wall
		int p=(unun[0]<0?0:1); // the point with overlap (uN<0) takes all
		contPt=AB[p];
		uN=unun[p];
	} else if(unun.sum()!=0.){
		// position according the level of overlap on each side, using weighted average
		Vector2r rel=unun/unun.sum();
		if(unun[0]>0) std::swap(rel[0],rel[1]); // inverse the weights if there is no overlap -- the one close to the wall gets more
		// alternative: rel=unun.normalized();
		assert(rel[0]>=0 && rel[1]>=0);
		contPt=rel[0]*AB[0]+rel[1]*AB[1];
		uN=rel[0]*unun[0]+rel[1]*unun[1];
	} else { // this is very unlikely
		contPt=.5*(AB[0]+AB[1]);
		uN=0.;
	}
	contPt[ax]=wallPos[ax];
	handleSpheresLikeContact(C,wallPos,wallDyn.vel,wallDyn.angVel,capPos,capDyn.vel,capDyn.angVel,normal,contPt,uN,/*r1*/-cap.radius,cap.radius);
	return true;
}


bool Cg2_InfCylinder_Capsule_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	if(scene->isPeriodic && scene->cell->hasShear()) throw std::logic_error("Cg2_InfCylinder_Capsule_L6Geom does not handle periodic boundary conditions with skew (Scene.cell.trsf is not diagonal).");
	const auto& cyl(s1->cast<InfCylinder>()); const auto& cap(s2->cast<Capsule>());
	const auto& cylPos(cyl.nodes[0]->pos);
	const auto& capNode(cap.nodes[0]);
	const Vector3r capPos(capNode->pos+shift2); const auto& capOri(capNode->ori);
	const DemData& cylDyn(cyl.nodes[0]->getData<DemData>()); const DemData& capDyn(capNode->getData<DemData>());
	const int& ax=cyl.axis; const int ax1((ax+1)%3), ax2((ax+2)%3);
	// do the computation in 2d
	Vector2r capPos2(capPos[ax1],capPos[ax2]);
	Vector3r capHalf=capOri*Vector3r(cap.shaft/2.,0,0);
	Vector2r capHalf2(capHalf[ax1],capHalf[ax2]);
	Vector2r cylPos2(cylPos[ax1],cylPos[ax2]);
	// see http://www.geometrictools.com/Documentation/DistancePointLine.pdf
	Real t=CompUtils::clamped(capHalf2.dot(cylPos2-capPos2)/capHalf2.squaredNorm(),-1,1);
	Vector2r cyl2seg2=(capPos2+t*capHalf2)-cylPos2; // relative position from cylinder to segment in 2d
	Real distSq=cyl2seg2.squaredNorm();
	if(!C->isReal() && distSq>pow(cyl.radius+cap.radius,2) && !force) return false;
	Vector3r normal=Vector3r::Zero(); normal[ax1]=cyl2seg2[0]; normal[ax2]=cyl2seg2[1]; normal.normalize();
	Real uN=sqrt(distSq)-(cyl.radius+cap.radius);
	Vector3r contPt=cylPos; contPt[ax]=(capPos+t*capHalf)[ax]; contPt+=(cyl.radius+.5*uN)*normal;
	handleSpheresLikeContact(C,cylPos,cylDyn.vel,cylDyn.angVel,capPos,capDyn.vel,capDyn.angVel,normal,contPt,uN,cyl.radius,cap.radius);
	return true;
}

void Cg2_Sphere_Capsule_L6Geom::setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C){
	const auto& s(s1->cast<Sphere>()); const auto& c(s2->cast<Capsule>());
	C->minDist00Sq=pow(s.radius+c.radius+.5*c.shaft,2);
}


bool Cg2_Sphere_Capsule_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const auto& sphere(s1->cast<Sphere>()); const auto& cap(s2->cast<Capsule>());
	const auto& spherePos(sphere.nodes[0]->pos);
	const auto& capNode(cap.nodes[0]);
	const Vector3r capPos(capNode->pos+shift2); const auto& capOri(capNode->ori);
	const DemData& sphereDyn(sphere.nodes[0]->getData<DemData>()); const DemData& capDyn(capNode->getData<DemData>());
	Real rSum(sphere.radius+cap.radius);
	Vector3r AB[2]={capPos-capOri*Vector3r(cap.shaft/2.,0,0),capPos+capOri*Vector3r(cap.shaft/2.,0,0)};
	Vector3r pt=CompUtils::closestSegmentPt(spherePos,AB[0],AB[1]);
	Real distSq=(pt-spherePos).squaredNorm();
	if(!C->isReal() && distSq>pow(rSum,2) && !force) return false;
	Real dist=sqrt(distSq);
	Vector3r normal=(pt-spherePos)/dist;
	Real uN=dist-rSum;
	Vector3r contPt=spherePos+normal*(sphere.radius+.5*uN);
	handleSpheresLikeContact(C,spherePos,sphereDyn.vel,sphereDyn.angVel,capPos,capDyn.vel,capDyn.angVel,normal,contPt,uN,sphere.radius,cap.radius);
	return true;
}


void Cg2_Capsule_Capsule_L6Geom::setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C){
	const auto& c1(s1->cast<Capsule>()); const auto& c2(s2->cast<Capsule>());
	C->minDist00Sq=pow(c1.radius+.5*c1.shaft+c2.radius+.5*c2.shaft,2);
}

bool Cg2_Capsule_Capsule_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const auto& c1(s1->cast<Capsule>()); const auto& c2(s2->cast<Capsule>());
	const Vector3r& pos1(c1.nodes[0]->pos); const Vector3r pos2(c2.nodes[0]->pos+shift2);
	const auto& ori1(c1.nodes[0]->ori); const auto& ori2(c2.nodes[0]->ori);
	const auto& dyn1(c1.nodes[0]->getData<DemData>()); const auto& dyn2(c2.nodes[0]->getData<DemData>());
	Vector3r dir1(ori1*Vector3r::UnitX()), dir2(ori2*Vector3r::UnitX());
	Real rSum=(c1.radius+c2.radius);
	Vector2r uv;
	bool parallel;
	Real distSq=CompUtils::distSq_SegmentSegment(pos1,dir1,.5*c1.shaft,pos2,dir2,.5*c2.shaft,uv,parallel);
	if(!C->isReal() && distSq>pow(rSum,2) && !force) return false;


	/* now check all endpoints against the other's segment, to detect if the capsules cross, or if they touch in the elongated direction, in which case we have to interpolate between ends just like in the wall contact (here, things are more complicated, as there are two capsules).

	The trick is to use the closest distance for overlap and normal computation, but endpoint overlaps for weighted contact point position. This should ensure smooth transition for all possible configurations without jump.
	*/
	//cerr<<"segment dist "<<sqrt(distSq)<<", capsule gap "<<sqrt(distSq)-rSum*rSum<<endl;
	//cerr<<"parameters: "<<uv.transpose()<<endl;
	Vector3r closePts[2]={pos1+dir1*uv[0],pos2+dir2*uv[1]};
	//cerr<<"segment point 1: "<<closePts[0]<<endl;
	//cerr<<"segment point 2: "<<closePts[1]<<endl;
	Vector3r ends[2][2]={{pos1-.5*c1.shaft*dir1,pos1+.5*c1.shaft*dir1},{pos2-.5*c2.shaft*dir2,pos2+.5*c2.shaft*dir2}};
	Real dist=sqrt(distSq);
	Real uN=dist-(c1.radius+c2.radius);
	Vector3r normal=(closePts[1]-closePts[0])/dist;
	Vector3r contPt;

	int bitsOver=0; // bitmap for overlaps
	short nOver=0; // number of overlaps - count of set bits (perhaps not the fastest, but does the job)
	Vector3r eClose[2][2];
	Real eDistSq[2][2];
	for(int a:{0,1}){
		int b((a+1)%2);
		for(int e:{0,1}){
			eClose[a][e]=CompUtils::closestSegmentPt(ends[a][e],ends[b][0],ends[b][1]);
			eDistSq[a][e]=(eClose[a][e]-ends[a][e]).squaredNorm();
			if(eDistSq[a][e]<pow(rSum,2)){ bitsOver|=1<<(2*a+e); nOver++; }
		}
	}
	Real r1r=c1.radius/rSum, r2r=c2.radius/rSum;
	if(nOver<2){
		contPt=closePts[0]+(c1.radius+0.5*uN)*normal;
	}
	else if(nOver==4){
		/* average all four points with their weights */
		//cerr<<"============"<<endl;
		Vector3r pp=Vector3r::Zero(); Real ww=0.;
		for(short ix0:{0,1}) for(short ix1:{0,1}){
			Real rel=(ix0==0?r2r:r1r);
			Real w=rSum-sqrt(eDistSq[ix0][ix1]);
			Vector3r p=eClose[ix0][ix1]+(ends[ix0][ix1]-eClose[ix0][ix1])*rel;
			//cerr<<"["<<ix0<<","<<ix1<<"]: p="<<p<<", w="<<w<<", rel"<<"="<<rel<<endl;
			//cerr<<"close "<<eClose[ix0][ix1]<<endl<<"end "<<ends[ix0][ix1]<<endl;
			pp+=w*p;
			ww+=w;
		}
		contPt=pp/ww;
		//cerr<<"contact point "<<contPt<<endl;
	} else {
		assert(nOver==2 || nOver==3);
		short iii[16][2][2]={
			{{0,0},{0,0}}, /* 0; unused */
			{{0,0},{0,0}}, /* 1; unused */
			{{0,0},{0,0}}, /* 2; unused */
			{{0,0},{0,1}}, /* 3=1+2 */
			{{0,0},{0,0}}, /* 4; unused  */
			{{0,0},{1,0}}, /* 5=1+4 */
			{{0,1},{1,0}}, /* 6=2+4 */
			{{0,0},{0,1}}, /* 7=1+2+4: just like 1+2 */
			{{0,0},{0,0}}, /* 8; unused  */
			{{0,0},{1,1}}, /* 9=1+8 */
			{{0,1},{1,1}}, /* A=10=2+8 */
			{{0,0},{0,1}}, /* B=11=1+2+8; just like 1+2 */
			{{1,0},{1,1}}, /* C=12=4+8 */
			{{1,0},{1,1}}, /* D=13=1+4+8; just like 4+8 */
			{{1,0},{1,1}}, /* E=14=2+4+8; just like 4+8 */
			{{0,0},{0,0}}  /* F=1+2+4+8; unused */
		};
		const auto& ix(iii[bitsOver]);
		Vector3r pts[2];
		Vector2r weights;
		//cerr<<"============"<<endl;
		//cerr<<"nOver="<<nOver<<", bitsOver="<<bitsOver<<endl;
		for(int i:{0,1}){
			const auto& ix0(ix[i][0]); const auto& ix1(ix[i][1]);
			Real rel=(ix0==0?r2r:r1r);
			pts[i]=eClose[ix0][ix1]+(ends[ix0][ix1]-eClose[ix0][ix1])*rel;
			weights[i]=rSum-sqrt(eDistSq[ix0][ix1]);
			//cerr<<"indices "<<ix0<<","<<ix1<<endl;
			//cerr<<"endpoint "<<ends[ix0][ix1]<<", close point "<<eClose[ix0][ix1]<<endl;
			//cerr<<"pts["<<i<<"]="<<pts[i]<<", weights["<<i<<"]="<<weights[i]<<endl;
			assert(weights[i]>0);
		}
		contPt=(weights[0]*pts[0]+weights[1]*pts[1])/weights.sum();
	}

	handleSpheresLikeContact(C,pos1,dyn1.vel,dyn1.angVel,pos2,dyn2.vel,dyn2.angVel,normal,contPt,uN,c1.radius,c2.radius);
	return true;
}


WOO_IMPL_LOGGER(Cg2_Facet_Capsule_L6Geom);
bool Cg2_Facet_Capsule_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const auto& facet(s1->cast<Facet>()); const auto& cap(s2->cast<Capsule>());
	// const auto& wallPos(wall.nodes[0]->pos);
	const auto& capNode(cap.nodes[0]);
	const Vector3r capPos(capNode->pos+shift2); const auto& capOri(capNode->ori);
	const DemData& capDyn(capNode->getData<DemData>());
	Vector3r fNormal=facet.getNormal();
	Vector3r AB[2]={capPos-capOri*Vector3r(cap.shaft/2.,0,0),capPos+capOri*Vector3r(cap.shaft/2.,0,0)};
	const Vector3r& f0pos(facet.nodes[0]->pos);
	typedef Eigen::Array<Real,2,1> Array2r;
	Array2r planeDists;
	for(int i:{0,1}) planeDists[i]=(AB[i]-f0pos).dot(fNormal);
	bool mayFail=(!C->isReal() && !force);
	// points on the same side and too far from the plane
	Real touchDist=facet.halfThick+cap.radius;
	if(mayFail && (std::signbit(planeDists[0])==std::signbit(planeDists[1])) && planeDists.abs().minCoeff()>touchDist){
		// cerr<<"Too far from the plane: "<<planeDists.transpose()<<endl;
		return false;
	}
	// compute points on the facet which are the closest to the segment ends
	Vector3r ffp[2]={facet.getNearestPt(AB[0]),facet.getNearestPt(AB[1])};
	// find points on the segment closest to those points (project back onto the segment)
	Array2r rp; // relative positions on the segment
	Vector3r ccp[2]={CompUtils::closestSegmentPt(ffp[0],AB[0],AB[1],&rp[0]),CompUtils::closestSegmentPt(ffp[1],AB[0],AB[1],&rp[1])};
	Array2r fcd2((ccp[0]-ffp[0]).squaredNorm(),(ccp[1]-ffp[1]).squaredNorm());
	if(mayFail && fcd2.minCoeff()>pow(touchDist,2)){
		#if 0
			cerr<<"===================================================="<<endl;
			cerr<<"  Too far from the triangle: "<<fcd2.transpose()<<endl;
			cerr<<"  Segment endpoints: "<<AB[0].transpose()<<"; "<<AB[1].transpose()<<endl;
			cerr<<"  Triangle points: "<<ffp[0].transpose()<<"; "<<ffp[1].transpose()<<endl;
			cerr<<"  Segment points: "<<ccp[0].transpose()<<"; "<<ccp[1].transpose()<<endl;
		#endif
		return false; // too far from the triangle
	}
	Array2r fcd=fcd2.sqrt();
	// penetration is the geometrical max
	Real uN=fcd.minCoeff()-touchDist;
	// normal vector and contact point are geometry-dependent
	Vector3r normal, contPt;
	// if one of the points is beyond physical touch, do no interpolation and take the close one
	// (if both are beyond, then take the closer one anyway as the contact must be computed;
	// the case of too far was handled above already)
	if(fcd.maxCoeff()>touchDist){
		// cerr<<"[1]";
		Array2r::Index ix;
		fcd.minCoeff(&ix);
		normal=(ccp[ix]-ffp[ix]).normalized();
		contPt=ffp[ix]+(facet.halfThick+0.5*uN)*normal;
	} else {
		//cerr<<"[2]";
		Array2r weights(fcd[0]-touchDist,fcd[1]-touchDist);
		weights/=weights.sum();
		// if(weights.maxCoeff()>0) LOG_ERROR("weights are positive but should not be: "<<weights.transpose());
		normal=(weights[0]*(ccp[0]-ffp[0])+weights[1]*(ccp[1]-ffp[1])).normalized(); // interpolated normal; is this not a nonsense?!
		#if 0
			contPt=weights[0]*ccp[0]+weights[1]*ccp[1]; // point on the segment line weighted by distance
			contPt+=-normal*(cap.radius+.5*uN);
		#else
			contPt=weights[0]*ffp[0]+weights[1]*ffp[1]+(facet.halfThick+.5*uN)*normal;
		#endif
		// interpolate between two points; this also handles the case when both
		#if 0
		cerr<<"===================================================="<<endl;
		cerr<<"  Segment endpoints: "<<AB[0].transpose()<<"; "<<AB[1].transpose()<<endl;
		cerr<<"  Triangle points: "<<ffp[0].transpose()<<"; "<<ffp[1].transpose()<<endl;
		cerr<<"  Segment points: "<<ccp[0].transpose()<<"; "<<ccp[1].transpose()<<endl;
		cerr<<"  normal: "<<normal<<endl;
		cerr<<"  weights: "<<weights.transpose()<<endl;
		cerr<<"  contPt: "<<contPt.transpose()<<endl;
		#endif
	}

	Vector3r linVel,angVel;
	std::tie(linVel,angVel)=facet.interpolatePtLinAngVel(contPt);
	handleSpheresLikeContact(C,contPt,linVel,angVel,capPos,capDyn.vel,capDyn.angVel,normal,contPt,uN,/*r1*/facet.halfThick,cap.radius);
	return true;
}

#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/lib/opengl/GLUtils.hpp>

void Gl1_Capsule::go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2,const GLViewInfo& glInfo){
	const shared_ptr<Node>& n=shape->nodes[0];
	Vector3r dPos=(n->hasData<GlData>()?n->getData<GlData>().dGlPos:Vector3r::Zero());
	Quaternionr dOri=(n->hasData<GlData>()?n->getData<GlData>().dGlOri:Quaternionr::Identity());
	const auto& capsule(shape->cast<Capsule>());
	Real r=scale*capsule.radius;
	Real shaft=scale*capsule.shaft;
	bool doPoints=(Renderer::fastDraw || quality<0 || (int)(quality*glutSlices)<2 || (int)(quality*glutStacks)<2);
	if(doPoints){
		if(smooth) glEnable(GL_POINT_SMOOTH);
		else glDisable(GL_POINT_SMOOTH);
		GLUtils::setLocalCoords(dPos+shape->nodes[0]->pos+shift,(dOri*shape->nodes[0]->ori));
		glPointSize(1.);
		if(shaft>0){
			glBegin(GL_LINE_STRIP);
				glVertex3v(Vector3r(-shaft/2.,0,0));
				glVertex3v(Vector3r( shaft/2.,0,0));
			glEnd();
		} else {
			glBegin(GL_POINTS);
			glVertex3v(Vector3r(0,0,0));
			glEnd();
		}
		return;
	}
	static double clipPlaneA[4]={0,0,-1,0};
	static double clipPlaneB[4]={0,0,1,0};
	// 2nd rotation: rotate so that the OpenGL z-axis aligns with the local x-axis that way the glut sphere poles will be at the capsule's poles, looking prettier
	// 1st rotation: rotate around x so that vertices or segments of cylinder and half-spheres coindice
	Quaternionr axisRot=/* Quaternionr(AngleAxisr(M_PI/2.,Vector3r::UnitX()))* */Quaternionr(AngleAxisr(M_PI/2.,Vector3r::UnitY())); // XXX: to be fixed
	GLUtils::setLocalCoords(dPos+shape->nodes[0]->pos+shift,(dOri*shape->nodes[0]->ori*axisRot));
	int cylStacks(max(1.,.5*(shaft/r)*quality*glutStacks)); // approx the same dist as on the caps
	if (wire || wire2 ){
		glLineWidth(1.);
		if(!smooth) glDisable(GL_LINE_SMOOTH);
		GLUtils::Cylinder(Vector3r(0,0,-shaft/2.),Vector3r(0,0,shaft/2.),r,/*color: keep current*/Vector3r(NaN,NaN,NaN),/*wire*/true,/*caps*/false,r,quality*glutSlices,/*stacks*/cylStacks);
		glEnable(GL_CLIP_PLANE0);
		glTranslatef(0,0,-shaft/2.); glClipPlane(GL_CLIP_PLANE0,clipPlaneA); glutWireSphere(r,quality*glutSlices,quality*glutStacks);
		glTranslatef(0,0,shaft);     glClipPlane(GL_CLIP_PLANE0,clipPlaneB); glutWireSphere(r,quality*glutSlices,quality*glutStacks);
		glDisable(GL_CLIP_PLANE0);
		if(!smooth) glEnable(GL_LINE_SMOOTH); // re-enable
	}
	else {
		glEnable(GL_LIGHTING);
		glShadeModel(GL_SMOOTH);
		GLUtils::Cylinder(Vector3r(0,0,-shaft/2.),Vector3r(0,0,shaft/2.),r,/*color: keep current*/Vector3r(NaN,NaN,NaN),/*wire*/false,/*caps*/false,r,quality*glutSlices,/*stacks*/cylStacks);
		glEnable(GL_CLIP_PLANE0);
		glTranslatef(0,0,-shaft/2.); glClipPlane(GL_CLIP_PLANE0,clipPlaneA); glutSolidSphere(r,quality*glutSlices,quality*glutStacks);
		glTranslatef(0,0,shaft);     glClipPlane(GL_CLIP_PLANE0,clipPlaneB); glutSolidSphere(r,quality*glutSlices,quality*glutStacks);
		glDisable(GL_CLIP_PLANE0);
	}
}
#endif /* WOO_OPENGL */


#endif /* WOO_NOCAPSULE */
