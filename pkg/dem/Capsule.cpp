#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/dem/Sphere.hpp>

WOO_PLUGIN(dem,(Capsule)(Bo1_Capsule_Aabb)(Cg2_Wall_Capsule_L6Geom)(Cg2_Facet_Capsule_L6Geom)(Cg2_Capsule_Capsule_L6Geom));

WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Capsule_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Capsule_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Capsule_L6Geom__CLASS_BASE_DOC);
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Capsule));
#endif

void Capsule::selfTest(const shared_ptr<Particle>& p){
	if(!(radius>0.) || !(shaft>0.)) throw std::runtime_error("Capsule #"+to_string(p->id)+": both radius and shaft must be positive lengths (current: radius="+to_string(radius)+", shaft="+to_string(shaft)+").");
	if(!numNodesOk()) throw std::runtime_error("Capsule #"+to_string(p->id)+": numNodesOk() failed: must be 1, not "+to_string(nodes.size())+".");
}

void Capsule::updateDyn(const Real& density) const {
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

void Bo1_Capsule_Aabb::go(const shared_ptr<Shape>& sh){
	if(!sh->bound){ sh->bound=make_shared<Aabb>(); }
	Aabb& aabb=sh->bound->cast<Aabb>();
	const auto& c(sh->cast<Capsule>());
	const auto& pos(sh->nodes[0]->pos); const auto& ori(sh->nodes[0]->ori);
	Vector3r dShaft=ori*Vector3r(.5*c.shaft,0,0);
	AlignedBox3r ab;
	for(int a:{-1,1}) for(int b:{-1,1}) ab.extend(pos+a*dShaft+b*c.radius*Vector3r::Ones());
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
	Vector3r dShaft[2]={ori1*Vector3r(c1.shaft/2.,0,0),ori2*Vector3r(c2.shaft/2.,0,0)};
	Vector3r ends[2][2]={{pos1-dShaft[0],pos1+dShaft[0]},{pos2-dShaft[1],pos2+dShaft[1]}};
	Vector2r uv;
	bool parallel;
	Real distSq=CompUtils::distSq_SegmentSegment(ends[0][0],ends[0][1],ends[1][0],ends[1][1],uv,parallel);
	if(!C->isReal() && distSq>pow(c1.radius+c2.radius,2) && !force) return false;
	// TODO
	cerr<<"@";
	return false;
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

