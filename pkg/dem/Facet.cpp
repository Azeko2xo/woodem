#include<woo/pkg/dem/Facet.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<boost/range/algorithm/count_if.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Renderer.hpp>
#endif

WOO_PLUGIN(dem,(Facet)(Bo1_Facet_Aabb)(Cg2_Facet_Sphere_L6Geom)(Cg2_Facet_Facet_L6Geom)(Cg2_Facet_InfCylinder_L6Geom)(In2_Facet_ElastMat));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_Facet__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Facet_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_InfCylinder_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Bo1_Facet_Aabb__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_In2_Facet_ElastMat__CLASS_BASE_DOC);


CREATE_LOGGER(Facet);

#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Facet));
#endif

void Facet::selfTest(const shared_ptr<Particle>& p){
	if(!numNodesOk()) throw std::runtime_error("Facet #"+to_string(p->id)+": numNodesOk() failed (has "+to_string(nodes.size())+" nodes)");
	for(int i:{0,1,2}) if((nodes[i]->pos-nodes[(i+1)%3]->pos).squaredNorm()==0) throw std::runtime_error("Facet #"+to_string(p->id)+": nodes "+to_string(i)+" and "+to_string((i+1)%3)+" are coincident.");
	// check that we don't have contact with another Facet -- it is not forbidden but usually a bug
	size_t ffCon=boost::range::count_if(p->contacts,[&](const Particle::MapParticleContact::value_type& C)->bool{ return C.second->isReal() && dynamic_pointer_cast<Facet>(C.second->leakOther(p.get())->shape); });
	if(ffCon>0) LOG_WARN("Facet.selfTest: Facet #"<<p->id<<" has "<<ffCon<<" contacts with other facets. This is not per se an error though very likely unintended -- there is no functor to handle such contact and it will be uselessly recomputed in every step. Set both particles masks to have some DemField.loneMask bits and contact will not be created at all.");
}

void Facet::updateMassInertia(const Real& density) const {
	checkNodesHaveDemData();
	for(int i:{0,1,2}){
		auto& dyn(nodes[i]->getData<DemData>());
		dyn.mass=0;
		dyn.inertia=Vector3r::Zero();
	}
}


void Facet::asRaw(Vector3r& center, Real& radius, vector<Real>& raw) const {
	center=CompUtils::circumscribedCircleCenter(nodes[0]->pos,nodes[1]->pos,nodes[2]->pos);
	radius=(nodes[0]->pos-center).norm();
	// store nodal positions as raw data
	raw.resize(9);
	// use positions relative to the center so that translation of center translates the whole thing
	for(int i:{0,1,2}) for(int ax:{0,1,2}) raw[3*i+ax]=nodes[i]->pos[ax]-center[ax];
}

void Facet::setFromRaw(const Vector3r& center, const Real& radius, const vector<Real>& raw){
	Shape::setFromRaw_helper_checkRaw_makeNodes(raw,9);
	// radius is ignored
	for(int i:{0,1,2}) for(int ax:{0,1,2}) nodes[i]->pos[ax]=raw[3*i+ax]+center[ax];
}



Vector3r Facet::getNormal() const {
	assert(numNodesOk());
	return ((nodes[1]->pos-nodes[0]->pos).cross(nodes[2]->pos-nodes[0]->pos)).normalized();
}

Vector3r Facet::getCentroid() const {
	return (1/3.)*(nodes[0]->pos+nodes[1]->pos+nodes[2]->pos);
}

#ifdef WOO_OPENGL
Vector3r Facet::getGlVertex(int i) const{
	assert(i>=0 && i<=2);
	const auto& n=nodes[i];
	return n->pos+(n->hasData<GlData>()?n->getData<GlData>().dGlPos:Vector3r::Zero());

}
Vector3r Facet::getGlNormal() const{
	return (getGlVertex(1)-getGlVertex(0)).cross(getGlVertex(2)-getGlVertex(0)).normalized();
}

Vector3r Facet::getGlCentroid() const{
	return (1/3.)*(getGlVertex(0)+getGlVertex(1)+getGlVertex(2));
}
#endif

Real Facet::getArea() const {
	assert(numNodesOk());
	const Vector3r& A=nodes[0]->pos;
	const Vector3r& B=nodes[1]->pos;
	const Vector3r& C=nodes[2]->pos;
	return .5*((B-A).cross(C-A)).norm();
}

