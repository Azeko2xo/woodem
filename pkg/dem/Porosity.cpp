#include<woo/pkg/dem/Porosity.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/pkg/dem/Sphere.hpp>

WOO_PLUGIN(dem,(AnisoPorosityAnalyzer));

#ifdef WOO_OPENGL
WOO_PLUGIN(gl,(GlExtra_AnisoPorosityAnalyzer));
#endif

WOO_IMPL_LOGGER(AnisoPorosityAnalyzer);

Real AnisoPorosityAnalyzer::relSolid(Real theta, Real phi, Vector3r pt0, bool vis){
	setField();
	initialize();
	vector<Vector3r> endPts=splitRay(theta,phi,pt0,scene->cell->hSize);
	assert(endPts.size()%2==0);
	Real solid(0), total(0);
	size_t sz=endPts.size()/2;
	for(size_t i=0; i<sz; i++){
		const Vector3r A(endPts[2*i]), B(endPts[2*i+1]);
		#ifdef WOO_DEBUG
			for(int j:{0,1,2}) if(A[j]<0 || A[j]>scene->cell->getSize()[j] || B[j]<0 || B[j]>scene->cell->getSize()[j]) throw std::logic_error((boost::format("Points outside periodic cell after splitting? %s %s (cell size %s)")%A.transpose()%B.transpose()%scene->cell->getSize().transpose()).str());
		#endif
		solid+=computeOneRay(A,B,vis);
		total+=(B-A).norm();
	}
	return solid/total;
}

vector<Vector3r> AnisoPorosityAnalyzer::splitRay(Real theta, Real phi, Vector3r pt0, const Matrix3r& T /* hSize*/){
	// theta=azimuth, phi=elevation
	Real epsN=1e-6; // tolerance for distances in unit cell
	Matrix3r Tinv=T.inverse(); // hSize is not orthonormal (rotation matrix), therefore inverse!=transpose
	// suffixes: R is real cell, N is normalized cell
	Vector3r unitR(cos(phi)*cos(theta),cos(phi)*sin(theta),sin(phi));
	for(int i:{0,1,2}) if(abs(unitR[i])<epsN) unitR[i]=0;
	Vector3r unitN=Tinv*unitR;
	LOG_TRACE("unitR="<<unitR.transpose()<<"unitN="<<unitN.transpose());
	// for now suppose we want the ray to stop when it reaches the same periodic coordinate, along whichever axis it comes first
	Vector3r rayN=unitN/unitN.array().abs().maxCoeff();
	Real totLenN=rayN.norm();
	// ray start and end in canonical position, inside the cell
	Vector3r pt0N=Tinv*pt0;
	for(int i:{0,1,2}){ pt0N[i]=pt0N[i]-floor(pt0N[i]); }
	vector<Vector3r> rayPtsR;
	bool done=false; int N=0; Real cummLenN=0;
	do{
		// current starting pos at the boundary opposite to ray direction for that axis, if on the boundary
		for(int i: {0,1,2}) if(abs(pt0N[i]-round(pt0N[i]))<epsN) pt0N[i]=(unitN[i]>=0?0:1); 
		// find in which sense we come to cell boundary first, and where the ray will be split
		Real t=/*true max is sqrt(3)*/10.;
		for(int i:{0,1,2}){ if(unitN[i]==0) continue; Real tt=((unitN[i]>0?1:0)-pt0N[i])/unitN[i]; /*cerr<<"[u"<<i<<"="<<unitN[i]<<";t"<<i<<"="<<tt<<"]";*/ if(tt>0) t=min(t,tt); }
		LOG_TRACE("Split "<<N<<" (l="<<cummLenN<<"/"<<totLenN<<"): from "<<pt0N.transpose()<<", unitN="<<unitN<<", t="<<t);
		assert(t>0); assert(t<sqrt(3));
		if(cummLenN+t>=totLenN){ t=totLenN-cummLenN; done=true; }
		else cummLenN+=t;
		// prune tiny segments (but still count their contribution to cummLenN and advance the point)
		if(t>epsN){
			Vector3r aR=T*pt0N, bR=T*(pt0N+unitN*t);
			rayPtsR.push_back(aR); rayPtsR.push_back(bR);
			LOG_TRACE("Added points: local "<<pt0N.transpose()<<" -- "<<(pt0N+unitN*t).transpose()<<"; global "<<aR.transpose()<<" -- "<<bR.transpose());
		}
		pt0N+=unitN*t;
		if(++N>10){
			cerr<<"Computed ray points:\npts=[";
			for(const Vector3r& p: rayPtsR) cerr<<"Vector3("<<p.transpose()<<"),";
			cerr<<"]";
			throw std::logic_error("Too many ray segments, bug?!");
		}
	} while(!done);
	return rayPtsR;
};

