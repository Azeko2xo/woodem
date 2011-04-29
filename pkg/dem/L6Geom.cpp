
#include<yade/pkg/dem/L6Geom.hpp>
#include<yade/pkg/dem/Sphere.hpp>
//#include<yade/pkg/common/Wall.hpp>
//#include<yade/pkg/common/Facet.hpp>
#include<yade/core/Field.hpp>

#include<sstream>

#ifdef YADE_OPENGL
	#include<yade/lib/opengl/OpenGLWrapper.hpp>
	#include<yade/lib/opengl/GLUtils.hpp>
	#include<GL/glu.h>
#endif

YADE_PLUGIN(dem,(L6Geom)(Cg2_Sphere_Sphere_L6Geom));
// (Ig2_Wall_Sphere_L3Geom)(Ig2_Facet_Sphere_L3Geom)(Ig2_Sphere_Sphere_L6Geom)(Law2_L3Geom_FrictPhys_ElPerfPl)(Law2_L6Geom_FrictPhys_Linear);
//#ifdef YADE_OPENGL
//	YADE_PLUGIN(gl,/*(Gl1_L3Geom)*/(Gl1_L6Geom));
//#endif

CREATE_LOGGER(Cg2_Sphere_Sphere_L6Geom);

bool Cg2_Sphere_Sphere_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Real& r1=s1->cast<Sphere>().radius; const Real& r2=s2->cast<Sphere>().radius;
	assert(s1->numNodesOk()); assert(s2->numNodesOk());
	assert(s1->nodes[0]->dyn); assert(s2->nodes[0]->dyn);

	Vector3r relPos=s2->nodes[0]->pos+shift2-s1->nodes[0]->pos;
	Real unDistSq=relPos.squaredNorm()-pow(abs(distFactor)*(r1+r2),2);
	if (unDistSq>0 && !C->isReal() && !force) return false;

	// contact exists, go ahead

	Real dist=relPos.norm();
	Real uN=dist-(r1+r2);
	Vector3r normal=relPos/dist;
	Vector3r contPt=s1->nodes[0]->pos+(r1+0.5*uN)*normal;

	handleSpheresLikeContact(C,s1->nodes[0]->pos,s1->nodes[0]->dyn->vel,s1->nodes[0]->dyn->angVel,s2->nodes[0]->pos+shift2,s2->nodes[0]->dyn->vel,s2->nodes[0]->dyn->angVel,normal,contPt,uN,r1,r2);

	return true;

};

