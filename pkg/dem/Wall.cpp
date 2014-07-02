// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<limits>

WOO_PLUGIN(dem,(Wall)(Bo1_Wall_Aabb)(In2_Wall_ElastMat)(Cg2_Wall_Sphere_L6Geom)(Cg2_Wall_Facet_L6Geom));
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Wall))
#endif

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_Wall__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Bo1_Wall_Aabb__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_In2_Wall_ElastMat__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Sphere_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Facet_L6Geom__CLASS_BASE_DOC);

void Wall::updateMassInertia(const Real& density) const {
	checkNodesHaveDemData();
	auto& dyn(nodes[0]->getData<DemData>());
	dyn.mass=0;
	dyn.inertia=Vector3r::Zero();
}

void Bo1_Wall_Aabb::go(const shared_ptr<Shape>& sh){
	Wall& wall=sh->cast<Wall>();
	if(!wall.bound){ wall.bound=make_shared<Aabb>(); /* ignore node rotation*/ wall.bound->cast<Aabb>().maxRot=-1;  }
	assert(wall.numNodesOk());
	Aabb& aabb=wall.bound->cast<Aabb>();
	if(scene->isPeriodic && scene->cell->hasShear()) throw logic_error(__FILE__ ": Walls not supported in skewed (Scene.cell.trsf is not diagonal) periodic boundary conditions.");
	const Real& inf=std::numeric_limits<Real>::infinity();
	aabb.min=Vector3r(-inf,-inf,-inf); aabb.max=Vector3r( inf, inf, inf);
	aabb.min[wall.axis]=aabb.max[wall.axis]=sh->nodes[0]->pos[wall.axis];
}

void In2_Wall_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle){
	FOREACH(const Particle::MapParticleContact::value_type& I,particle->contacts){
		const shared_ptr<Contact>& C(I.second); if(!C->isReal()) continue;
		Vector3r F,T,xc;
		std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/0,scene);
		sh->nodes[0]->getData<DemData>().addForceTorque(F,/*discard any torque on wall*/Vector3r::Zero());
	}
}

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
		else normal[ax]=C->geom->cast<L6Geom>().trsf.col(0)[ax]; // QQQ
	}
	else normal[ax]=(sense==1?1.:-1);
	Real uN=normal[ax]*dist-radius; // takes in account sense, radius and distance

	// this may not happen anymore as per conditions above
	assert(!(C->geom && C->geom->cast<L6Geom>().trsf.col(0)!=normal)); // QQQ
	#if 0
		// check that the normal did not change orientation (would be abrupt here)
		if(C->geom && C->geom->cast<L6Geom>().trsf.col(0)!=normal.transpose()){
			throw std::logic_error((boost::format("Cg2_Wall_Sphere_L6Geom: normal changed from %s to %s in Wall+Sphere ##%d+%d (with Wall.sense=0, a particle might cross the Wall plane if Δt is too high, repulsive force to small or velocity too high.")%C->geom->cast<L6Geom>().trsf.col(0)%normal.transpose()%C->leakPA()->id%C->leakPB()->id).str());
		}
	#endif

	const DemData& dyn1(wall.nodes[0]->getData<DemData>());const DemData& dyn2(sphere.nodes[0]->getData<DemData>());
	handleSpheresLikeContact(C,wallPos,dyn1.vel,dyn1.angVel,spherePos,dyn2.vel,dyn2.angVel,normal,contPt,uN,/*r1*/-radius,radius);
	return true;
};

