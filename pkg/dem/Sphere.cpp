#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>

WOO_PLUGIN(dem,(Sphere)(Cg2_Sphere_Sphere_L6Geom)(Cg2_Facet_Sphere_L6Geom)(Cg2_Wall_Sphere_L6Geom)(Cg2_InfCylinder_Sphere_L6Geom)(Cg2_Truss_Sphere_L6Geom)(Bo1_Sphere_Aabb)(In2_Sphere_ElastMat));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_Sphere__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Bo1_Sphere_Aabb__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_In2_Sphere_ElastMat__CLASS_BASE_DOC_ATTRS);

WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Sphere_Sphere_L6Geom__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_InfCylinder_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Truss_Sphere_L6Geom__CLASS_BASE_DOC);


void woo::Sphere::selfTest(const shared_ptr<Particle>& p){
	if(radius<=0) throw std::runtime_error("Sphere #"+to_string(p->id)+": radius must be positive (not "+to_string(radius)+")");
	if(!numNodesOk()) throw std::runtime_error("Sphere #"+to_string(p->id)+": numNodesOk() failed (has "+to_string(nodes.size())+" nodes)");
}

void woo::Sphere::updateDyn(const Real& density) const {
	checkNodesHaveDemData();
	auto& dyn=nodes[0]->getData<DemData>();
	dyn.mass=(4/3.)*M_PI*pow(radius,3)*density;
	dyn.inertia=Vector3r::Ones()*(2./5.)*dyn.mass*pow(radius,2);
};

void Bo1_Sphere_Aabb::go(const shared_ptr<Shape>& sh){
	Sphere& s=sh->cast<Sphere>();
	Vector3r halfSize=(distFactor>0?distFactor:1.)*s.radius*Vector3r::Ones();
	goGeneric(sh, halfSize);
}

void Bo1_Sphere_Aabb::goGeneric(const shared_ptr<Shape>& sh, Vector3r halfSize){
	if(!sh->bound){ sh->bound=make_shared<Aabb>(); }
	Aabb& aabb=sh->bound->cast<Aabb>();
	assert(sh->numNodesOk());
	const Vector3r& pos=sh->nodes[0]->pos;
	if(!scene->isPeriodic){ aabb.min=pos-halfSize; aabb.max=pos+halfSize; return; }

	// adjust box size along axes so that sphere doesn't stick out of the box even if sheared (i.e. parallelepiped)
	if(scene->cell->hasShear()){
		Vector3r refHalfSize(halfSize);
		const Vector3r& cos=scene->cell->getCos();
		for(int i=0; i<3; i++){
			int i1=(i+1)%3,i2=(i+2)%3;
			halfSize[i1]+=.5*refHalfSize[i1]*(1/cos[i]-1);
			halfSize[i2]+=.5*refHalfSize[i2]*(1/cos[i]-1);
		}
	}
	//cerr<<" || "<<halfSize<<endl;
	aabb.min=scene->cell->unshearPt(pos)-halfSize;
	aabb.max=scene->cell->unshearPt(pos)+halfSize;	
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




void In2_Sphere_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle){
	FOREACH(const Particle::MapParticleContact::value_type& I,particle->contacts){
		const shared_ptr<Contact>& C(I.second); if(!C->isReal()) continue;
		Vector3r F,T,xc;
		std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/0,scene);
		#ifdef WOO_DEBUG
			const Particle *pA=C->leakPA(), *pB=C->leakPB();
			bool isPA=(pA==particle.get()); // int sign=(isPA?1:-1);
			if(isnan(F[0])||isnan(F[1])||isnan(F[2])||isnan(T[0])||isnan(T[1])||isnan(T[2])){
				std::ostringstream oss; oss<<"NaN force/torque on particle #"<<particle->id<<" from ##"<<pA->id<<"+"<<pB->id<<":\n\tF="<<F<<", T="<<T; //"\n\tlocal F="<<C->phys->force*sign<<", T="<<C->phys->torque*sign<<"\n";
				throw std::runtime_error(oss.str().c_str());
			}
			if(watch.maxCoeff()==max(pA->id,pB->id) && watch.minCoeff()==min(pA->id,pB->id)){
				cerr<<"Step "<<scene->step<<": apply ##"<<pA->id<<"+"<<pB->id<<": F="<<F.transpose()<<", T="<<T.transpose()<<endl;
				cerr<<"\t#"<<(isPA?pA->id:pB->id)<<" @ "<<xc.transpose()<<", F="<<F.transpose()<<", T="<<(xc.cross(F)+T).transpose()<<endl;
		}
		#endif
		sh->nodes[0]->getData<DemData>().addForceTorque(F,xc.cross(F)+T);
	}
}


