#include<yade/pkg/dem/Porosity.hpp>
#include<yade/lib/base/CompUtils.hpp>
#include<yade/pkg/dem/Sphere.hpp>

YADE_PLUGIN(dem,(AnisoPorosityAnalyzer));

#ifdef YADE_OPENGL
YADE_PLUGIN(gl,(GlExtra_AnisoPorosityAnalyzer));
#endif

Real AnisoPorosityAnalyzer::relSolid(Real theta, Real phi, Vector3r pt0){
	setField();
	initialize();
	vector<Vector3r> endPts=splitRay(theta,phi,pt0,scene->cell->hSize);
	assert(endPts.size()%2==0);
	Real solid(0), total(0);
	size_t sz=endPts.size()/2;
	for(size_t i=0; i<sz; i++){
		const Vector3r A(endPts[2*i]), B(endPts[2*i+1]);
		solid+=computeOneRay(A,B,/*vis*/false);
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
		for(int i: {0,1,2}) if(pt0N[i]-floor(pt0N[i])<epsN) pt0N[i]=(unitN[i]>=0?0:1); 
		// find in which sense we come to cell boundary first, and where the ray will be split
		Real t=/*true max is sqrt(3)*/10.;
		for(int i:{0,1,2}){ if(unitN[i]==0) continue; Real tt=((unitN[i]>0?1:0)-pt0N[i])/unitN[i]; /*cerr<<"[u"<<i<<"="<<unitN[i]<<";t"<<i<<"="<<tt<<"]";*/ if(tt>0) t=min(t,tt); }
		LOG_TRACE("Split "<<N<<" (l="<<cummLenN<<"/"<<totLenN<<"): from "<<pt0N.transpose()<<", unitN="<<unitN<<", t="<<t);
		assert(t>0); assert(t<sqrt(3));
		if(cummLenN+t>=totLenN){ t=totLenN-cummLenN; done=true; }
		else cummLenN+=t;
		// prune tiny segments (but still count their contribution to cummLenN and advance the point)
		if(t>epsN){ rayPtsR.push_back(T*pt0N); rayPtsR.push_back(T*(pt0N+unitN*t)); }
		pt0N=pt0N+unitN*t;
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
	if(theta<0 || theta>Mathr::PI/2. || phi<0 || phi>Mathr::PI/2.) throw std::invalid_argument((boost::format("AnisoPorosityAnalyzer.oneRay: value out of valid range: theta=%d (0…π/2), phi=%d (0…π/2)")%theta%phi).str());
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
	return computeOneRay(A,B,/*storeVis*/true);
}

Real AnisoPorosityAnalyzer::computeOneRay(const Vector3r& A, const Vector3r& B, bool vis){
	Real tot=0; // cummulative intersected length
	Vector3r rayDir=(B-A).normalized();
	int i=-1;
	if(vis){ rayIds.clear(); rayPts.clear(); }
	FOREACH(const SpherePack::Sph& s, pack.pack){
		i++;
		Vector3r P=CompUtils::closestSegmentPt(s.c,A,B);
		Vector3r PC(P-s.c);
		if(PC.squaredNorm()>=pow(s.r,2)) continue; // does not intersect the sphere
		// intersected distance (is perpendicular to the center-closest point segment, with hypotenuse of r)
		Real intersected=2*sqrt(pow(s.r,2)-PC.squaredNorm());
		// if one of endpoints is inside this sphere as well, then take just corresponding part
		for(int j=0; j<2; j++){
			const Vector3r& end(j==0?A:B);
			// if the endpoint is inside the sphere, take half of the intersection plus the rest (which may be negative if the dot-product is negative)
			if((end-s.c).squaredNorm()<pow(s.r,2)){ intersected=.5*intersected+(end-P).dot(rayDir*(j==0?-1:1)); break; }
		}
		tot+=intersected;
		if(vis){ rayIds.push_back(s.shadowOf>=0?s.shadowOf:i); rayPts.push_back(P+.5*intersected*rayDir); rayPts.push_back(P-.5*intersected*rayDir); }
	}
	return tot;
}

void AnisoPorosityAnalyzer::initialize(){
	if(!scene->isPeriodic) throw std::runtime_error("AnisoPorosityAnalyzer can only be used with periodic BC");
	if(scene->cell->hasShear()) throw std::runtime_error("AnisoPorosityAnalyzer only works so far with PBC without skew.");
	dem=dynamic_cast<DemField*>(field.get());
	if(scene->step==initStep && dem->particles.size()==initNum) return;
	pack=SpherePack();
	pack.cellSize=scene->cell->getSize();
	FOREACH(const shared_ptr<Particle>& p, dem->particles){
		const shared_ptr<Sphere> s=dynamic_pointer_cast<Sphere>(p->shape);
		if(!s || s->nodes.size()==0)continue;
		pack.add(s->nodes[0]->pos,s->radius);
	}
	int sh=pack.addShadows();
	LOG_WARN("Added "<<sh<<" shadow spheres");
	initStep=scene->step;
	initNum=dem->particles.size();
}

void AnisoPorosityAnalyzer::run(){
	throw std::runtime_error("AnisoPorosityAnalyzer::run not yet implemented.");
}

#ifdef YADE_OPENGL
#include<yade/lib/opengl/GLUtils.hpp>

void GlExtra_AnisoPorosityAnalyzer::render(){
	if(analyzer && analyzer->scene!=scene) analyzer=shared_ptr<AnisoPorosityAnalyzer>();
	if(!analyzer) return;
	glLineWidth(wd);
	glBegin(GL_LINES);
		FOREACH(const Vector3r& v, analyzer->rayPts){
			glVertex3v(v);
		}
	glEnd();
	if(ids){
		size_t sz=analyzer->rayIds.size(); assert(analyzer->rayPts.size()==2*sz);
		for(size_t i=0; i<sz; i++){
			GLUtils::GLDrawInt(analyzer->rayIds[i],.5*(analyzer->rayPts[2*i]+analyzer->rayPts[2*i+1]));
		}
	};
	glLineWidth(1);
}
#endif

