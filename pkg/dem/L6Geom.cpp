#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/core/Field.hpp>
#include<woo/lib/base/CompUtils.hpp>

#include<sstream>

#ifdef WOO_OPENGL
	#include<woo/lib/opengl/OpenGLWrapper.hpp>
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<GL/glu.h>
#endif

WOO_PLUGIN(dem,(L6Geom)(Cg2_Any_Any_L6Geom__Base));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_L6Geom__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Any_Any_L6Geom__Base__CLASS_BASE_DOC_ATTRS);

#if 0
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_L6Geom));
#endif
#endif


// QQQ: ALL occurences of .row(..) were replaced by .col(...) !!!

void L6Geom::setInitialLocalCoords(const Vector3r& locX){
	// initial local y-axis orientation, in the xz or xy plane, depending on which component is larger to avoid singularities
	Vector3r locY=locX.cross(abs(locX[1])<abs(locX[2])?Vector3r::UnitY():Vector3r::UnitZ()); locY-=locX*locY.dot(locX); locY.normalize();
	Vector3r locZ=locX.cross(locY);
	// set our data
	#ifdef L6_TRSF_QUATERNION
		Matrix3r T; T.col(0)=locX; T.col(1)=locY; T.col(2)=locZ;
		trsf=Quaternionr(T); // from transformation matrix
	#else
		trsf.col(0)=locX; trsf.col(1)=locY; trsf.col(2)=locZ;
	#endif
}


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
	const Vector3r& currNormal(normal); const Vector3r& prevNormal(g.trsf.col(0));
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
	if(approxMask&APPROX_NO_MID_TRSF){ midTrsf.col(0)=prevNormal; midTrsf.col(1)=prevTrsf.col(1); }
	else{ midTrsf.col(0)=midNormal; midTrsf.col(1)=prevTrsf.col(1)-prevTrsf.col(1).cross(normRotVec+normTwistVec)/2.; }
	midTrsf.col(2)=midTrsf.col(0).cross(midTrsf.col(1));

	/* current transformation now */
	Matrix3r currTrsf;
	currTrsf.col(0)=currNormal;
	currTrsf.col(1)=prevTrsf.col(1)-midTrsf.col(1).cross(normRotVec+normTwistVec);
	currTrsf.col(2)=currTrsf.col(0).cross(currTrsf.col(1));

	#ifdef L6_TRSF_QUATERNION
		Quaternionr currTrsfQ(currTrsf);
		if(trsfRenorm>0 && (scene->iter % trsfRenorm)==0) currTrsfQ.normalize();
	#else
		/* orthonormalize in a way to not alter local x-axis */
		if(trsfRenorm>0 && (scene->step % trsfRenorm)==0){
			currTrsf.col(0).normalize();
			currTrsf.col(1)-=currTrsf.col(0)*currTrsf.col(1).dot(currTrsf.col(0)); // take away y projected on x, to stabilize numerically
			currTrsf.col(1).normalize();
			currTrsf.col(2)=currTrsf.col(0).cross(currTrsf.col(1)); // normalized automatically
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
	Vector3r c2x=((noRatch && r2>0) ? (-r2*midNormal).eval() : (midContPt-midPos2).eval());
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
	#ifdef L6_TRSF_QUATERNION
		const auto& midTrsfInv=midTrsf.conjugate();
	#else
		const auto& midTrsfInv=midTrsf.transpose();
	#endif
	g.vel=midTrsfInv*relVel;
	g.angVel=midTrsfInv*(angVel2-angVel1);
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
