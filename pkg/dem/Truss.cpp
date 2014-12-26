#include<woo/pkg/dem/Truss.hpp>

WOO_PLUGIN(dem,(Rod)(Truss)(Bo1_Rod_Aabb)(In2_Truss_ElastMat)(Cg2_Rod_Sphere_L6Geom));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_Rod__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_Truss__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Rod_Sphere_L6Geom__CLASS_BASE_DOC);

void Rod::lumpMassInertia(const shared_ptr<Node>&, Real density, Real& mass, Matrix3r& I, bool& rotateOk){
	throw std::runtime_error("Rod::lumpMassInertia: not yet implemented.");
	#if 0
		checkNodesHaveDemData();
		for(int i:{0,1}){
			auto& dyn(nodes[i]->getData<DemData>());
			dyn.mass=0;
			dyn.inertia=Vector3r::Zero();
		}
	#endif
}


void Bo1_Rod_Aabb::go(const shared_ptr<Shape>& sh){
	assert(sh->numNodesOk());
	Rod& t=sh->cast<Rod>();
	if(!t.bound){ t.bound=make_shared<Aabb>(); /* ignore node rotation*/ t.bound->cast<Aabb>().maxRot=-1; }
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
	if(true){
		for(const auto& pC: particle->contacts){
			const shared_ptr<Contact>& C(pC.second); if(!C->isReal()) continue;
			// force and torque at the contact point
			Vector3r Fc(C->geom->node->ori*C->phys->force *C->forceSign(particle));
			Vector3r Tc(C->geom->node->ori*C->phys->torque*C->forceSign(particle));
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

void In2_Truss_ElastMat::addIntraStiffnesses(const shared_ptr<Particle>& p, const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const {
	// see http://people.duke.edu/~hpgavin/cee421/truss-3d.pdf
	const auto& t(p->shape->cast<Truss>());
	Vector3r dx=t.nodes[1]->pos-t.nodes[0]->pos;
	Real L=dx.norm();
	Real A=M_PI*pow(t.radius,2);
	Vector3r cc=dx/L; // direction cosines
	if(!p->material || !p->material->isA<ElastMat>()) return;
	const Real& E=p->material->cast<ElastMat>().young;
	// same for both nodes
	ktrans+=(E*A/L)*cc.array().pow(2).matrix();
	// nothing to do for krot
}


bool Cg2_Rod_Sphere_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Rod& t=s1->cast<Rod>(); const Sphere& s=s2->cast<Sphere>();
	assert(s1->numNodesOk()); assert(s2->numNodesOk());
	assert(s1->nodes[0]->hasData<DemData>()); assert(s2->nodes[0]->hasData<DemData>());
	const DemData& dynRod(t.nodes[0]->getData<DemData>());
	const DemData& dynSphere(s.nodes[0]->getData<DemData>());
	Real normAxisPos;

	Vector3r cylAxisPt=CompUtils::closestSegmentPt(s.nodes[0]->pos+shift2,t.nodes[0]->pos,t.nodes[1]->pos,&normAxisPos);
	Vector3r relPos=s.nodes[0]->pos-cylAxisPt;
	Real unDistSq=relPos.squaredNorm()-pow(s.radius+t.radius,2); // no distFactor here

	// TODO: find closest point of sharp-cut ends, remove this error
	// if(!(t.caps|Rod::CAP_A) || !(t.caps|Rod::CAP_B)) throw std::runtime_error("Cg2_Sphere_Rod_L6Geom: only handles capped trusses for now.");

	if (unDistSq>0 && !C->isReal() && !force) return false;

	// contact exists, go ahead
	Real dist=relPos.norm();
	Real uN=dist-(s.radius+t.radius);
	Vector3r normal=relPos/dist;
	Vector3r contPt=s.nodes[0]->pos-(s.radius+0.5*uN)*normal;

	#if 0
		Vector3r cylPtVel=(1-normAxisPos)*dynRod.vel+normAxisPos*dynRod.vel; // average velocity
		// angular velocity of the contact point, exclusively due to linear velocity of endpoints
		Vector3r cylPtAngVel=dynRod.vel.cross(t.nodes[0]->pos-contPt)+dynRod.vel.cross(t.nodes[1]->pos-contPt);
	#endif

	handleSpheresLikeContact(C,cylAxisPt,dynRod.vel,dynRod.angVel,s.nodes[0]->pos+shift2,dynSphere.vel,dynSphere.angVel,normal,contPt,uN,t.radius,s.radius);

	return true;
};





#ifdef WOO_OPENGL
WOO_PLUGIN(gl,(Gl1_Rod));
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/lib/base/CompUtils.hpp>
bool Gl1_Rod::wire;
int Gl1_Rod::slices;
int Gl1_Rod::stacks;
Vector2r Gl1_Rod::stressRange;
bool Gl1_Rod::colorStress;
void Gl1_Rod::go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2, const GLViewInfo&){
	const Rod& t(shape->cast<Rod>());
	assert(t.numNodesOk());
	#if 0
		Vector3r color;
		if(!colorStress) color=Vector3r(NaN,NaN,NaN); // keep current
		else {
			stressRange[0]=min(stressRange[0],t.axialStress);
			stressRange[1]=max(stressRange[1],t.axialStress);
			color=CompUtils::scalarOnColorScale(t.axialStress,stressRange[0],stressRange[1]);
		}
	#endif
	glShadeModel(GL_SMOOTH);
	GLUtils::Cylinder(t.nodes[0]->pos+shift,t.nodes[1]->pos+shift,t.radius,/*color*/Vector3r(NaN,NaN,NaN),t.getWire()||wire2,/*caps*/false,t.radius,slices,stacks);
	for(int end=0; end<2; end++){
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