/*
Generic function to compute L6Geom, used for {sphere,facet,wall}+sphere contacts
*/
void Cg2_Sphere_Sphere_L6Geom::handleSpheresLikeContact(const shared_ptr<Contact>& C, const Vector3r& pos1, const Vector3r& vel1, const Vector3r& angVel1, const Vector3r& pos2, const Vector3r& vel2, const Vector3r& angVel2, const Vector3r& normal, const Vector3r& contPt, Real uN, Real r1, Real r2){
	// create geometry
	if(!C->geom){
		C->geom=shared_ptr<L6Geom>(new L6Geom);
		L6Geom& g(C->geom->cast<L6Geom>());
		const Vector3r& locX(normal);
		// initial local y-axis orientation, in the xz or xy plane, depending on which component is larger to avoid singularities
		Vector3r locY=normal.cross(abs(normal[1])<abs(normal[2])?Vector3r::UnitY():Vector3r::UnitZ()); locY-=locX*locY.dot(locX); locY.normalize();
		Vector3r locZ=normal.cross(locY);
		#ifdef L6_TRSF_QUATERNION
			Matrix3r trsf; trsf.row(0)=locX; trsf.row(1)=locY; trsf.row(2)=locZ;
			g.trsf=Quaternionr(trsf); // from transformation matrix
		#else
			g.trsf.row(0)=locX; g.trsf.row(1)=locY; g.trsf.row(2)=locZ;
		#endif
		g.uN=uN;
		g.lengthHint=Vector2r(r1,r2);
		g.node->pos=contPt;
		g.node->ori=Quaternionr(g.trsf);
		// cerr<<"Init trsf=\n"<<g.trsf<<endl<<"locX="<<locX<<", locY="<<locY<<", locZ="<<locZ<<endl;
		return;
	}
	// update geometry

	/* motion of the conctact consists in rigid motion (normRotVec, normTwistVec) and mutual motion (relShearDu);
	   they are used to update trsf, which is then used to convert relative motion to local coordinates
	*/

	L6Geom& g(C->geom->cast<L6Geom>());
	const Vector3r& currNormal(normal); const Vector3r& prevNormal(g.trsf.row(0));
	//cerr<<"prevNormal="<<prevNormal<<", currNomal="<<currNormal<<endl;
	// normal rotation vector, between last steps
	Vector3r normRotVec=prevNormal.cross(currNormal);
	#ifdef YADE_DEBUG
		// cerr<<"Error: prevNormal="<<prevNormal<<", currNomal="<<currNormal<<endl;
		// if(normRotVec.squaredNorm()==0) throw std::runtime_error("Normal moving too fast (changed sense during one step), motion numerically unstable?");
	#endif
	// contrary to what ScGeom::precompute does now (r2486), we take average normal, i.e. .5*(prevNormal+currNormal),
	// so that all terms in the equation are in the previous mid-step
	// the re-normalization might not be necessary for very small increments, but better do it
	Vector3r avgNormal=(approxMask&APPROX_NO_MID_NORMAL) ? prevNormal : .5*(prevNormal+currNormal);
	if(!(approxMask&APPROX_NO_RENORM_MID_NORMAL) && !(approxMask&APPROX_NO_MID_NORMAL)) avgNormal.normalize(); // normalize only if used and if requested via approxMask
	// twist vector of the normal from the last step
	Vector3r normTwistVec=avgNormal*scene->dt*.5*avgNormal.dot(angVel1+angVel2);
	// compute relative velocity
	// noRatch: take radius or current distance as the branch vector; see discussion in ScGeom::precompute (avoidGranularRatcheting)
	Vector3r c1x=((noRatch && !r1>0) ? ( r1*normal).eval() : (contPt-pos1).eval()); // used only for sphere-sphere
	Vector3r c2x=( noRatch           ? (-r2*normal).eval() : (contPt-pos2).eval());
	//Vector3r state2velCorrected=state2.vel+(scene->isPeriodic?scene->cell->intrShiftVel(I->cellDist):Vector3r::Zero()); // velocity of the second particle, corrected with meanfield velocity if necessary
	//cerr<<"correction "<<(scene->isPeriodic?scene->cell->intrShiftVel(I->cellDist):Vector3r::Zero())<<endl;
	Vector3r relVel=(vel2+angVel2.cross(c2x))-(vel1+angVel1.cross(c1x));
	// account for relative velocity of particles in different cell periods
	if(scene->isPeriodic) relVel+=scene->cell->intrShiftVel(C->cellDist);

	// relVel-=avgNormal.dot(relShearVel)*avgNormal;
	//Vector3r relShearDu=relShearVel*scene->dt;
	// cerr<<"normRotVec="<<normRotVec<<", avgNormal="<<avgNormal<<", normTwistVec="<<normTwistVec<<"c1x="<<c1x<<", c2x="<<c2x<<", relVel="<<relVel<<endl;




	/* Update of quantities in global coords consists in adding 3 increments we have computed; in global coords (a is any vector)

		1. +relShearVel*scene->dt; // mutual motion of the contact
		2. -a.cross(normRotVec);   // rigid rotation perpendicular to the normal
		3. -a.cross(normTwistVec); // rigid rotation parallel to the normal

	*/

	// compute current transformation, by updating previous axes
	// the X axis can be prescribed directly (copy of normal)
	// the mutual motion on the contact does not change transformation
	#ifdef L6_TRSF_QUATERNION
		const Matrix3r prevTrsf(g.trsf.toRotationMatrix());
		Quaternionr prevTrsfQ(g.trsf);
	#else
		const Matrix3r prevTrsf(g.trsf); // could be reference perhaps, but we need it to compute midTrsf (if applicable)
	#endif
	Matrix3r currTrsf; currTrsf.row(0)=currNormal;
	for(int i=1; i<3; i++){
		currTrsf.row(i)=prevTrsf.row(i)-prevTrsf.row(i).cross(normRotVec)-prevTrsf.row(i).cross(normTwistVec);
	}
	#ifdef L6_TRSF_QUATERNION
		Quaternionr currTrsfQ(currTrsf);
		if((scene->iter % trsfRenorm)==0 && trsfRenorm>0) currTrsfQ.normalize();
	#else
		if((scene->step % trsfRenorm)==0 && trsfRenorm>0){
			#if 1
				currTrsf.row(0).normalize();
				currTrsf.row(1)-=currTrsf.row(0)*currTrsf.row(1).dot(currTrsf.row(0)); // take away y projected on x, to stabilize numerically
				currTrsf.row(1).normalize();
				currTrsf.row(2)=currTrsf.row(0).cross(currTrsf.row(1));
				currTrsf.row(2).normalize();
			#else
				currTrsf=Matrix3r(Quaternionr(currTrsf).normalized());
			#endif
			#ifdef YADE_DEBUG
				if(abs(currTrsf.determinant()-1)>.05){
					LOG_ERROR("##"<<C->pA->id<<"+"<<C->pB->id<<", |trsf|="<<currTrsf.determinant());
					g.trsf=currTrsf;
					throw runtime_error("Transformation matrix far from orthonormal.");
				}
			#endif
		}
	#endif

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
	#ifdef L6_TRSF_QUATERNION
		Quaternionr midTrsf=(approxMask&APPROX_NO_MID_TRSF) ? prevTrsfQ : prevTrsfQ.slerp(.5,currTrsfQ);
	#else
		Quaternionr midTrsf=(approxMask&APPROX_NO_MID_TRSF) ? Quaternionr(prevTrsf) : Quaternionr(prevTrsf).slerp(.5,Quaternionr(currTrsf));
	#endif
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
bool Ig2_Wall_Sphere_L3Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const State& state1, const State& state2, const Vector3r& shift2, const bool& force, const shared_ptr<Interaction>& I){
	if(scene->isPeriodic) throw std::logic_error("Ig2_Wall_Sphere_L3Geom does not handle periodic boundary conditions.");
	const Real& radius=s2->cast<Sphere>().radius; const int& ax(s1->cast<Wall>().axis); const int& sense(s1->cast<Wall>().sense);
	Real dist=state2.pos[ax]+shift2[ax]-state1.pos[ax]; // signed "distance" between centers
	if(!I->isReal() && abs(dist)>radius && !force) { return false; }// wall and sphere too far from each other
	// contact point is sphere center projected onto the wall
	Vector3r contPt=state2.pos+shift2; contPt[ax]=state1.pos[ax];
	Vector3r normal=Vector3r::Zero();
	// wall interacting from both sides: normal depends on sphere's position
	assert(sense==-1 || sense==0 || sense==1);
	if(sense==0) normal[ax]=dist>0?1.:-1.;
	else normal[ax]=(sense==1?1.:-1);
	Real uN=normal[ax]*dist-radius; // takes in account sense, radius and distance

	// check that the normal did not change orientation (would be abrup here)
	if(I->geom && I->geom->cast<L3Geom>().normal!=normal){
		ostringstream oss; oss<<"Ig2_Wall_Sphere_L3Geom: normal changed from ("<<I->geom->cast<L3Geom>().normal<<" to "<<normal<<" in Wall+Sphere ##"<<I->getId1()<<"+"<<I->getId2()<<" (with Wall.sense=0, a particle might cross the Wall plane, if Δt is too high)"; throw std::logic_error(oss.str().c_str());
	}
	handleSpheresLikeContact(I,state1,state2,shift2,/*is6Dof*/false,normal,contPt,uN,/*r1*/0,/*r2*/radius);
	return true;
};

