#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/dem/Sphere.hpp>

WOO_PLUGIN(dem,(Capsule)(Bo1_Capsule_Aabb)(Cg2_Wall_Capsule_L6Geom)(Cg2_Facet_Capsule_L6Geom)(Cg2_Capsule_Capsule_L6Geom));

WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Capsule_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Capsule_L6Geom__CLASS_BASE_DOC);
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Capsule));
#endif

/*
copied and adapted from http://www.geometrictools.com/LibMathematics/Distance/Wm5DistSegment3Segment3.cpp
(licensed under the Boost license) and adapted. Much appreciated!

The arguments provided are such that there is minimum compution in the routine itself; direction* arguments must be unit vectors.

The *st* vector returns parameters for the closest points; both s and t run from 0 to the SEGMENT LENGTH (not to 1).
*/
Real distSq_SegmentSegment(const Vector3r& center0, const Vector3r& direction0, const Real& halfLength0, const Vector3r& center1, const Vector3r& direction1, const Real& halfLength1, Vector2r& st, bool& parallel){
	Vector3r diff=center0-center1;
	const Real& extent0(halfLength0);
	const Real& extent1(halfLength1);
	Real a01=-direction0.dot(direction1);
	Real b0=diff.dot(direction0);
	Real b1=-diff.dot(direction1);
	Real c=diff.squaredNorm();
	Real det=abs(1.-a01*a01);
	Real& s0(st[0]);
	Real& s1(st[1]);
	Real sqrDist, extDet0, extDet1, tmpS0, tmpS1;
	#if 0
	    Real a01 = -direction0.Dot(direction1);
	    Real b0 = diff.Dot(direction0);
	    Real b1 = -diff.Dot(direction1);
	    Real c = diff.SquaredLength();
	    Real det = Math<Real>::FAbs((Real)1 - a01*a01);
	    Real s0, s1, sqrDist, extDet0, extDet1, tmpS0, tmpS1;
    if (det >= Math<Real>::ZERO_TOLERANCE)
	#endif
	if(det>=1e-8){
		parallel=false;
		// Segments are not parallel.
		s0 = a01*b1 - b0;
		s1 = a01*b0 - b1;
		extDet0 = extent0*det;
		extDet1 = extent1*det;

		if (s0 >= -extDet0)
		{
			if (s0 <= extDet0)
			{
				if (s1 >= -extDet1)
				{
					if (s1 <= extDet1)  // region 0 (interior)
					{
						// Minimum at interior points of segments.
						Real invDet = ((Real)1)/det;
						s0 *= invDet;
						s1 *= invDet;
						sqrDist = s0*(s0 + a01*s1 + ((Real)2)*b0) +
							s1*(a01*s0 + s1 + ((Real)2)*b1) + c;
					}
					else  // region 3 (side)
					{
						s1 = extent1;
						tmpS0 = -(a01*s1 + b0);
						if (tmpS0 < -extent0)
						{
							s0 = -extent0;
							sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
								s1*(s1 + ((Real)2)*b1) + c;
						}
						else if (tmpS0 <= extent0)
						{
							s0 = tmpS0;
							sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
						}
						else
						{
							s0 = extent0;
							sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
								s1*(s1 + ((Real)2)*b1) + c;
						}
					}
				}
				else  // region 7 (side)
				{
					s1 = -extent1;
					tmpS0 = -(a01*s1 + b0);
					if (tmpS0 < -extent0)
					{
						s0 = -extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
					else if (tmpS0 <= extent0)
					{
						s0 = tmpS0;
						sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
					}
					else
					{
						s0 = extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
				}
			}
			else
			{
				if (s1 >= -extDet1)
				{
					if (s1 <= extDet1)  // region 1 (side)
					{
						s0 = extent0;
						tmpS1 = -(a01*s0 + b1);
						if (tmpS1 < -extent1)
						{
							s1 = -extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
						else if (tmpS1 <= extent1)
						{
							s1 = tmpS1;
							sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
						}
						else
						{
							s1 = extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
					}
					else  // region 2 (corner)
					{
						s1 = extent1;
						tmpS0 = -(a01*s1 + b0);
						if (tmpS0 < -extent0)
						{
							s0 = -extent0;
							sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
								s1*(s1 + ((Real)2)*b1) + c;
						}
						else if (tmpS0 <= extent0)
						{
							s0 = tmpS0;
							sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
						}
						else
						{
							s0 = extent0;
							tmpS1 = -(a01*s0 + b1);
							if (tmpS1 < -extent1)
							{
								s1 = -extent1;
								sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
									s0*(s0 + ((Real)2)*b0) + c;
							}
							else if (tmpS1 <= extent1)
							{
								s1 = tmpS1;
								sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
							}
							else
							{
								s1 = extent1;
								sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
									s0*(s0 + ((Real)2)*b0) + c;
							}
						}
					}
				}
				else  // region 8 (corner)
				{
					s1 = -extent1;
					tmpS0 = -(a01*s1 + b0);
					if (tmpS0 < -extent0)
					{
						s0 = -extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
					else if (tmpS0 <= extent0)
					{
						s0 = tmpS0;
						sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
					}
					else
					{
						s0 = extent0;
						tmpS1 = -(a01*s0 + b1);
						if (tmpS1 > extent1)
						{
							s1 = extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
						else if (tmpS1 >= -extent1)
						{
							s1 = tmpS1;
							sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
						}
						else
						{
							s1 = -extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
					}
				}
			}
		}
		else 
		{
			if (s1 >= -extDet1)
			{
				if (s1 <= extDet1)  // region 5 (side)
				{
					s0 = -extent0;
					tmpS1 = -(a01*s0 + b1);
					if (tmpS1 < -extent1)
					{
						s1 = -extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
					else if (tmpS1 <= extent1)
					{
						s1 = tmpS1;
						sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
					}
					else
					{
						s1 = extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
				}
				else  // region 4 (corner)
				{
					s1 = extent1;
					tmpS0 = -(a01*s1 + b0);
					if (tmpS0 > extent0)
					{
						s0 = extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
					else if (tmpS0 >= -extent0)
					{
						s0 = tmpS0;
						sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
					}
					else
					{
						s0 = -extent0;
						tmpS1 = -(a01*s0 + b1);
						if (tmpS1 < -extent1)
						{
							s1 = -extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
						else if (tmpS1 <= extent1)
						{
							s1 = tmpS1;
							sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
						}
						else
						{
							s1 = extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
					}
				}
			}
			else   // region 6 (corner)
			{
				s1 = -extent1;
				tmpS0 = -(a01*s1 + b0);
				if (tmpS0 > extent0)
				{
					s0 = extent0;
					sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
						s1*(s1 + ((Real)2)*b1) + c;
				}
				else if (tmpS0 >= -extent0)
				{
					s0 = tmpS0;
					sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
				}
				else
				{
					s0 = -extent0;
					tmpS1 = -(a01*s0 + b1);
					if (tmpS1 < -extent1)
					{
						s1 = -extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
					else if (tmpS1 <= extent1)
					{
						s1 = tmpS1;
						sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
					}
					else
					{
						s1 = extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
				}
			}
		}
	}
	else
	{
		parallel=true;
		// The segments are parallel.  The average b0 term is designed to
		// ensure symmetry of the function.  That is, dist(seg0,seg1) and
		// dist(seg1,seg0) should produce the same number.
		Real e0pe1 = extent0 + extent1;
		Real sign = (a01 > (Real)0 ? (Real)-1 : (Real)1);
		Real b0Avr = ((Real)0.5)*(b0 - sign*b1);
		Real lambda = -b0Avr;
		if (lambda < -e0pe1)
		{
			lambda = -e0pe1;
		}
		else if (lambda > e0pe1)
		{
			lambda = e0pe1;
		}

		s1 = -sign*lambda*extent1/e0pe1;
		s0 = lambda + sign*s1;
		sqrDist = lambda*(lambda + ((Real)2)*b0Avr) + c;
	}
	
	#if 0
		mClosestPoint0 = center0 + s0*direction0;
		mClosestPoint1 = center1 + s1*direction1;
		mSegment0Parameter = s0;
		mSegment1Parameter = s1;
	#endif

	// Account for numerical round-off errors.
	if (sqrDist < (Real)0)
	{
		sqrDist = (Real)0;
	}
	return sqrDist;
}


void Capsule::selfTest(const shared_ptr<Particle>& p){
	if(!(radius>0.) || !(shaft>0.)) throw std::runtime_error("Capsule #"+to_string(p->id)+": both radius and shaft must be positive lengths (current: radius="+to_string(radius)+", shaft="+to_string(shaft)+").");
	if(!numNodesOk()) throw std::runtime_error("Capsule #"+to_string(p->id)+": numNodesOk() failed: must be 1, not "+to_string(nodes.size())+".");
}

Real Capsule::equivRadius() const {
	Real V=(4/3.)*M_PI*pow(radius,3)+M_PI*pow(radius,2)*shaft;
	return cbrt(V)*3/(4*M_PI);
}

void Capsule::updateMassInertia(const Real& density) const {
	checkNodesHaveDemData();
	auto& dyn=nodes[0]->getData<DemData>();
	Real r2(pow(radius,2)), r3(pow(radius,3));
	Real mCaps=(4/3.)*M_PI*r3*density;
	Real mShaft=M_PI*r2*shaft*density;
	Real distCap=.5*shaft+(3/8.)*radius; // distance between centroid and the cap's centroid
	Real Ix=2*mCaps*r2/5.+.5*mShaft*r2;
	Real Iy=(83/320.)*mCaps*r2+mCaps*pow(distCap,2)+(1/12.)*mShaft*(3*r2+pow(shaft,2));
	dyn.inertia=Vector3r(Ix,Iy,Iy);
	dyn.mass=mCaps+mShaft;
}

AlignedBox3r Capsule::alignedBox() const {
	const auto& pos(nodes[0]->pos); const auto& ori(nodes[0]->ori);
	Vector3r dShaft=ori*Vector3r(.5*shaft,0,0);
	AlignedBox3r ret;
	for(int a:{-1,1}) for(int b:{-1,1}) ret.extend(pos+a*dShaft+b*radius*Vector3r::Ones());
	return ret;
}

void Bo1_Capsule_Aabb::go(const shared_ptr<Shape>& sh){
	if(!sh->bound){ sh->bound=make_shared<Aabb>(); }
	const auto& c(sh->cast<Capsule>());
	AlignedBox3r ab=c.alignedBox();
	Aabb& aabb=sh->bound->cast<Aabb>();
	aabb.min=ab.min(); 
	aabb.max=ab.max();
}

void Cg2_Capsule_Capsule_L6Geom::setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C){
	const auto& c1(s1->cast<Capsule>()); const auto& c2(s2->cast<Capsule>());
	C->minDist00Sq=pow(c1.radius+.5*c1.shaft+c2.radius+.5*c2.shaft,2);
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
		else normal[ax]=C->geom->cast<L6Geom>().trsf.row(0)[ax]; // existing contacts (preserve previous)
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



bool Cg2_Capsule_Capsule_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const auto& c1(s1->cast<Capsule>()); const auto& c2(s2->cast<Capsule>());
	const Vector3r& pos1(c1.nodes[0]->pos); const Vector3r pos2(c2.nodes[0]->pos+shift2);
	const auto& ori1(c1.nodes[0]->ori); const auto& ori2(c2.nodes[0]->ori);
	const auto& dyn1(c1.nodes[0]->getData<DemData>()); const auto& dyn2(c2.nodes[0]->getData<DemData>());
	Vector3r dir1(ori1*Vector3r::UnitX()), dir2(ori2*Vector3r::UnitX());
	Real rSum=(c1.radius+c2.radius);
	Vector2r uv;
	bool parallel;
	Real distSq=distSq_SegmentSegment(pos1,dir1,.5*c1.shaft,pos2,dir2,.5*c2.shaft,uv,parallel);
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



bool Cg2_Facet_Capsule_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ return false; }

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
		glBegin(GL_LINE_STRIP);
			glVertex3v(Vector3r(-shaft/2.,0,0));
			glVertex3v(Vector3r( shaft/2.,0,0));
		glEnd();
		return;
	}
	static double clipPlaneA[4]={0,0,-1,0};
	static double clipPlaneB[4]={0,0,1,0};
	// 2nd rotation: rotate so that the OpenGL z-axis aligns with the local x-axis that way the glut sphere poles will be at the capsule's poles, looking prettier
	// 1st rotation: rotate around x so that vertices or segments of cylinder and half-spheres coindice
	Quaternionr axisRot=/* Quaternionr(AngleAxisr(M_PI/2.,Vector3r::UnitX()))* */Quaternionr(AngleAxisr(M_PI/2.,Vector3r::UnitY())); // XXX: to be fixed
	GLUtils::setLocalCoords(dPos+shape->nodes[0]->pos+shift,(dOri*shape->nodes[0]->ori*axisRot));
	int cylStacks(.5*(shaft/r)*quality*glutStacks); // approx the same dist as on the caps
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
#endif