Real Facet::getPerimeterSq() const {
	assert(numNodesOk());
	return (nodes[1]->pos-nodes[0]->pos).squaredNorm()+(nodes[2]->pos-nodes[1]->pos).squaredNorm()+(nodes[0]->pos-nodes[2]->pos).squaredNorm();

}

std::tuple<Vector3r,Vector3r,Vector3r> Facet::getOuterVectors() const {
	assert(numNodesOk());
	// is not normalized
	Vector3r nn=(nodes[1]->pos-nodes[0]->pos).cross(nodes[2]->pos-nodes[0]->pos);
	return std::make_tuple((nodes[1]->pos-nodes[0]->pos).cross(nn),(nodes[2]->pos-nodes[1]->pos).cross(nn),(nodes[0]->pos-nodes[2]->pos).cross(nn));
}

vector<Vector3r> Facet::outerEdgeNormals() const{
	auto o=getOuterVectors();
	return {std::get<0>(o).normalized(),std::get<1>(o).normalized(),std::get<2>(o).normalized()};
}

#if 0
Vector3r Facet::getNearestTrianglePt(const Vector3r& pt, const Vector3r[3] pts){
	const Vector3r& A(pts[0]); const Vector3r& B(pts[1]); const Vector3r& C(pts[2]);
	Vector3r n0=((B-A).cross(C-A));
	Vector3r n=n0.normalized();
	Vector3r outVec[3]={(B-A).cross(n0),(C-B).cross(n),(A-C).cross(n)};
	short w=0;
	for(int i:{0,1,2}) 
}
#endif


Vector3r Facet::getNearestPt(const Vector3r& pt) const {
	// FIXME: actually no need to project, sign of the dot product will be the same regardless?!
	Vector3r fNormal=getNormal();
	Real planeDist=(pt-nodes[0]->pos).dot(fNormal);
	Vector3r fC=pt-planeDist*fNormal; // point projected to facet's plane
	Vector3r outVec[3];
	std::tie(outVec[0],outVec[1],outVec[2])=getOuterVectors();
	short w=0;
	for(int i:{0,1,2}) w|=(outVec[i].dot(fC-nodes[i]->pos)>0.?1:0)<<i;
	Vector3r contPt;
	switch(w){
		case 0: return fC; // ---: inside triangle
		case 1: return CompUtils::closestSegmentPt(fC,nodes[0]->pos,nodes[1]->pos); // +-- (n1)
		case 2: return CompUtils::closestSegmentPt(fC,nodes[1]->pos,nodes[2]->pos); // -+- (n2)
		case 4: return CompUtils::closestSegmentPt(fC,nodes[2]->pos,nodes[0]->pos); // --+ (n3)
		case 3: return nodes[1]->pos; // ++- (v1)
		case 5: return nodes[0]->pos; // +-+ (v0)
		case 6: return nodes[2]->pos; // -++ (v2)
		case 7: throw logic_error("Facet::getNearestPt: Impossible sphere-facet intersection (all points are outside the edges). (please report bug)"); // +++ (impossible)
		default: throw logic_error("Facet::getNearestPt: Nonsense intersection value. (please report bug)");
	}
}


std::tuple<Vector3r,Vector3r> Facet::interpolatePtLinAngVel(const Vector3r& x) const {
	assert(numNodesOk());
	Vector3r a=CompUtils::triangleBarycentrics(x,nodes[0]->pos,nodes[1]->pos,nodes[2]->pos);
	Vector3r vv[3]={nodes[0]->getData<DemData>().vel,nodes[1]->getData<DemData>().vel,nodes[2]->getData<DemData>().vel};
	Vector3r linVel=a[0]*vv[0]+a[1]*vv[1]+a[2]*vv[2];
	Vector3r angVel=(nodes[0]->pos-x).cross(vv[0])+(nodes[1]->pos-x).cross(vv[1])+(nodes[2]->pos-x).cross(vv[2]);
	if(fakeVel==Vector3r::Zero()) return std::make_tuple(linVel,angVel);
	else {
		// add fake velocity, unless the first component is NaN
		// which ignores fakeVel and reset in-plane velocities
		if(!isnan(fakeVel[0])) return std::make_tuple(linVel+fakeVel,angVel);
		// with isnan(fakeVel[0]), zero out in-plane components
		Vector3r fNormal=getNormal();
		linVel=linVel.dot(fNormal)*fNormal; 
		return std::make_tuple(linVel,angVel);
	}
}


