#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/core/Field.hpp>
#include<woo/lib/base/CompUtils.hpp>

#include<sstream>

#ifdef WOO_OPENGL
	#include<woo/lib/opengl/OpenGLWrapper.hpp>
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<GL/glu.h>
#endif

WOO_PLUGIN(dem,(L6Geom)(Cg2_Any_Any_L6Geom__Base)(Cg2_Sphere_Sphere_L6Geom)(Cg2_Facet_Sphere_L6Geom)(Cg2_Wall_Sphere_L6Geom)(Cg2_InfCylinder_Sphere_L6Geom)(Cg2_Truss_Sphere_L6Geom));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_L6Geom__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Any_Any_L6Geom__Base__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Sphere_Sphere_L6Geom__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_InfCylinder_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Truss_Sphere_L6Geom__CLASS_BASE_DOC);

#if 0
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_L6Geom));
#endif
#endif

void L6Geom::setInitialLocalCoords(const Vector3r& locX){
	// initial local y-axis orientation, in the xz or xy plane, depending on which component is larger to avoid singularities
	Vector3r locY=locX.cross(abs(locX[1])<abs(locX[2])?Vector3r::UnitY():Vector3r::UnitZ()); locY-=locX*locY.dot(locX); locY.normalize();
	Vector3r locZ=locX.cross(locY);
	// set our data
	#ifdef L6_TRSF_QUATERNION
		Matrix3r T; T.row(0)=locX; T.row(1)=locY; T.row(2)=locZ;
		trsf=Quaternionr(T); // from transformation matrix
	#else
		trsf.row(0)=locX; trsf.row(1)=locY; trsf.row(2)=locZ;
	#endif
}




void Cg2_Sphere_Sphere_L6Geom::setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C){
	C->minDist00Sq=pow(abs(distFactor*(s1->cast<Sphere>().radius+s2->cast<Sphere>().radius)),2);
}


bool Cg2_Sphere_Sphere_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Real& r1=s1->cast<Sphere>().radius; const Real& r2=s2->cast<Sphere>().radius;
	assert(s1->numNodesOk()); assert(s2->numNodesOk());
	assert(s1->nodes[0]->hasData<DemData>()); assert(s2->nodes[0]->hasData<DemData>());
	const DemData& dyn1(s1->nodes[0]->getData<DemData>());	const DemData& dyn2(s2->nodes[0]->getData<DemData>());

	Vector3r relPos=s2->nodes[0]->pos+shift2-s1->nodes[0]->pos;
	Real unDistSq=relPos.squaredNorm()-pow(abs(distFactor)*(r1+r2),2);
	if (unDistSq>0 && !C->isReal() && !force) return false;

	// contact exists, go ahead

	Real dist=relPos.norm();
	Real uN=dist-(r1+r2);
	Vector3r normal=relPos/dist;
	Vector3r contPt=s1->nodes[0]->pos+(r1+0.5*uN)*normal;

	handleSpheresLikeContact(C,s1->nodes[0]->pos,dyn1.vel,dyn1.angVel,s2->nodes[0]->pos+shift2,dyn2.vel,dyn2.angVel,normal,contPt,uN,r1,r2);

	return true;

};

CREATE_LOGGER(Cg2_Facet_Sphere_L6Geom);

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
	#define CATCH_NAN_FACET
	#ifdef CATCH_NAN_FACET
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
	#ifdef CATCH_NAN_FACET
		if(isnan(linVel[0])||isnan(linVel[1])||isnan(linVel[2])||isnan(angVel[0])||isnan(angVel[1])||isnan(angVel[2])) LOG_FATAL("NaN in interpolated facet velocity: linVel="<<linVel.transpose()<<", angVel="<<angVel.transpose()<<", contPt="<<contPt.transpose()<<"; particles Facet #"<<C->leakPA()->id<<" @ "<<f.nodes[0]->pos.transpose()<<", "<<f.nodes[1]->pos.transpose()<<", "<<f.nodes[2]->pos.transpose()<<" and Sphere #"<<C->leakPB()->id<<" @ "<<s.nodes[0]->pos.transpose()<<", r="<<s.radius)
	#endif
	const DemData& dyn2(s.nodes[0]->getData<DemData>()); // sphere
	// check if this will work when contact point == pseudo-position of the facet
	handleSpheresLikeContact(C,contPt,linVel,angVel,sC,dyn2.vel,dyn2.angVel,normal,contPt,uN,f.halfThick,s.radius);
	return true;
};