Real AnisoPorosityAnalyzer::computeOneRay_angles_check(Real theta, Real phi, bool vis){
	setField();
	initialize();

	// theta=azimuth, phi=elevation
	if(theta<0 || theta>M_PI/2. || phi<0 || phi>M_PI/2.) throw std::invalid_argument((boost::format("AnisoPorosityAnalyzer.oneRay: value out of valid range: theta=%d (0…π/2), phi=%d (0…π/2)")%theta%phi).str());
	// find boundary points of the cell which has given polar coords
	const Vector3r& csz=scene->cell->getSize();
	Real y_x=csz[1]/csz[0], tt=tan(theta);
	Vector3r pt(Vector3r::Zero());
	if(tt<y_x){ pt[0]=csz[0]; pt[1]=pt[0]*tt; }
	else { pt[1]=csz[1]; pt[0]=pt[1]/tt; }
	Real xy=Vector2r(pt[0],pt[1]).norm();
	Real z_xy=csz[2]/xy, tp=tan(phi);
	if(tp<z_xy){ pt[2]=xy*tp; }
	else{ pt[2]=csz[2]; Real ratio=(csz[2]/tp)/xy; pt[0]*=ratio; pt[1]*=ratio; }

	return computeOneRay(Vector3r::Zero(),pt,vis);
};


Real AnisoPorosityAnalyzer::computeOneRay_check(const Vector3r& A, const Vector3r& B, bool vis){
	setField();
	initialize();
	return computeOneRay(A,B,vis);
}

Real AnisoPorosityAnalyzer::computeOneRay(const Vector3r& A, const Vector3r& B, bool vis){
	Real tot=0; // cummulative intersected length
	Real lenAB=(B-A).norm();
	Vector3r rayDir=(B-A)/lenAB;
	int i=-1;
	FOREACH(const SpherePack::Sph& s, pack.pack){
		i++;
		Real t0,t1;
		int nIntr=CompUtils::lineSphereIntersection(A,rayDir,s.c,s.r,t0,t1);
		if(nIntr<2) continue;
		assert(t1>t0);
		// segment endpoints have parameters 0 and lenAB, we constrain the intersection to that length only
		//cerr<<"#"<<s.shadowOf>=0?s.shadowOf:i<<(s.shadowOf>=0?"s":"")<<": ";	cerr<<"t="<<t0<<","<<t1<<" | ";
		t0=max(0.,t0); t1=min(lenAB,t1);
		//cerr<<"t="<<t0<<","<<t1<<endl;
		// same as: t0=max(0.,min(lenAB,t0)); t1=max(0.,min(lenAB,t1)); if(t1==t0) continue;
		if(t1<=t0) continue; // no intersection on the segment: t0>lenAB or t1<0
		tot+=t1-t0;
		if(vis){ rayIds.push_back(s.shadowOf>=0?s.shadowOf:i); rayPts.push_back(A+t0*rayDir); rayPts.push_back(A+t1*rayDir);	}
	}
	return tot;
}

void AnisoPorosityAnalyzer::initialize(){
	if(!scene->isPeriodic) throw std::runtime_error("AnisoPorosityAnalyzer can only be used with periodic BC");
	if(scene->cell->hasShear()) throw std::runtime_error("AnisoPorosityAnalyzer only works so far with PBC without skew.");
	dem=dynamic_cast<DemField*>(field.get());
	if(scene->step==initStep && dem->particles->size()==initNum) return;
	pack=SpherePack();
	pack.cellSize=scene->cell->getSize();
	FOREACH(const shared_ptr<Particle>& p, *dem->particles){
		const shared_ptr<Sphere> s=dynamic_pointer_cast<Sphere>(p->shape);
		if(!s || s->nodes.size()==0)continue;
		pack.add(s->nodes[0]->pos,s->radius);
	}
	__attribute__((unused)) int sh=pack.addShadows();
	LOG_DEBUG("Added "<<sh<<" shadow spheres");
	initStep=scene->step;
	initNum=dem->particles->size();
}

void AnisoPorosityAnalyzer::run(){
	throw std::runtime_error("AnisoPorosityAnalyzer::run not yet implemented.");
}

#ifdef WOO_OPENGL
#include<woo/lib/opengl/GLUtils.hpp>

void GlExtra_AnisoPorosityAnalyzer::render(){
	if(analyzer && analyzer->scene!=scene) analyzer=shared_ptr<AnisoPorosityAnalyzer>();
	if(!analyzer) return;
	size_t sz=min(analyzer->rayIds.size(),analyzer->rayPts.size()/2); // assert(analyzer->rayPts.size()==2*sz);
	glLineWidth(wd);
	// for(size_t i=0; i<sz; i++) GLUtils::GLDrawLine(analyzer->rayPts[2*i],analyzer->rayPts[2*i+1],CompUtils::mapColor(idColor(analyzer->rayIds[i])));
	glBegin(GL_LINES);
		for(size_t i=0; i<sz; i++){ glColor3v(CompUtils::mapColor(idColor(analyzer->rayIds[i]))); glVertex3v(analyzer->rayPts[2*i]); glVertex3v(analyzer->rayPts[2*i+1]); }
	glEnd();
	if(num>0){
		switch(num){
			case 1: for(size_t i=0; i<sz; i++){ glColor3v(CompUtils::mapColor(idColor(analyzer->rayIds[i]))); GLUtils::GLDrawInt(analyzer->rayIds[i],.5*(analyzer->rayPts[2*i]+analyzer->rayPts[2*i+1])); } break; 
			case 2: for(size_t i=0; i<sz; i++){ glColor3v(CompUtils::mapColor(idColor(analyzer->rayIds[i]))); GLUtils::GLDrawNum((analyzer->rayPts[2*i+1]-analyzer->rayPts[2*i]).norm(),.5*(analyzer->rayPts[2*i]+analyzer->rayPts[2*i+1])); } break;
		}
	};
	glLineWidth(1);
}
#endif