bool Ig2_Facet_Sphere_L3Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const State& state1, const State& state2, const Vector3r& shift2, const bool& force, const shared_ptr<Interaction>& I){
	const Facet& facet(s1->cast<Facet>());
	Real radius=s2->cast<Sphere>().radius;
	// begin facet-local coordinates
		Vector3r cogLine=state1.ori.conjugate()*(state2.pos+shift2-state1.pos); // connect centers of gravity
		Vector3r normal=facet.normal; // trial contact normal
		Real planeDist=normal.dot(cogLine);
		if(abs(planeDist)>radius && !I->isReal() && !force) return false; // sphere too far

		// HACK: refuse to collide sphere and facet with common node; for that, body is needed :-|
		// this could be moved to the collider (the mayCollide function)
		vector<shared_ptr<Node> > fn=Body::byId(I->getId1(),scene)->nodes;
		vector<shared_ptr<Node> > sn=Body::byId(I->getId2(),scene)->nodes;
		assert(fn.size()==3 && sn.size()==1);
		if(fn[0]==sn[0] || fn[1]==sn[0] || fn[2]==sn[0]) return false;

		if(planeDist<0){normal*=-1; planeDist*=-1; }
		Vector3r planarPt=cogLine-planeDist*normal; // project sphere center to the facet plane
		Vector3r contactPt; // facet's point closes to the sphere
		Real normDotPt[3];  // edge outer normals dot products
		for(int i=0; i<3; i++) normDotPt[i]=facet.ne[i].dot(planarPt-facet.vertices[i]);
		short w=(normDotPt[0]>0?1:0)+(normDotPt[1]>0?2:0)+(normDotPt[2]>0?4:0); // bitmask whether the closest point is outside (1,2,4 for respective edges)
		switch(w){
			case 0: contactPt=planarPt; break; // ---: inside triangle
			case 1: contactPt=getClosestSegmentPt(planarPt,facet.vertices[0],facet.vertices[1]); break; // +-- (n1)
			case 2: contactPt=getClosestSegmentPt(planarPt,facet.vertices[1],facet.vertices[2]); break; // -+- (n2)
			case 4: contactPt=getClosestSegmentPt(planarPt,facet.vertices[2],facet.vertices[0]); break; // --+ (n3)
			case 3: contactPt=facet.vertices[1]; break; // ++- (v1)
			case 5: contactPt=facet.vertices[0]; break; // +-+ (v0)
			case 6: contactPt=facet.vertices[2]; break; // -++ (v2)
			case 7: throw logic_error("Ig2_Facet_Sphere_L3Geom: Impossible sphere-facet intersection (all points are outside the edges). (please report bug)"); // +++ (impossible)
			default: throw logic_error("Ig2_Facet_Sphere_L3Geom: Nonsense intersection value. (please report bug)");
		}
		normal=cogLine-contactPt; // normal is now the contact normal, still in local coords
		if(!I->isReal() && normal.squaredNorm()>radius*radius && !force) { return false; } // fast test before sqrt
		Real dist=normal.norm(); normal/=dist; // normal is unit vector now
	// end facet-local coordinates
	normal=state1.ori*normal; // normal is in global coords now
	handleSpheresLikeContact(I,state1,state2,shift2,/*is6Dof*/false,normal,/*contact pt*/state2.pos+shift2-normal*dist,dist-radius,0,radius);
	return true;
}