CREATE_LOGGER(Cg2_Wall_Sphere_L6Geom);

bool Cg2_Wall_Sphere_L6Geom::go(const shared_ptr<Shape>& sh1, const shared_ptr<Shape>& sh2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	if(scene->isPeriodic && scene->cell->hasShear()) throw std::logic_error("Cg2_Wall_Sphere_L6Geom does not handle periodic boundary conditions with skew (Scene.cell.trsf is not diagonal).");
	const Wall& wall=sh1->cast<Wall>(); const Sphere& sphere=sh2->cast<Sphere>();
	assert(wall.numNodesOk()); assert(sphere.numNodesOk());
	const Real& radius=sphere.radius; const int& ax=wall.axis; const int& sense=wall.sense;
	const Vector3r& wallPos=wall.nodes[0]->pos; Vector3r spherePos=sphere.nodes[0]->pos+shift2;
	Real dist=spherePos[ax]-wallPos[ax]; // signed "distance" between centers
	if(!C->isReal() && abs(dist)>radius && !force) { return false; }// wall and sphere too far from each other
	// contact point is sphere center projected onto the wall
	Vector3r contPt=spherePos; contPt[ax]=wallPos[ax];
	Vector3r normal=Vector3r::Zero();
	// wall interacting from both sides: normal depends on sphere's position
	assert(sense==-1 || sense==0 || sense==1);
	if(sense==0){
		// for new contacts, normal given by the sense of approaching the wall
		if(!C->geom) normal[ax]=dist>0?1.:-1.; 
		// for existing contacts, use the previous normal 
		else normal[ax]=C->geom->cast<L6Geom>().trsf.row(0)[ax];
	}
	else normal[ax]=(sense==1?1.:-1);
	Real uN=normal[ax]*dist-radius; // takes in account sense, radius and distance

	// this may not happen anymore as per conditions above
	assert(!(C->geom && C->geom->cast<L6Geom>().trsf.row(0)!=normal.transpose()));
	#if 0
		// check that the normal did not change orientation (would be abrupt here)
		if(C->geom && C->geom->cast<L6Geom>().trsf.row(0)!=normal.transpose()){
			throw std::logic_error((boost::format("Cg2_Wall_Sphere_L6Geom: normal changed from %s to %s in Wall+Sphere ##%d+%d (with Wall.sense=0, a particle might cross the Wall plane if Δt is too high, repulsive force to small or velocity too high.")%C->geom->cast<L6Geom>().trsf.row(0)%normal.transpose()%C->leakPA()->id%C->leakPB()->id).str());
		}
	#endif

	const DemData& dyn1(wall.nodes[0]->getData<DemData>());const DemData& dyn2(sphere.nodes[0]->getData<DemData>());
	handleSpheresLikeContact(C,wallPos,dyn1.vel,dyn1.angVel,spherePos,dyn2.vel,dyn2.angVel,normal,contPt,uN,/*r1*/-radius,radius);
	return true;
};

CREATE_LOGGER(Cg2_InfCylinder_Sphere_L6Geom);

