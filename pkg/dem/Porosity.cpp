#include<yade/pkg/dem/Porosity.hpp>
#include<yade/lib/base/CompUtils.hpp>
#include<yade/pkg/dem/Sphere.hpp>

YADE_PLUGIN(dem,(AnisoPorosityAnalyzer));

#ifdef YADE_OPENGL
YADE_PLUGIN(gl,(GlExtra_AnisoPorosityAnalyzer));
#endif

Real AnisoPorosityAnalyzer::computeOneRay_check(const Vector3r& A, const Vector3r& B, bool vis){
	setField();
	initialize();
	return computeOneRay(A,B,/*storeVis*/true);
}

Real AnisoPorosityAnalyzer::computeOneRay(const Vector3r& A, const Vector3r& B, bool vis){
	Real tot=0; // cummulative intersected length
	Real lenAB=(A-B).norm();
	int i=-1;
	Vector3r rayNorm=(B-A).normalized();
	if(vis){ rayIds.clear(); rayPts.clear(); }
	FOREACH(const SpherePack::Sph& s, pack.pack){
		i++;
		Vector3r P=CompUtils::closestSegmentPt(s.c,A,B);
		Vector3r PC(P-s.c);
		if(PC.squaredNorm()>=pow(s.r,2)) continue; // does not intersect the sphere
		// intersected distance (is perpendicular to the center-closest point segment, with hypotenuse of r)
		Real intersected=2*sqrt(pow(s.r,2)-PC.squaredNorm());
		tot+=intersected;
		if(vis){ rayIds.push_back(s.shadowOf>=0?s.shadowOf:i); rayPts.push_back(P+.5*intersected*rayNorm); rayPts.push_back(P-.5*intersected*rayNorm); }
	}
	return tot/lenAB;
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
			GLUtils::GLDrawNum(analyzer->rayIds[i],.5*(analyzer->rayPts[2*i]+analyzer->rayPts[2*i+1]));
		}
	};
	glLineWidth(1);
}
#endif