void Bo1_Facet_Aabb::go(const shared_ptr<Shape>& sh){
	Facet& f=sh->cast<Facet>();
	if(!f.bound){ f.bound=make_shared<Aabb>(); /* ignore node rotation*/ sh->bound->cast<Aabb>().maxRot=-1;
	}
	Aabb& aabb=f.bound->cast<Aabb>();
	const Vector3r halfThickVec=Vector3r::Constant(f.halfThick);
	if(!scene->isPeriodic){
		aabb.min=f.nodes[0]->pos-halfThickVec;
		aabb.max=f.nodes[0]->pos+halfThickVec;
		for(int i:{1,2}){
			aabb.min=aabb.min.array().min((f.nodes[i]->pos-halfThickVec).array()).matrix();
			aabb.max=aabb.max.array().max((f.nodes[i]->pos+halfThickVec).array()).matrix();
		}
	} else {
		// periodic cell: unshear everything
		aabb.min=scene->cell->unshearPt(f.nodes[0]->pos)-halfThickVec;
		aabb.max=scene->cell->unshearPt(f.nodes[0]->pos)+halfThickVec;
		for(int i:{1,2}){
			Vector3r v=scene->cell->unshearPt(f.nodes[i]->pos);
			aabb.min=aabb.min.array().min((v-halfThickVec).array()).matrix();
			aabb.max=aabb.max.array().max((v+halfThickVec).array()).matrix();
		}
	}
}

CREATE_LOGGER(Cg2_Facet_Sphere_L6Geom);


void In2_Facet_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle, const bool skipContacts){
	// nothing to do
	if(skipContacts) return;
	auto& f=sh->cast<Facet>();
	for(const auto& I: particle->contacts){
		const shared_ptr<Contact>& C(I.second); if(!C->isReal()) continue;
		Vector3r F,T,xc;
		for(int i:{0,1,2}){
			std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/i,scene);
			F/=3.; T/=3.;
			f.nodes[i]->getData<DemData>().addForceTorque(F,xc.cross(F)+T);
		}
	}
};