bool Cg2_InfCylinder_Sphere_L6Geom::go(const shared_ptr<Shape>& sh1, const shared_ptr<Shape>& sh2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	if(scene->isPeriodic && scene->cell->hasShear()) throw std::logic_error("Cg2_InfCylinder_Sphere_L6Geom does not handle periodic boundary conditions with skew (Scene.cell.trsf is not diagonal).");
	const InfCylinder& cyl=sh1->cast<InfCylinder>(); const Sphere& sphere=sh2->cast<Sphere>();
	assert(cyl.numNodesOk()); assert(sphere.numNodesOk());
	const Real& sphRad=sphere.radius;
	const Real& cylRad=cyl.radius; const int& ax=cyl.axis;
	const Vector3r& cylPos=cyl.nodes[0]->pos; Vector3r sphPos=sphere.nodes[0]->pos+shift2;
	Vector3r relPos=sphPos-cylPos;
	relPos[ax]=0.;
	if(!C->isReal() && relPos.squaredNorm()>pow(cylRad+sphRad,2) && !force){ return false; }
	Real dist=relPos.norm();
	#ifdef CATCH_NAN_FACET
		if(dist==0.) LOG_FATAL("dist==0.0 between InfCylinder #"<<C->leakPA()->id<<" @ "<<cyl.nodes[0]->pos.transpose()<<", r="<<cylRad<<" and Sphere #"<<C->leakPB()->id<<" @ "<<sphere.nodes[0]->pos.transpose()<<", r="<<sphere.radius);
	#else
		#error You forgot to implement dist==0 handler with InfCylinder
	#endif
	Real uN=dist-(cylRad+sphRad);
	Vector3r normal=relPos/dist;
	Vector3r cylPosAx=cylPos; cylPosAx[ax]=sphPos[ax]; // projected
	Vector3r contPt=cylPosAx+(cylRad+0.5*uN)*normal;

	const DemData& cylDyn(cyl.nodes[0]->getData<DemData>());	const DemData& sphDyn(sphere.nodes[0]->getData<DemData>());
	// check impossible rotations of the infinite cylinder
	assert(cylDyn.angVel[(ax+1)%3]==0. && cylDyn.angVel[(ax+2)%3]==0.);
	handleSpheresLikeContact(C,cylPos,cylDyn.vel,cylDyn.angVel,sphPos,sphDyn.vel,sphDyn.angVel,normal,contPt,uN,cylRad,sphRad);
	return true;
};

bool Cg2_Truss_Sphere_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Truss& t=s1->cast<Truss>(); const Sphere& s=s2->cast<Sphere>();
	assert(s1->numNodesOk()); assert(s2->numNodesOk());
	assert(s1->nodes[0]->hasData<DemData>()); assert(s2->nodes[0]->hasData<DemData>());
	const DemData& dynTruss(t.nodes[0]->getData<DemData>());
	const DemData& dynSphere(s.nodes[0]->getData<DemData>());
	Real normAxisPos;

	Vector3r cylAxisPt=CompUtils::closestSegmentPt(s.nodes[0]->pos+shift2,t.nodes[0]->pos,t.nodes[1]->pos,&normAxisPos);
	Vector3r relPos=s.nodes[0]->pos-cylAxisPt;
	Real unDistSq=relPos.squaredNorm()-pow(s.radius+t.radius,2); // no distFactor here

	// TODO: find closest point of sharp-cut ends, remove this error
	if(!(t.caps|Truss::CAP_A) || !(t.caps|Truss::CAP_B)) throw std::runtime_error("Cg2_Sphere_Truss_L6Geom: only handles capped trusses for now.");

	if (unDistSq>0 && !C->isReal() && !force) return false;

	// contact exists, go ahead
	Real dist=relPos.norm();
	Real uN=dist-(s.radius+t.radius);
	Vector3r normal=relPos/dist;
	Vector3r contPt=s.nodes[0]->pos-(s.radius+0.5*uN)*normal;

	#if 0
		Vector3r cylPtVel=(1-normAxisPos)*dynTruss.vel+normAxisPos*dynTruss.vel; // average velocity
		// angular velocity of the contact point, exclusively due to linear velocity of endpoints
		Vector3r cylPtAngVel=dynTruss.vel.cross(t.nodes[0]->pos-contPt)+dynTruss.vel.cross(t.nodes[1]->pos-contPt);
	#endif

	handleSpheresLikeContact(C,cylAxisPt,dynTruss.vel,dynTruss.angVel,s.nodes[0]->pos+shift2,dynSphere.vel,dynSphere.angVel,normal,contPt,uN,t.radius,s.radius);

	return true;
};

/*
Generic function to compute L6Geom, used for {sphere,facet,wall}+sphere contacts

NB. the vel2 should be given WITHOUT periodic correction due to C->cellDist, it is handled inside
pos2 however is with periodic correction already!!

*/

CREATE_LOGGER(Cg2_Any_Any_L6Geom__Base);