void Law2_L3Geom_FrictPhys_ElPerfPl::go(shared_ptr<IGeom>& ig, shared_ptr<IPhys>& ip, Interaction* I){
	L3Geom* geom=static_cast<L3Geom*>(ig.get()); FrictPhys* phys=static_cast<FrictPhys*>(ip.get());

	// compute force
	Vector3r& localF(geom->F);
	localF=geom->relU().cwise()*Vector3r(phys->kn,phys->ks,phys->ks);
	// break if necessary
	if(localF[0]>0 && !noBreak){ scene->interactions->requestErase(I->getId1(),I->getId2()); return; }

	if(!noSlip){
		// plastic slip, if necessary; non-zero elastic limit only for compression
		Real maxFs=-min(0.,localF[0]*phys->tanPhi); Eigen::Map<Vector2r> Fs(&localF[1]);
		//cerr<<"u="<<geom->relU()<<", maxFs="<<maxFs<<", Fn="<<localF[0]<<", |Fs|="<<Fs.norm()<<", Fs="<<Fs<<endl;
		if(Fs.squaredNorm()>maxFs*maxFs){
			Real ratio=sqrt(maxFs*maxFs/Fs.squaredNorm());
			Vector3r u0slip=(1-ratio)*Vector3r(/*no slip in the normal sense*/0,geom->relU()[1],geom->relU()[2]);
			geom->u0+=u0slip; // increment plastic displacement
			Fs*=ratio; // decrement shear force value;
			if(unlikely(scene->trackEnergy)){ Real dissip=Fs.norm()*u0slip.norm(); if(dissip>0) scene->energy->add(dissip,"plastDissip",plastDissipIx,/*reset*/false); }
		}
	}
	if(unlikely(scene->trackEnergy)){ scene->energy->add(0.5*(pow(geom->relU()[0],2)*phys->kn+(pow(geom->relU()[1],2)+pow(geom->relU()[2],2))*phys->ks),"elastPotential",elastPotentialIx,/*reset at every timestep*/true); }
	// apply force: this converts the force to global space, updates NormShearPhys::{normal,shear}Force, applies to particles
	geom->applyLocalForce(localF,I,scene,phys);
}