bool Cg2_Facet_Sphere_L6Geom::go(const shared_ptr<Shape>& sh1, const shared_ptr<Shape>& sh2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Facet& f=sh1->cast<Facet>(); const Sphere& s=sh2->cast<Sphere>();
	const Vector3r& sC=s.nodes[0]->pos+shift2;
	Vector3r fNormal=f.getNormal();
	Real planeDist=(sC-f.nodes[0]->pos).dot(fNormal);
	// cerr<<"planeDist="<<planeDist<<endl;
	if(abs(planeDist)>(s.radius+f.halfThick) && !C->isReal() && !force) return false;
	Vector3r fC=sC-planeDist*fNormal; // sphere's center projected to facet's plane
	Vector3r outVec[3];
	std::tie(outVec[0],outVec[1],outVec[2])=f.getOuterVectors();
	Real ll[3]; // whether the projected center is on the outer or inner side of each side's line
	for(int i:{0,1,2}) ll[i]=outVec[i].dot(fC-f.nodes[i]->pos);
	short w=(ll[0]>0?1:0)+(ll[1]>0?2:0)+(ll[2]>0?4:0); // bitmask whether the closest point is outside (1,2,4 for respective edges)
	Vector3r contPt;
	switch(w){
		case 0: contPt=fC; break; // ---: inside triangle
		case 1: contPt=CompUtils::closestSegmentPt(fC,f.nodes[0]->pos,f.nodes[1]->pos); break; // +-- (n1)
		case 2: contPt=CompUtils::closestSegmentPt(fC,f.nodes[1]->pos,f.nodes[2]->pos); break; // -+- (n2)
		case 4: contPt=CompUtils::closestSegmentPt(fC,f.nodes[2]->pos,f.nodes[0]->pos); break; // --+ (n3)
		case 3: contPt=f.nodes[1]->pos; break; // ++- (v1)
		case 5: contPt=f.nodes[0]->pos; break; // +-+ (v0)
		case 6: contPt=f.nodes[2]->pos; break; // -++ (v2)
		case 7: throw logic_error("Cg2_Facet_Sphere_L3Geom: Impossible sphere-facet intersection (all points are outside the edges). (please report bug)"); // +++ (impossible)
		default: throw logic_error("Ig2_Facet_Sphere_L3Geom: Nonsense intersection value. (please report bug)");
	}
	Vector3r normal=sC-contPt; // normal is now the contact normal (not yet normalized)
	//cerr<<"sC="<<sC.transpose()<<", contPt="<<contPt.transpose()<<endl;
	//cerr<<"dist="<<normal.norm()<<endl;
	if(normal.squaredNorm()>pow(s.radius+f.halfThick,2) && !C->isReal() && !force) { return false; }
	Real dist=normal.norm();
	#define CATCH_NAN_FACET_SPHERE
	#ifdef CATCH_NAN_FACET_SPHERE
		if(dist==0) LOG_FATAL("dist==0.0 between Facet #"<<C->leakPA()->id<<" @ "<<f.nodes[0]->pos.transpose()<<", "<<f.nodes[1]->pos.transpose()<<", "<<f.nodes[2]->pos.transpose()<<" and Sphere #"<<C->leakPB()->id<<" @ "<<s.nodes[0]->pos.transpose()<<", r="<<s.radius);
		normal/=dist; // normal is normalized now
	#else
		// this tries to handle that
		if(dist!=0) normal/=dist; // normal is normalized now
		// zero distance (sphere's center sitting exactly on the facet):
		// use previous normal, or just unitX for new contacts (arbitrary, sorry)
		else normal=(C->geom?C->geom->node->ori*Vector3r::UnitX():Vector3r::UnitX());
	#endif
	if(f.halfThick>0) contPt+=normal*f.halfThick;
	Real uN=dist-s.radius-f.halfThick;
	// TODO: not yet tested!!
	Vector3r linVel,angVel;
	std::tie(linVel,angVel)=f.interpolatePtLinAngVel(contPt);
	#ifdef CATCH_NAN_FACET_SPHERE
		if(isnan(linVel[0])||isnan(linVel[1])||isnan(linVel[2])||isnan(angVel[0])||isnan(angVel[1])||isnan(angVel[2])) LOG_FATAL("NaN in interpolated facet velocity: linVel="<<linVel.transpose()<<", angVel="<<angVel.transpose()<<", contPt="<<contPt.transpose()<<"; particles Facet #"<<C->leakPA()->id<<" @ "<<f.nodes[0]->pos.transpose()<<", "<<f.nodes[1]->pos.transpose()<<", "<<f.nodes[2]->pos.transpose()<<" and Sphere #"<<C->leakPB()->id<<" @ "<<s.nodes[0]->pos.transpose()<<", r="<<s.radius)
	#endif
	const DemData& dyn2(s.nodes[0]->getData<DemData>()); // sphere
	// check if this will work when contact point == pseudo-position of the facet
	handleSpheresLikeContact(C,contPt,linVel,angVel,sC,dyn2.vel,dyn2.angVel,normal,contPt,uN,f.halfThick,s.radius);
	return true;
};

CREATE_LOGGER(Cg2_Facet_Facet_L6Geom);