#ifdef WOO_OPENGL
WOO_PLUGIN(gl,(Gl1_Sphere));

#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/base/CompUtils.hpp>

bool Gl1_Sphere::wire;
bool Gl1_Sphere::smooth;
Real Gl1_Sphere::scale;
//bool Gl1_Sphere::stripes;
//bool  Gl1_Sphere::localSpecView;
int  Gl1_Sphere::glutSlices;
int  Gl1_Sphere::glutStacks;
Real  Gl1_Sphere::quality;
vector<Vector3r> Gl1_Sphere::vertices, Gl1_Sphere::faces;
int Gl1_Sphere::glStripedSphereList=-1;
int Gl1_Sphere::glGlutSphereList=-1;
Real  Gl1_Sphere::prevQuality=0;

// called for rendering spheres both and ellipsoids, differing in the scale parameter
void Gl1_Sphere::renderScaledSphere(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2, const GLViewInfo& glInfo, const Real& radius, const Vector3r& scaleAxes){

	const shared_ptr<Node>& n=shape->nodes[0];
	Vector3r dPos=(n->hasData<GlData>()?n->getData<GlData>().dGlPos:Vector3r::Zero());
	GLUtils::setLocalCoords(shape->nodes[0]->pos+shift+dPos,shape->nodes[0]->ori);

	// for rendering ellipsoid
	if(!isnan(scaleAxes[0])) glScalev(scaleAxes);

	glClearDepth(1.0f);
	glEnable(GL_NORMALIZE);

	if(quality>10) quality=10; // insane setting can quickly kill the GPU

	Real r=radius*scale;
	//glColor3v(CompUtils::mapColor(shape->getBaseColor()));
	bool doPoints=(Renderer::fastDraw || quality<0 || (int)(quality*glutSlices)<2 || (int)(quality*glutStacks)<2);
	if(doPoints){
		if(smooth) glEnable(GL_POINT_SMOOTH);
		else glDisable(GL_POINT_SMOOTH);
		glPointSize(1.);
		glBegin(GL_POINTS);
			glVertex3v(Vector3r(0,0,0));
		glEnd();

	} else if (wire || wire2 ){
		glLineWidth(1.);
		if(!smooth) glDisable(GL_LINE_SMOOTH);
		glutWireSphere(r,quality*glutSlices,quality*glutStacks);
		if(!smooth) glEnable(GL_LINE_SMOOTH); // re-enable
	}
	else {
		glEnable(GL_LIGHTING);
		glShadeModel(GL_SMOOTH);
		glutSolidSphere(r,quality*glutSlices,quality*glutStacks);
	#if 0
		//Check if quality has been modified or if previous lists are invalidated (e.g. by creating a new qt view), then regenerate lists
		bool somethingChanged = (abs(quality-prevQuality)>0.001 || glIsList(glStripedSphereList)!=GL_TRUE);
		if (somethingChanged) {initStripedGlList(); initGlutGlList(); prevQuality=quality;}
		glScalef(r,r,r);
		if(stripes) glCallList(glStripedSphereList);
		else glCallList(glGlutSphereList);
	#endif
	}
	return;
}