void Cg2_Any_Any_L6Geom__Base::handleSpheresLikeContact(const shared_ptr<Contact>& C, const Vector3r& pos1, const Vector3r& vel1, const Vector3r& angVel1, const Vector3r& pos2, const Vector3r& vel2, const Vector3r& angVel2, const Vector3r& normal, const Vector3r& contPt, Real uN, Real r1, Real r2){
	// create geometry
	if(!C->geom){
		C->geom=make_shared<L6Geom>();
		L6Geom& g(C->geom->cast<L6Geom>());
		g.setInitialLocalCoords(normal);
		g.uN=uN;
		// FIXME FIXME FIXME: this needs to be changed
		// for now, assign lengths of half of the distance between spheres
		// this works good for distance, but breaks radius, which is used for stiffness computation and is hacked around in Cp2_FrictMat_FrictPhys
		// perhaps separating those 2 would help, or computing A/l1, A/l2 for stiffness in Cg2 functors already would be a good idea??
		// XXX: this also fails with uN/2>ri, which is possible for Wall with one sense of contact only
		g.lens=Vector2r(abs(r1)+uN/2,abs(r2)+uN/2); // 
		// this is a hack around that
		if(g.lens[0]<0) g.lens[0]=g.lens[1];
		if(g.lens[1]<0) g.lens[1]=g.lens[0];
		g.contA=M_PI*pow((r1>0&&r2>0)?min(r1,r2):(r1>0?r1:r2),2);
		g.node->pos=contPt;
		g.node->ori=Quaternionr(g.trsf);
		//cerr<<"##"<<C->leakPA()->id<<"+"<<C->leakPB()->id<<": init trsf=\n"<<g.trsf<<endl<<"locX="<<locX<<", locY="<<locY<<", locZ="<<locZ<<"; normal="<<normal<<endl;
		return;
	}
	// update geometry

	/* motion of the conctact consists in rigid motion (normRotVec, normTwistVec) and mutual motion (relShearDu);
	   they are used to update trsf, which is then used to convert relative motion to local coordinates
	*/

	L6Geom& g(C->geom->cast<L6Geom>());
	const Vector3r& currNormal(normal); const Vector3r& prevNormal(g.trsf.row(0));
	const Vector3r& prevContPt(C->geom->node->pos);
	const Real& dt(scene->dt);
	Vector3r shiftVel2(scene->isPeriodic?scene->cell->intrShiftVel(C->cellDist):Vector3r::Zero());


	//cerr<<"prevNormal="<<prevNormal<<", currNomal="<<currNormal<<endl;
	// normal rotation vector, between last steps
	Vector3r normRotVec=prevNormal.cross(currNormal);
	Vector3r midNormal=(approxMask&APPROX_NO_MID_NORMAL) ? prevNormal : .5*(prevNormal+currNormal);
	/* this can happen in exceptional cases (normal gets inverted by motion,
		i.e. when sphere is pushed through a facet;
		this situation is physically meaningless, so there is no physically correct result really;
		it would cause NaN in midNormal.normalize(), which we want need to avoid
	*/
	if(midNormal.norm()==0) midNormal=currNormal; 
	else if(!(approxMask&APPROX_NO_RENORM_MID_NORMAL) && !(approxMask&APPROX_NO_MID_NORMAL)) midNormal.normalize(); // normalize only if used and if requested via approxMask
	Vector3r normTwistVec=midNormal*dt*.5*midNormal.dot(angVel1+angVel2);

	// compute current transformation, by updating previous axes
	// the X axis can be prescribed directly (copy of normal)
	// the mutual motion on the contact does not affect the transformation, that is handled below
	#ifdef L6_TRSF_QUATERNION
		const Matrix3r prevTrsf(g.trsf.toRotationMatrix());
		Quaternionr prevTrsfQ(g.trsf);
	#else
		const Matrix3r prevTrsf(g.trsf); // could be reference perhaps, but we need it to compute midTrsf (if applicable)
	#endif
	/* middle transformation first */
	Matrix3r midTrsf;
	if(approxMask&APPROX_NO_MID_TRSF){ midTrsf.row(0)=prevNormal; midTrsf.row(1)=prevTrsf.row(1); }
	else{ midTrsf.row(0)=midNormal; midTrsf.row(1)=prevTrsf.row(1)-prevTrsf.row(1).cross(normRotVec+normTwistVec)/2.; }
	midTrsf.row(2)=midTrsf.row(0).cross(midTrsf.row(1));

	/* current transformation now */
	Matrix3r currTrsf;
	currTrsf.row(0)=currNormal;
	currTrsf.row(1)=prevTrsf.row(1)-midTrsf.row(1).cross(normRotVec+normTwistVec);
	currTrsf.row(2)=currTrsf.row(0).cross(currTrsf.row(1));

	#ifdef L6_TRSF_QUATERNION
		Quaternionr currTrsfQ(currTrsf);
		if(trsfRenorm>0 && (scene->iter % trsfRenorm)==0) currTrsfQ.normalize();
	#else
		/* orthonormalize in a way to not alter local x-axis */
		if(trsfRenorm>0 && (scene->step % trsfRenorm)==0){
			currTrsf.row(0).normalize();
			currTrsf.row(1)-=currTrsf.row(0)*currTrsf.row(1).dot(currTrsf.row(0)); // take away y projected on x, to stabilize numerically
			currTrsf.row(1).normalize();
			currTrsf.row(2)=currTrsf.row(0).cross(currTrsf.row(1)); // normalized automatically
			#ifdef WOO_DEBUG
				if(abs(currTrsf.determinant()-1)>.05){
					LOG_ERROR("##"<<C->leakPA()->id<<"+"<<C->leakPB()->id<<", |trsf|="<<currTrsf.determinant());
					g.trsf=currTrsf;
					throw runtime_error("Transformation matrix far from orthonormal.");
				}
			#endif
		}
	#endif

#if 0
		// compute midTrsf
	#ifdef L6_TRSF_QUATERNION
		Quaternionr midTrsf=(approxMask&APPROX_NO_MID_TRSF) ? prevTrsfQ : prevTrsfQ.slerp(.5,currTrsfQ);
	#else
		Quaternionr midTrsf=(approxMask&APPROX_NO_MID_TRSF) ? Quaternionr(prevTrsf) : Quaternionr(prevTrsf).slerp(.5,Quaternionr(currTrsf));
	#endif
#endif

	#ifdef WOO_DEBUG
		// cerr<<"Error: prevNormal="<<prevNormal<<", currNomal="<<currNormal<<endl;
		// if(normRotVec.squaredNorm()==0) throw std::runtime_error("Normal moving too fast (changed sense during one step), motion numerically unstable?");
	#endif

	// compute relative velocity
	// noRatch: take radius or current distance as the branch vector; see discussion in ScGeom::precompute (avoidGranularRatcheting)
	Vector3r midContPt, midPos1, midPos2;
	if(approxMask&APPROX_NO_MID_BRANCH){ midContPt=contPt; midPos1=pos1; midPos2=pos2; }
	else{ midContPt=.5*(prevContPt+contPt); midPos1=pos1-(dt/2.)*vel1; /* pos2 is wrapped, use corrected vel2 as well */ midPos2=pos2-(dt/2.)*(vel2+shiftVel2); }
	Vector3r c1x=((noRatch && r1>0) ? ( r1*midNormal).eval() : (midContPt-midPos1).eval()); // used only for sphere-sphere
	Vector3r c2x=( noRatch          ? (-r2*midNormal).eval() : (midContPt-midPos2).eval());
	//Vector3r state2velCorrected=state2.vel+(scene->isPeriodic?scene->cell->intrShiftVel(I->cellDist):Vector3r::Zero()); // velocity of the second particle, corrected with meanfield velocity if necessary
	//cerr<<"correction "<<(scene->isPeriodic?scene->cell->intrShiftVel(I->cellDist):Vector3r::Zero())<<endl;
	Vector3r relVel=(vel2+shiftVel2+angVel2.cross(c2x))-(vel1+angVel1.cross(c1x));

	// relVel-=avgNormal.dot(relShearVel)*avgNormal;
	//Vector3r relShearDu=relShearVel*scene->dt;
	// cerr<<"normRotVec="<<normRotVec<<", avgNormal="<<avgNormal<<", normTwistVec="<<normTwistVec<<"c1x="<<c1x<<", c2x="<<c2x<<", relVel="<<relVel<<endl;


	/* Update of quantities in global coords consists in adding 3 increments we have computed; in global coords (a is any vector)

		1. +relShearVel*scene->dt; // mutual motion of the contact
		2. -a.cross(normRotVec);   // rigid rotation perpendicular to the normal
		3. -a.cross(normTwistVec); // rigid rotation parallel to the normal

	*/


	/* Previous local trsf u'⁻ must be updated to current u'⁰. We have transformation T⁻ and T⁰,
		δ(a) denotes increment of a as defined above.  Two possibilities:

		1. convert to global, update, convert back: T⁰(T⁻*(u'⁻)+δ(T⁻*(u'⁻))). Quite heavy.
		2. update u'⁻ straight, using relShearVel in local coords; since relShearVel is computed 
			at (t-Δt/2), we would have to find intermediary transformation (same axis, half angle;
			the same as slerp at t=.5 between the two).

			This could be perhaps simplified by using T⁰ or T⁻ since they will not differ much,
			but it would have to be verified somehow.
	*/
	// if requested via approxMask, just use prevTrsf
	// cerr<<"prevTrsf=\n"<<prevTrsf<<", currTrsf=\n"<<currTrsf<<", midTrsf=\n"<<Matrix3r(midTrsf)<<endl;
	
	// updates of geom here

	// midTrsf*relShearVel should have the 0-th component (approximately) zero -- to be checked
	//g.u+=midTrsf*relShearDu;

	//cerr<<"midTrsf=\n"<<midTrsf<<",relShearDu="<<relShearDu<<", transformed "<<midTrsf*relShearDu<<endl;
	#ifdef L6_TRSF_QUATERNION
		g.trsf=currTrsfQ;
	#else
		g.trsf=currTrsf;
	#endif

	// update data here
	g.vel=midTrsf*relVel;
	g.angVel=midTrsf*(angVel2-angVel1);
	g.uN=uN; // this does not have to be computed incrementally
	// Node
	g.node->pos=contPt;
	g.node->ori=Quaternionr(g.trsf);
};