bool Cg2_Facet_Facet_L6Geom::go(const shared_ptr<Shape>& sh1, const shared_ptr<Shape>& sh2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Facet& A=sh1->cast<Facet>(); const Facet& B=sh2->cast<Facet>();
	const Vector3r pp[2][3]={{A.nodes[0]->pos,A.nodes[1]->pos,A.nodes[2]->pos},{B.nodes[0]->pos+shift2,B.nodes[1]->pos+shift2,B.nodes[2]->pos+shift2}};
	// Vector3r nn[]={A.getNormal(),B.getNormal()};
	const Real triD=A.halfThick+B.halfThick;
	const Real triD2=pow(triD,2);
	Vector3r oo[2][3]; // outer normals
	Vector3r ee[2][3]; // edge unit vectors
	Vector3r em[2][3]; // edge midpoints
	Real     eh[2][3]; // edge half-lengths
	Vector3r nn[2]; // plane normals (normalized)
	for(short a:{0,1}){
		nn[a]=(pp[a][1]-pp[a][0]).cross(pp[a][2]-pp[a][0]).normalized(); // normalization needed?
		for(short e:{0,1,2}){
			ee[a][e]=pp[a][(e+1)%3]-pp[a][e];
			oo[a][e]=ee[a][e].cross(nn[a]);
			eh[a][e]=.5*ee[a][e].norm();
			ee[a][e]/=2*eh[a][e];
			em[a][e]=.5*(pp[a][(e+1)%3]+pp[a][e]);
		}
	}
	vector<Vector6r> close; close.reserve(6);
	// we must find some closest point, even if not overlapping
	// that is done only if there is nothing in *close*, which always have precendence
	bool must=(C->isReal() || force);
	Real minD2=Inf; Vector6r minPts=Vector6r::Zero();

	// find nearest points both ways, A-B and B-A
	// a is the facet considered as facet, b is the other one (vertices, edges)
	//LOG_TRACE("A: "<<pp[0][0].transpose()<<"; "<<pp[0][1].transpose()<<"; "<<pp[0][2].transpose());
	//LOG_TRACE("B: "<<pp[1][0].transpose()<<"; "<<pp[1][1].transpose()<<"; "<<pp[1][2].transpose());
	for(short a:{0,1}){
		short b=(a==0?1:0);
		// dist of vertices to the triangle
		for(short v:{0,1,2}){
			const Vector3r& p=pp[b][v]; // vertex of b
			short w=0;
			for(short i:{0,1,2}) w|=(oo[a][i].dot(p-pp[a][i])>0.?1:0)<<i;
			Vector3r c;
			switch(w){
				case 0: c=p-(p-pp[a][0]).dot(nn[a])*nn[a]; break; // ---: inside triangle, project onto the plane
				case 1: c=CompUtils::closestSegmentPt(p,pp[a][0],pp[a][1]); break; // +-- (n1)
				case 2: c=CompUtils::closestSegmentPt(p,pp[a][1],pp[a][2]); break; // -+- (n2)
				case 4: c=CompUtils::closestSegmentPt(p,pp[a][2],pp[a][0]); break; // --+ (n3)
				case 3: c=pp[a][1]; break; // ++- (v1)
				case 5: c=pp[a][0]; break; // +-+ (v0)
				case 6: c=pp[a][2]; break; // -++ (v2)
			}
			Real d2=(c-p).squaredNorm();
			Vector6r cc;
			//LOG_TRACE("ab="<<a<<b<<": w="<<w<<" d="<<sqrt(d2)<<" triD="<<triD<<"; c="<<c.transpose()<<", p="<<p.transpose());
			if (a==0) cc<<c,p; else cc<<p,c;
			if(d2<triD2){
				close.push_back(cc);
				//LOG_TRACE("added");
			} else {
				// if point is not being added to close, put it to minPts if necessary
				if(close.empty() && must && d2<=minD2){ minPts=cc; minD2=d2; }
			}
		}
	}
	// dist of edges to edges
	for(short e0:{0,1,2}){
		for(short e1:{0,1,2}){
			Vector2r st; bool parallel;
			Real d2=CompUtils::distSq_SegmentSegment(em[0][e0],ee[0][e0],eh[0][e0],em[1][e1],ee[1][e1],eh[1][e1],st,parallel);
			Vector6r c; c<<em[0][e0]+st[0]*ee[0][e0],em[1][e1]+st[1]*ee[1][e1];
			//LOG_TRACE("ee="<<e0<<e1<<": d="<<sqrt(d2)<<"; "<<c.transpose());
			// too far || vertex proximity was already detected above (in vertex-triangle tests)
			if(d2>=triD2 || max(abs(st[0]),abs(st[1]))>.99*max(eh[0][e0],eh[1][e1])){
				if(close.empty() && must && d2<=minD2){ minPts=c; minD2=d2; }
				continue;
			} else close.push_back(c);
		}
	}
	// handle close points
	//LOG_TRACE(close.size()<<" close points, must="<<must<<", minD2="<<minD2<<".");
	if(close.empty() && !must){
		//LOG_TRACE("---------------------------------------------------------");
		return false;
	}
	Vector3r ptA, ptB, contPt, normal;
	Real uN;

	if(close.empty () && must && isinf(minD2)) throw std::logic_error("Cf2_Facet_Facet_L6GeomL: minD2==Inf but the contact must be computed!");
	if(close.empty()){
		// use the closest point
		ptA=minPts.head<3>(); ptB=minPts.tail<3>();
		Real d=(ptB-ptA).norm();
		uN=d-triD;
		//LOG_TRACE("minPts: uN="<<uN<<"; "<<ptA.transpose()<<", "<<ptB.transpose());
		normal=(ptB-ptA)/d;
	} else {
		// weighted average
		Vector6r c=Vector6r::Zero(); Real wSum=0; Real minUn=Inf;
		for(const auto& v: close){
			Real d=(v.head<3>()-v.tail<3>()).norm();
			minUn=min(minUn,d-triD);
			Real w=d-triD; 
			//LOG_TRACE("Close: w="<<w<<", d="<<d<<"; "<<v.transpose());
			assert(w<=0);
			wSum+=w;
			c+=v*w;
		}
		c/=wSum;
		ptA=c.head<3>(); ptB=c.tail<3>();
		uN=minUn;
		//LOG_TRACE("Weighted: uN="<<uN<<"; "<<ptA.transpose()<<", "<<ptB.transpose());
		normal=(ptB-ptA).normalized();
		// LOG_TRACE((ptB-ptA).transposed()<<", |ptB-ptA|="<<(ptB-ptA).norm()<<", normal="<<(ptB-ptA).normalized());
	}

	// LOG_TRACE("ptA="<<ptA.transpose()<<", A.halfThick="<<A.halfThick<<", uN="<<uN<<", normal="<<normal);
	contPt=ptA+(A.halfThick+.5*uN)*normal;
	//LOG_TRACE("contPt "<<contPt<<", normal "<<normal);

	Vector3r linA,angA,linB,angB;
	std::tie(linA,angA)=A.interpolatePtLinAngVel(ptA);
	std::tie(linB,angB)=B.interpolatePtLinAngVel(ptB);
	handleSpheresLikeContact(C,ptA,linA,angA,ptB,linB,angB,normal,contPt,uN,A.halfThick,B.halfThick);
	//LOG_TRACE("===========================================================");
	return true;
}