void Gl1_Sphere::subdivideTriangle(Vector3r& v1,Vector3r& v2,Vector3r& v3, int depth){
	Vector3r v;
	//Change color only at the appropriate level, i.e. 8 times in total, since we draw 8 mono-color sectors one after another
	if (depth==int(quality) || quality<=0){
		v = (v1+v2+v3)/3.0;
		GLfloat matEmit[4];
		if (v[1]*v[0]*v[2]>0){
			matEmit[0] = 0.3;
			matEmit[1] = 0.3;
			matEmit[2] = 0.3;
			matEmit[3] = 1.f;
		}else{
			matEmit[0] = 0.15;
			matEmit[1] = 0.15;
			matEmit[2] = 0.15;
			matEmit[3] = 0.2;
		}
 		glMaterialfv(GL_FRONT, GL_EMISSION, matEmit);
	}
	if (depth==1){//Then display 4 triangles
		Vector3r v12 = v1+v2;
		Vector3r v23 = v2+v3;
		Vector3r v31 = v3+v1;
		v12.normalize();
		v23.normalize();
		v31.normalize();
		//Use TRIANGLE_STRIP for faster display of adjacent facets
		glBegin(GL_TRIANGLE_STRIP);
			glNormal3v(v1); glVertex3v(v1);
			glNormal3v(v31); glVertex3v(v31);
			glNormal3v(v12); glVertex3v(v12);
			glNormal3v(v23); glVertex3v(v23);
			glNormal3v(v2); glVertex3v(v2);
		glEnd();
		//terminate with this triangle left behind
		glBegin(GL_TRIANGLES);
			glNormal3v(v3); glVertex3v(v3);
			glNormal3v(v23); glVertex3v(v23);
			glNormal3v(v31); glVertex3v(v31);
		glEnd();
		return;
	}
	Vector3r v12 = v1+v2;
	Vector3r v23 = v2+v3;
	Vector3r v31 = v3+v1;
	v12.normalize();
	v23.normalize();
	v31.normalize();
	subdivideTriangle(v1,v12,v31,depth-1);
	subdivideTriangle(v2,v23,v12,depth-1);
	subdivideTriangle(v3,v31,v23,depth-1);
	subdivideTriangle(v12,v23,v31,depth-1);
}

void Gl1_Sphere::initStripedGlList() {
	if (!vertices.size()){//Fill vectors with vertices and facets
		//Define 6 points for +/- coordinates
		vertices.push_back(Vector3r(-1,0,0));//0
		vertices.push_back(Vector3r(1,0,0));//1
		vertices.push_back(Vector3r(0,-1,0));//2
		vertices.push_back(Vector3r(0,1,0));//3
		vertices.push_back(Vector3r(0,0,-1));//4
		vertices.push_back(Vector3r(0,0,1));//5
		//Define 8 sectors of the sphere
		faces.push_back(Vector3r(3,4,1));
		faces.push_back(Vector3r(3,0,4));
		faces.push_back(Vector3r(3,5,0));
		faces.push_back(Vector3r(3,1,5));
		faces.push_back(Vector3r(2,1,4));
		faces.push_back(Vector3r(2,4,0));
		faces.push_back(Vector3r(2,0,5));
		faces.push_back(Vector3r(2,5,1));
	}
	//Generate the list. Only once for each qtView, or more if quality is modified.
	glDeleteLists(glStripedSphereList,1);
	glStripedSphereList = glGenLists(1);
	glNewList(glStripedSphereList,GL_COMPILE);
	glEnable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
	// render the sphere now
	for (int i=0;i<8;i++)
		subdivideTriangle(vertices[(unsigned int)faces[i][0]],vertices[(unsigned int)faces[i][1]],vertices[(unsigned int)faces[i][2]],1+ (int) quality);
	glEndList();

}

void Gl1_Sphere::initGlutGlList(){
	//Generate the "no-stripes" display list, each time quality is modified
	glDeleteLists(glGlutSphereList,1);
	glGlutSphereList = glGenLists(1);
	glNewList(glGlutSphereList,GL_COMPILE);
		glEnable(GL_LIGHTING);
		glShadeModel(GL_SMOOTH);
		glutSolidSphere(1.0,max(quality*glutSlices,2.),max(quality*glutStacks,3.));
	glEndList();
}

#endif