#if 0
#if WOO_OPENGL
bool Gl1_L6Geom::axesLabels;
Real Gl1_L6Geom::axesScale;
int Gl1_L6Geom::axesWd;
Vector2i Gl1_L6Geom::axesWd_range;
//int Gl1_L6Geom::uPhiWd;
//Vector2i Gl1_L6Geom::uPhiWd_range;
//Real Gl1_L6Geom::uScale;
//Real Gl1_L6Geom::phiScale;
//Vector2r Gl1_L6Geom::phiScale_range;


//void Gl1_L3Geom::go(const shared_ptr<IGeom>& ig, const shared_ptr<Interaction>&, const shared_ptr<Body>&, const shared_ptr<Body>&, bool){ draw(ig); }
// void Gl1_L6Geom::go(const shared_ptr<CGeom>& ig, const shared_ptr<Contact>& C, bool){ draw(ig,true,phiScale); }

void Gl1_L6Geom::go(const shared_ptr<CGeom>& ig, const shared_ptr<Contact>& C, bool){
	const L6Geom& g(ig->cast<L6Geom>());
	const Node& cn=*(C->geom->node);
	glTranslatev(cn.pos);
	#ifdef L6_TRSF_QUATERNION
		#if EIGEN_MAJOR_VERSION<30
 			glMultMatrixd(Eigen::Transform3d(Matrix3r(cn.ori).transpose()).data());
		#else
 			glMultMatrixd(Eigen::Affine3d(Matrix3r(cn.ori).transpose()).data());
		#endif
	#else
		#if EIGEN_MAJOR_VERSION<30
 			glMultMatrixd(Eigen::Transform3d(cn.ori.transpose()).data());
		#else
 			glMultMatrixd(Eigen::Affine3d(cn.ori.transpose()).data());
		#endif
	#endif
	Real rMin=g.getMinRefLen();
	if(axesWd>0){
		glLineWidth(axesWd);
		for(int i=0; i<3; i++){
			Vector3r pt=Vector3r::Zero(); pt[i]=.5*rMin*axesScale; Vector3r color=.3*Vector3r::Ones(); color[i]=1;
			GLUtils::GLDrawLine(Vector3r::Zero(),pt,color);
			if(axesLabels) GLUtils::GLDrawText(string(i==0?"x":(i==1?"y":"z")),pt,color);
		}
	}
	#if 0
	if(uPhiWd>0){
		glLineWidth(uPhiWd);
		if(uScale!=0) GLUtils::GLDrawLine(Vector3r::Zero(),uScale*g.relU(),Vector3r(0,1,.5));
		if(isL6Geom && phiScale>0) GLUtils::GLDrawLine(Vector3r::Zero(),ig->cast<L6Geom>().relPhi()/M_PI*rMin*phiScale,Vector3r(.8,0,1));
	}
	#endif
	glLineWidth(1.);
};

#endif

#endif