bool Cg2_Facet_InfCylinder_L6Geom::go(const shared_ptr<Shape>& sh1, const shared_ptr<Shape>& sh2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Facet& f=sh1->cast<Facet>(); const InfCylinder& cyl=sh2->cast<InfCylinder>();
	const Vector3r& cylPos(cyl.nodes[0]->pos);
	const int& ax0=cyl.axis;
	const int ax1=(ax0+1)%3, ax2=(ax0+2)%3;
	const Vector2r P(cylPos[ax1],cylPos[ax2]);
	Vector2r vv2[3];
	for(short v:{0,1,2}) vv2[v]=Vector2r(f.nodes[v]->pos[ax1],f.nodes[v]->pos[ax2]);
	const Real dTouch=cyl.radius+f.halfThick;
	const Real dTouch2=pow(dTouch,2);
	Vector3r pp[3];
	Vector3r dd2;
	for(short i:{0,1,2}){
		const Vector2r& B=vv2[i];
		Vector2r M=vv2[(i+1)%3]-B;
		// http://www.geometrictools.com/Documentation/DistancePointLine.pdf
		Real t=CompUtils::clamped(M.dot(P-B)/M.squaredNorm(),0,1);
		// closest point
		Real d2=(P-(B+t*M)).squaredNorm();
		pp[i]=f.nodes[i]->pos+t*(f.nodes[(i+1)%3]->pos-f.nodes[i]->pos);
		dd2[i]=d2;
	}
	bool touch=(dd2.minCoeff()<dTouch2);
	if(!C->isReal() && !touch && !force) return false;
	
	Vector3r fP=Vector3r::Zero(); // facet contact point
	Real uN;
	
	if(!touch){
		// return the closest point, no interpolation at all
		int e; dd2.minCoeff(&e);
		fP=pp[e];
		uN=sqrt(dd2[e])-dTouch;
	} else {
		// interpolate using distances as weights
		Real wSum=0.; Real minUn=Inf;
		for(short e:{0,1,2}){
			if(dd2[e]>=dTouch2) continue;
			Real w=sqrt(dd2[e])-dTouch;
			wSum+=w;
			minUn=min(minUn,w);
			// cerr<<"uN=w="<<w<<", pp["<<e<<"]="<<pp[e].transpose()<<endl;
			fP+=w*pp[e];
		}
		uN=minUn;
		fP/=wSum;
	}

	Vector3r normal, contPt;
	normal=cylPos-fP; normal[cyl.axis]=0; normal.normalize();
	contPt=fP+(f.halfThick+.5*uN)*normal;
	// cerr<<"normal="<<normal.transpose()<<", uN="<<uN<<", contPt="<<contPt.transpose()<<endl;

	Vector3r vel,angVel;
	std::tie(vel,angVel)=f.interpolatePtLinAngVel(fP);
	const DemData& cylDyn(cyl.nodes[0]->getData<DemData>());
	handleSpheresLikeContact(C,fP,vel,angVel,cylPos,cylDyn.vel,cylDyn.angVel,normal,contPt,uN,f.halfThick,cyl.radius);

	return true;
}



