#include<woo/pkg/dem/Truss.hpp>
WOO_PLUGIN(dem,(Truss)(Bo1_Truss_Aabb)(In2_Truss_ElastMat)(Cg2_Truss_Sphere_L6Geom));
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Truss_Sphere_L6Geom__CLASS_BASE_DOC);

void Bo1_Truss_Aabb::go(const shared_ptr<Shape>& sh){
	assert(sh->numNodesOk());
	Truss& t=sh->cast<Truss>();
	if(!t.bound){ t.bound=shared_ptr<Bound>(new Aabb); }
	Aabb& aabb=sh->bound->cast<Aabb>();
	// TODO: fix caps and so on; for now, approximate, a bit larger than necessary.
	// TODO: test periodic shear
	Real r=t.radius;
	if(scene->isPeriodic && scene->cell->hasShear()) r*=1./scene->cell->getCos().minCoeff();
	Vector3r pt[4];
	for(int i=0; i<4; i++) pt[i]=Vector3r(t.nodes[(i%2)?1:0]->pos+(i/2?-1:1)*r*Vector3r::Ones());
	if(scene->isPeriodic) for(int i=0; i<4; i++) pt[i]=scene->cell->unshearPt(pt[i]);
	for(int i=0; i<3; i++){
		aabb.min[i]=min(pt[0][i],min(pt[1][i],min(pt[2][i],pt[3][i])));
		aabb.max[i]=max(pt[0][i],max(pt[1][i],max(pt[2][i],pt[3][i])));
	}
};

void In2_Truss_ElastMat::go(const shared_ptr<Shape>& shape, const shared_ptr<Material>& mat, const shared_ptr<Particle>& particle){
	Truss& t=shape->cast<Truss>();
	assert(t.numNodesOk());
	Vector3r Fa(Vector3r::Zero()), Fb(Vector3r::Zero());
	const Vector3r AB(t.nodes[1]->pos-t.nodes[0]->pos);
	const Real len=AB.norm();
	const Vector3r axUnit=AB/len;
	if(isnan(t.l0)){
		if(setL0) t.l0=len;
		else woo::ValueError(("#"+lexical_cast<string>(particle->id)+": Truss.l0==NaN (set In2_Truss_ElastMat.setL0=True to initialize this value automatically."));
	}
	FOREACH(const Particle::MapParticleContact::value_type& I,particle->contacts){
		const shared_ptr<Contact>& C(I.second); if(!C->isReal()) continue;
		// force and torque at the contact point
		Vector3r Fc(C->geom->node->ori.conjugate()*C->phys->force *C->forceSign(particle));
		Vector3r Tc(C->geom->node->ori.conjugate()*C->phys->torque*C->forceSign(particle));
		// compute reactions (simply supported beam) on both ends
		Vector3r AC(C->geom->node->pos-t.nodes[0]->pos);
		// torque on A from contact
		//Vector3r Ta=(Tc+Fc.cross(AC));

		// force FC and torque TC (at contact point C) are to be replaced by
		// Fac abd Fbc; from torque around A, we get the perpendicular contribution as
		// Tc-AC×FcPerp+AB×Fbc=0 and Fbc.AB=0, from which
		// Fbc=AB×(-Tc+AC×FcPerp)/|AB|²
		// (http://boards.straightdope.com/sdmb/showpost.php?p=5107984&postcount=3)
		Vector3r FcAxial=axUnit*Fc.dot(axUnit);
		Vector3r FcPerp=Fc-FcAxial;
		Vector3r Fbc=AB.cross(-Tc+AC.cross(FcPerp))/AB.squaredNorm();
		// axial force is distributed according to relative position on the truss
		// max(0,min(1,AC.n/|AB|)) (min and max in case the contact points is on the cap)
		// Fbc+=FcAxial*cRelPos
		// from Fac+Fbc=Fc → Fac=Fc-Fbc
		Real cRelPos=max(0.,min(1.,AC.dot(axUnit)/len));
		Fbc+=FcAxial*cRelPos;
		Fb+=Fbc;
		Fa+=Fc-Fbc;
	}
	// elastic response of the truss
	// natural strain
	Real natStrain=log(1+(len-t.l0)/t.l0);
	// Fn=E*ε*A
	t.axialStress=t.preStress+natStrain*mat->cast<ElastMat>().young;
	Real Fn=t.axialStress*M_PI*pow(t.radius,2);
	Fa+=Fn*AB/len;
	Fb-=Fn*AB/len;
	// cerr<<"AB="<<AB<<", len="<<len<<", natStrain="<<natStrain<<", l0="<<t.l0<<", E="<<mat->cast<ElastMat>().young<<", Fn="<<Fn<<endl;

	// apply nodal forces
	t.nodes[0]->getData<DemData>().addForceTorque(Fa);
	t.nodes[1]->getData<DemData>().addForceTorque(Fb);
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





#ifdef WOO_OPENGL
WOO_PLUGIN(gl,(Gl1_Truss));
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/lib/base/CompUtils.hpp>
bool Gl1_Truss::wire;
int Gl1_Truss::slices;
int Gl1_Truss::stacks;
Vector2r Gl1_Truss::stressRange;
bool Gl1_Truss::colorStress;
void Gl1_Truss::go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2, const GLViewInfo&){
	const Truss& t(shape->cast<Truss>());
	assert(t.numNodesOk());
	Vector3r color;
	if(!colorStress) color=Vector3r(NaN,NaN,NaN); // keep current
	else {
		stressRange[0]=min(stressRange[0],t.axialStress);
		stressRange[1]=max(stressRange[1],t.axialStress);
		color=CompUtils::scalarOnColorScale(t.axialStress,stressRange[0],stressRange[1]);
	}
	glShadeModel(GL_SMOOTH);
	GLUtils::Cylinder(t.nodes[0]->pos+shift,t.nodes[1]->pos+shift,t.radius,color,t.getWire()||wire2,/*caps*/false,t.radius,slices,stacks);
	for(int end=0; end<2; end++){
		if(!(t.caps&(end==0?Truss::CAP_A:Truss::CAP_B))) continue;
		glPushMatrix();
			glTranslatev(t.nodes[end]->pos);
			// do not use stack value here, that is for cylinder length subdivision
			// TODO: only render half of the sphere
			if(wire||wire2) glutWireSphere(t.radius,slices,slices/3);
			else glutSolidSphere(t.radius,slices,slices/3);
		glPopMatrix();
	}
}
#endif