CREATE_LOGGER(Cg2_Wall_Facet_L6Geom);
bool Cg2_Wall_Facet_L6Geom::go(const shared_ptr<Shape>& sh1, const shared_ptr<Shape>& sh2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	if(scene->isPeriodic && scene->cell->hasShear()) throw std::logic_error("Cg2_Wall_Facet_L6Geom does not handle periodic boundary conditions with skew (Scene.cell.trsf is not diagonal).");
	const Wall& wall=sh1->cast<Wall>(); const Facet& facet=sh2->cast<Facet>();
	assert(wall.numNodesOk()); assert(facet.numNodesOk());
	if(!(facet.halfThick>0.)){
		LOG_WARN("Cg2_Wall_Facet_L6Geom: Contact of Wall with zero-thickness facet is always false.");
		return false;
	}
	const Real& radius=facet.halfThick; const int& ax=wall.axis; const int& sense=wall.sense;
	const Vector3r& wallPos=wall.nodes[0]->pos;
	Vector3r fPos[]={facet.nodes[0]->pos+shift2,facet.nodes[1]->pos+shift2,facet.nodes[2]->pos+shift2};
	Eigen::Array<Real,3,1> dist(fPos[0][ax]-wallPos[ax],fPos[1][ax]-wallPos[ax],fPos[2][ax]-wallPos[ax]);
	if(!C->isReal() && abs(dist[0])>radius && abs(dist[1])>radius && abs(dist[2])>radius && !force) return false;
	Vector3r normal=Vector3r::Zero();
	if(sense==0){
		// for new contacts, normal is given by the sense the wall is being approached
		// use average distance (which means distance to the facet midpoint) to determine that
		if(!C->geom) normal[ax]=dist.sum()>0?1.:-1.;
		// use the previous normal for existing contacts
		else normal[ax]=C->geom->cast<L6Geom>().trsf.col(0)[ax];
	}
	else normal[ax]=(sense==1?1.:-1);
	Vector3r contPt=Vector3r::Zero();
	Real contPtWeightSum=0.;
	Real uN=Inf;
	short minIx=-1;
	for(int i:{0,1,2}){
		// negative uN0 means overlap
		Real uNi=dist[i]*normal[ax]-radius; 
		// minimal distance vertex
		if(uNi<uN){ uN=uNi; minIx=i; }
		// with no overlap, skip the vertex, it will not contribute to the contact point
		if(uNi>=0) continue;
		Vector3r c=fPos[i];
		const Real& weight=uNi;
		contPt+=c*weight;
		contPtWeightSum+=weight;
	}
	// some vertices overlapping, use weighted average on those
	if(contPtWeightSum!=0.) contPt/=contPtWeightSum;
	// no overlapping vertices, use the closest one
	else contPt=fPos[minIx]; 
	contPt[ax]=wallPos[ax];

	Vector3r fCenter=facet.getCentroid();
	Vector3r fLinVel,fAngVel;
	std::tie(fLinVel,fAngVel)=facet.interpolatePtLinAngVel(fCenter);

	const DemData& wallDyn(wall.nodes[0]->getData<DemData>());
	handleSpheresLikeContact(C,wallPos,wallDyn.vel,wallDyn.angVel,fCenter,fLinVel,fAngVel,normal,contPt,uN,/*r1*/-radius,radius);
	return true;
}





#ifdef WOO_OPENGL
	#include<woo/lib/opengl/OpenGLWrapper.hpp>
	#include<woo/pkg/gl/Renderer.hpp>
	#include<woo/lib/base/CompUtils.hpp>
	#include<woo/lib/opengl/GLUtils.hpp>
	CREATE_LOGGER(Gl1_Wall);

	int  Gl1_Wall::div=20;
	void Gl1_Wall::go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){
		const Wall& wall=shape->cast<Wall>();
		int ax0=wall.axis,ax1=(wall.axis+1)%3,ax2=(wall.axis+2)%3;
		const Vector3r& pos(wall.nodes[0]->pos);
		// rotate if the wall is rotated
		if(wall.nodes[0]->ori!=Quaternionr::Identity()){
			AngleAxisr aa(wall.nodes[0]->ori);
			if(abs(aa.axis()[ax1])>1e-9 || abs(aa.axis()[ax2])>1e-9) LOG_ERROR("Rotation of wall does not respect its Wall.axis="<<wall.axis<<": rotated around "<<aa.axis()<<" by "<<aa.angle()<<" rad.");
			glRotatef(aa.angle()*(180./M_PI),aa.axis()[0],aa.axis()[1],aa.axis()[2]);
		}
		//glColor3v(CompUtils::mapColor(shape->getBaseColor()));
		glLineWidth(1);
		assert(wall.nodes.size()==1);
		// corner of the drawn plane is at the edge of the visible scene, except for the axis sense, which is in the wall plane
		Vector3r A, unit1, unit2;
		if(scene->isPeriodic){
			assert(!scene->cell->hasShear());
			const Vector3r& hSize=scene->cell->getHSize().diagonal();
			A=Vector3r::Zero();
			A[ax0]=CompUtils::wrapNum(pos[ax0],hSize[ax0]);
			unit1=Vector3r::Unit(ax1)*hSize[ax1]/div;
			unit2=Vector3r::Unit(ax2)*hSize[ax2]/div;
		} else if(isnan(wall.glAB.min()[0])){
			A=viewInfo.sceneCenter-Vector3r::Ones()*viewInfo.sceneRadius;
			A[ax0]=pos[ax0];
			unit1=Vector3r::Unit(ax1)*2*viewInfo.sceneRadius/div;
			unit2=Vector3r::Unit(ax2)*2*viewInfo.sceneRadius/div;
		} else {
			A[ax0]=pos[ax0];
			A[ax1]=pos[ax1]+wall.glAB.min()[0];
			A[ax2]=pos[ax2]+wall.glAB.min()[1];
			unit1=Vector3r::Unit(ax1)*wall.glAB.sizes()[0]/div;
			unit2=Vector3r::Unit(ax2)*wall.glAB.sizes()[1]/div;
		}
		if(wall.nodes[0]->hasData<GlData>()) A[ax0]+=wall.nodes[0]->getData<GlData>().dGlPos[ax0];
		glDisable(GL_LINE_SMOOTH);
		GLUtils::Grid(A,unit1,unit2,Vector2i(div,div),/*edgeMask*/0);
		glEnable(GL_LINE_SMOOTH);
	}
#endif