#ifdef WOO_OPENGL
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/base/CompUtils.hpp>

void halfCylinder(const Vector3r& A, const Vector3r& B, Real radius, const Vector3r& upVec, const Vector3r& color, bool wire, int slices=12, int stacks=1, Real connectorAngle=0.){
	if(wire) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		glLineWidth(1);
	}
	if(!isnan(color[0]))	glColor3v(color);
	Real len=(B-A).norm();
	Vector3r axis=(B-A)/len;
	Matrix3r T; T.col(0)=axis; T.col(1)=upVec.cross(axis); T.col(2)=upVec;
	glPushMatrix();
		GLUtils::setLocalCoords(A,Quaternionr(T));
		slices=max(1,slices/2); stacks=max(1,stacks);
		auto makeNormCoords=[&](int sl, int st)->Vector3r{ return Vector3r((st*1./stacks),-sin(sl*1./slices*M_PI),-cos(sl*1./slices*M_PI)); };
		for(int stack=0; stack<stacks; stack++){
			glBegin(GL_QUAD_STRIP);
				for(int slice=0; slice<=slices; slice++){
					for(int dStack:{0,1}){
						Vector3r nc=makeNormCoords(slice,stack+dStack);
						if(!wire) glNormal3v(Vector3r(0,nc[1],nc[2]));
						glVertex3v(nc.cwiseProduct(Vector3r(len,radius,radius)).eval());
					}
				}
			glEnd();
		}
		// sphere wedge connector
		if(connectorAngle!=0){
			glTranslatev(Vector3r(len,0,0)); // move to the far end
			stacks=slices; // what were slices for cylinder are stacks for the sphere
			slices=stacks*(abs(connectorAngle+.01)/M_PI); // compute slices to make the sphere round; add .01 to avoid oscillation for pi/2 and such
			auto makeNormCoords=[&](int sl, int st)->Vector3r{ return Vector3r(sin(sl*1./slices*connectorAngle)*sin(st*1./stacks*M_PI),-cos(-sl*1./slices*connectorAngle)*sin(st*1./stacks*M_PI),-cos(st*1./stacks*M_PI)); };
			for(int slice=0; slice<slices; slice++){
				glBegin(GL_QUAD_STRIP);
					for(int stack=0; stack<=stacks; stack++){
						for(int dSlice:{0,1}){
							Vector3r nc=makeNormCoords(slice+dSlice,stack);
							if(!wire) glNormal3v(nc);
							glVertex3v((radius*nc).eval());
						}
					}
				glEnd();
			}
		}
	glPopMatrix();
	if(wire) glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

bool Gl1_Facet::wire;
int Gl1_Facet::slices;
int Gl1_Facet::wd;
Real Gl1_Facet::fastDrawLim;
#if 0
Vector2r Gl1_Facet::fuzz;
#endif