void Law2_L6Geom_FrictPhys_Linear::go(shared_ptr<IGeom>& ig, shared_ptr<IPhys>& ip, Interaction* I){
	L6Geom& geom=ig->cast<L6Geom>(); FrictPhys& phys=ip->cast<FrictPhys>();

	// simple linear relationships
	Vector3r localF=geom.relU().cwise()*Vector3r(phys.kn,phys.ks,phys.ks);
	Vector3r localT=charLen*(geom.relPhi().cwise()*Vector3r(phys.kn,phys.ks,phys.ks));

	geom.applyLocalForceTorque(localF,localT,I,scene,static_cast<NormShearPhys*>(ip.get()));
}
#endif 
#if 0 // YADE_OPENGL
bool Gl1_L3Geom::axesLabels;
int Gl1_L3Geom::axesWd;
Vector2i Gl1_L3Geom::axesWd_range;
Real Gl1_L3Geom::axesScale;
int Gl1_L3Geom::uPhiWd;
Vector2i Gl1_L3Geom::uPhiWd_range;
Real Gl1_L3Geom::uScale;
Real Gl1_L6Geom::phiScale;
Vector2r Gl1_L6Geom::phiScale_range;


void Gl1_L3Geom::go(const shared_ptr<IGeom>& ig, const shared_ptr<Interaction>&, const shared_ptr<Body>&, const shared_ptr<Body>&, bool){ draw(ig); }
void Gl1_L6Geom::go(const shared_ptr<IGeom>& ig, const shared_ptr<Interaction>&, const shared_ptr<Body>&, const shared_ptr<Body>&, bool){ draw(ig,true,phiScale); }

void Gl1_L3Geom::draw(const shared_ptr<IGeom>& ig, bool isL6Geom, const Real& phiScale){
	const L3Geom& g(ig->cast<L3Geom>());
	glTranslatev(g.contactPoint);
	#ifdef L6_TRSF_QUATERNION
		#if EIGEN_MAJOR_VERSION<30
 			glMultMatrixd(Eigen::Transform3d(Matrix3r(g.trsf).transpose()).data());
		#else
 			glMultMatrixd(Eigen::Affine3d(Matrix3r(g.trsf).transpose()).data());
		#endif
	#else
		#if EIGEN_MAJOR_VERSION<30
 			glMultMatrixd(Eigen::Transform3d(g.trsf.transpose()).data());
		#else
 			glMultMatrixd(Eigen::Affine3d(g.trsf.transpose()).data());
		#endif
	#endif
	Real rMin=g.refR1<=0?g.refR2:(g.refR2<=0?g.refR1:min(g.refR1,g.refR2));
	if(axesWd>0){
		glLineWidth(axesWd);
		for(int i=0; i<3; i++){
			Vector3r pt=Vector3r::Zero(); pt[i]=.5*rMin*axesScale; Vector3r color=.3*Vector3r::Ones(); color[i]=1;
			GLUtils::GLDrawLine(Vector3r::Zero(),pt,color);
			if(axesLabels) GLUtils::GLDrawText(string(i==0?"x":(i==1?"y":"z")),pt,color);
		}
	}
	if(uPhiWd>0){
		glLineWidth(uPhiWd);
		if(uScale!=0) GLUtils::GLDrawLine(Vector3r::Zero(),uScale*g.relU(),Vector3r(0,1,.5));
		if(isL6Geom && phiScale>0) GLUtils::GLDrawLine(Vector3r::Zero(),ig->cast<L6Geom>().relPhi()/Mathr::PI*rMin*phiScale,Vector3r(.8,0,1));
	}
	glLineWidth(1.);
};

#endif