void Gl1_Facet::drawEdges(const Facet& f, const Vector3r& facetNormal, const Vector3r shifts[3], bool wire){
	if(slices>=4){
		// draw half-cylinders and caps
		Vector3r edges[]={(f.getGlVertex(1)-f.getGlVertex(0)).normalized(),(f.getGlVertex(2)-f.getGlVertex(1)).normalized(),(f.getGlVertex(0)-f.getGlVertex(2)).normalized()};
		for(int i:{0,1,2}){
			const Vector3r& A(f.getGlVertex(i)+shifts[i]), B(f.getGlVertex((i+1)%3)+shifts[i]);
			halfCylinder(A,B,f.halfThick,facetNormal,Vector3r(NaN,NaN,NaN),wire,slices,1,acos(edges[i].dot(edges[(i+1)%3])));
		}
	}else{
		// just the edges
		Vector3r dz=f.halfThick*facetNormal;
		if(wire){
			Vector3r A(f.getGlVertex(0)+shifts[0]), B(f.getGlVertex(1)+shifts[1]), C(f.getGlVertex(2)+shifts[2]);
			Vector3r vv[]={A+dz,B+dz,C+dz,A+dz,A-dz,B-dz,B+dz,B-dz,C-dz,C+dz,C-dz,A-dz};
			glLineWidth(wd);
			glBegin(GL_LINE_LOOP);
				for(const auto& v: vv) glVertex3v(v);
			glEnd();
		} else {
			glBegin(GL_QUADS);
				for(int edge:{0,1,2}){
					const Vector3r A(f.getGlVertex(edge)+shifts[edge]), B(f.getGlVertex((edge+1)%3)+shifts[(edge+1)%3]);
					glNormal3v((B-A).cross(facetNormal).normalized());
					Vector3r vv[]={A-dz,A+dz,B+dz,B-dz};
					for(const Vector3r& v: vv) glVertex3v(v);
				}
			glEnd();
		}
	}
}

void Gl1_Facet::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){   
	Facet& f=sh->cast<Facet>();

	// don't draw very small facets when doing fastDraw
	if(Renderer::fastDraw && f.getPerimeterSq()<pow(fastDrawLim*viewInfo.sceneRadius,2)) return;
	Vector3r shifts[3]={shift,shift,shift};
	if(scene->isPeriodic && f.nodes[0]->hasData<GlData>() && f.nodes[1]->hasData<GlData>() && f.nodes[2]->hasData<GlData>()){
		GlData* g[3]={&(f.nodes[0]->getData<GlData>()),&(f.nodes[1]->getData<GlData>()),&(f.nodes[2]->getData<GlData>())};
		// not all nodes shifted the same; in that case, check where the centroid is, and use that one
		if(!(g[0]->dCellDist==g[1]->dCellDist && g[1]->dCellDist==g[2]->dCellDist)){
			Vector3i dCell;
			scene->cell->canonicalizePt(f.getCentroid(),dCell);
			for(int i:{0,1,2}) shifts[i]+=scene->cell->intrShiftPos(g[i]->dCellDist-dCell);
		}
	}

	if(wire || wire2 || Renderer::fastDraw){
		glDisable(GL_LINE_SMOOTH);
		#if 0
			Vector3r shift2(shift);
			if(fuzz.minCoeff()>0){
				// shift point along the normal to avoid z-battles, based on facet address
				shift2+=normal*abs((f.nodes[0]->pos-f.nodes[1]->pos).maxCoeff())*(fuzz[0]/fuzz[1])*(((ptrdiff_t)(&f))%((int)fuzz[1]));
			}
		#endif
		if(f.halfThick==0 || slices<0 || Renderer::fastDraw){
			glLineWidth(wd);
			glBegin(GL_LINE_LOOP);
				for(int i:{0,1,2}) glVertex3v((f.getGlVertex(i)+shifts[i]).eval());
		   glEnd();
		} else {
			// draw noting inside, just the boundary
			drawEdges(f,f.getGlNormal(),shifts,true);
		}
		glEnable(GL_LINE_SMOOTH);
	} else {
		Vector3r normal=f.getGlNormal();
		// this makes every triangle different WRT the light direction; important for shading
		if(f.halfThick==0){
			glDisable(GL_CULL_FACE); 
			glBegin(GL_TRIANGLES);
				glNormal3v(normal);
				for(int i:{0,1,2}) glVertex3v((f.getGlVertex(i)+shifts[i]).eval());
			glEnd();
		} else {
			glDisable(GL_CULL_FACE); 
			glBegin(GL_TRIANGLES);
				glNormal3v(normal);
				for(int i:{0,1,2}) glVertex3v((f.getGlVertex(i)+1*normal*f.halfThick+shifts[i]).eval());
				glNormal3v((-normal).eval());
				for(int i:{0,2,1}) glVertex3v((f.getGlVertex(i)-1*normal*f.halfThick+shifts[i]).eval());
			glEnd();
			drawEdges(f,normal,shifts,false);
		}
	}
}

#endif /* WOO_OPENGL */

