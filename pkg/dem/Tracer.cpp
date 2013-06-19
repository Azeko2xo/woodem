#ifdef WOO_OPENGL

#include<woo/pkg/dem/Tracer.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>

#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/gl/Renderer.hpp> // for displacement scaling

WOO_PLUGIN(gl,(TraceGlRep));
WOO_PLUGIN(dem,(Tracer));

void TraceGlRep::compress(int ratio){
	int i;
	int skip=(Tracer::compSkip<0?ratio:Tracer::compSkip);
	for(i=0; ratio*i+skip<(int)pts.size(); i++){
		pts[i]=pts[ratio*i+skip];
		scalars[i]=scalars[ratio*i+skip];
	}
	writeIx=i;
}
void TraceGlRep::addPoint(const Vector3r& p, const Real& scalar){
	if(flags&FLAG_MINDIST){
		size_t lastIx=(writeIx>0?writeIx-1:pts.size());
		if((p-pts[lastIx]).norm()<Tracer::minDist) return;
	}
	pts[writeIx]=p;
	scalars[writeIx]=scalar;
	if(flags&FLAG_COMPRESS){
		if(writeIx>=pts.size()-1) compress(Tracer::compress);
		else writeIx+=1;
	} else {
		writeIx=(writeIx+1)%pts.size();
	}
}

void TraceGlRep::render(const shared_ptr<Node>& n, const GLViewInfo*){
	if(isHidden()) return;
	if(!Tracer::glSmooth) glDisable(GL_LINE_SMOOTH);
	else glEnable(GL_LINE_SMOOTH);
	bool scale=(Renderer::dispScale!=Vector3r::Ones() && Renderer::scaleOn && n->hasData<GlData>());
	glLineWidth(Tracer::glWidth);
	glBegin(GL_LINE_STRIP);
		for(size_t i=0; i<pts.size(); i++){
			size_t ix;
			// compressed start from the very beginning, till they reach the write position
			if(flags&FLAG_COMPRESS){
				ix=i;
				if(ix>=writeIx) break;
			// cycle through the array
			} else {
				ix=(writeIx+i)%pts.size();
			}
			if(!isnan(pts[ix][0])){
				if(isnan(scalars[ix])){
					// if there is no scalar and no scalar should be saved, color by history position
					if(Tracer::scalar==Tracer::SCALAR_NONE) glColor3v(Tracer::lineColor->color((flags&FLAG_COMPRESS ? i*1./writeIx : i*1./pts.size())));
					// if other scalars are saved, use noneColor to not destroy Tracer::lineColor range by auto-adjusting to bogus
					else glColor3v(Tracer::noneColor);
				}
				else glColor3v(Tracer::lineColor->color(scalars[ix]));
				if(!scale) glVertex3v(pts[ix]);
				else{
					const auto& gl=n->getData<GlData>();
					// don't scale if refpos is invalid
					if(isnan(gl.refPos.maxCoeff())) glVertex3v(pts[ix]); 
					// x+(s-1)*(x-x0)
					else glVertex3v((pts[i]+((Renderer::dispScale-Vector3r::Ones()).array()*(pts[i]-gl.refPos).array()).matrix()).eval());
				}
			}
		}
	glEnd();
}

void TraceGlRep::resize(size_t size){
	pts.resize(size,Vector3r(NaN,NaN,NaN));
	scalars.resize(size,NaN);
}

void TraceGlRep::consolidate(){
	if(flags&FLAG_COMPRESS){
		// compressed traces are always sequential, only discard invalid tail data
		resize(writeIx);
	}
	if(writeIx==0) return; // just filled up, nothing to do
	if(pts.size()<2) return; // strange things would happen here
	for(size_t i=0; i<pts.size(); i++){
		size_t j=(i+writeIx)%pts.size(); // i≠j since writeIx≠0
		std::swap(pts[i],pts[j]);
		std::swap(scalars[i],scalars[j]);
	}
	writeIx=0;
}

Vector2i Tracer::modulo;
Vector2r Tracer::rRange;
int Tracer::num;
int Tracer::scalar;
int Tracer::lastScalar;
int Tracer::compress;
int Tracer::compSkip;
bool Tracer::glSmooth;
bool Tracer::clumps;
int Tracer::glWidth;
Vector3r Tracer::noneColor;
Real Tracer::minDist;
shared_ptr<ScalarRange> Tracer::lineColor;

void Tracer::resetNodesRep(bool setupEmpty, bool includeDead){
	auto& dem=field->cast<DemField>();
	boost::mutex::scoped_lock lock(dem.nodesMutex);
	//for(const auto& p: *dem.particles){
	//	for(const auto& n: p->shape->nodes){
	for(const auto& n: dem.nodes){
			n->rep.reset();
			if(setupEmpty){
				n->rep=make_shared<TraceGlRep>();
				auto& tr=n->rep->cast<TraceGlRep>();
				tr.resize(num);
				tr.flags=(compress>0?TraceGlRep::FLAG_COMPRESS:0) | (minDist>0?TraceGlRep::FLAG_MINDIST:0);
			}
		}
	//}
	if(includeDead){
		for(const auto& n: dem.deadNodes){
			n->rep.reset();
		}
	}
}

void Tracer::run(){
	if(scalar!=lastScalar){
		resetNodesRep(/*setup empty*/true,/*includeDead*/false);
		lastScalar=scalar;
		lineColor->reset();
	}
	switch(scalar){
		case SCALAR_NONE: lineColor->label="[index]"; break;
		case SCALAR_TIME: lineColor->label="time"; break;
		case SCALAR_VEL: lineColor->label="|vel|"; break;
		case SCALAR_SIGNED_ACCEL: lineColor->label="signed |accel|"; break;
		case SCALAR_RADIUS: lineColor->label="radius"; break;
		case SCALAR_SHAPE_COLOR: lineColor->label="Shape.color"; break;
	}
	auto& dem=field->cast<DemField>();
	size_t i=0;
	for(auto& n: dem.nodes){
		// node added
		if(!n->rep || !dynamic_pointer_cast<TraceGlRep>(n->rep)){
			boost::mutex::scoped_lock lock(dem.nodesMutex);
			n->rep=make_shared<TraceGlRep>();
			auto& tr=n->rep->cast<TraceGlRep>();
			tr.resize(num);
			tr.flags=(compress>0?TraceGlRep::FLAG_COMPRESS:0) | (minDist>0?TraceGlRep::FLAG_MINDIST:0);
		}
		auto& tr=n->rep->cast<TraceGlRep>();
		const auto& dyn(n->getData<DemData>());
		bool hasP=!dyn.parRef.empty();
		const auto& pI(dyn.parRef.begin()); // use the iterator pI is hasP is true
		bool hidden=false;
		Real radius=NaN;
		if(modulo[0]>0 && (i+modulo[1])%modulo[0]!=0) hidden=true;
		if(dyn.isClump() && !clumps) hidden=true;
		// get redius only when actually needed
		if(!hidden && (SCALAR_RADIUS || rRange.maxCoeff()>0)){
			if(hasP && dynamic_pointer_cast<Sphere>((*pI)->shape)) radius=(*pI)->shape->cast<Sphere>().radius;
			else if(dyn.isClump()) radius=dyn.cast<ClumpData>().equivRad;
			if(rRange.maxCoeff()>0) hidden=(isnan(radius) || (rRange[0]>0 && radius<rRange[0]) || (rRange[1]>0 && radius>rRange[1]));
		}
		if(tr.isHidden()!=hidden) tr.setHidden(hidden);
		Real sc;
		switch(scalar){
			case SCALAR_VEL: sc=n->getData<DemData>().vel.norm(); break;
			case SCALAR_SIGNED_ACCEL:{
				const auto& dyn=n->getData<DemData>();
				if(dyn.mass==0) sc=NaN;
				else sc=(dyn.vel.dot(dyn.force)>0?1:-1)*dyn.force.norm()/dyn.mass;
				break;
			}
			case SCALAR_RADIUS: sc=radius; break;
			case SCALAR_SHAPE_COLOR: sc=(hasP?(*pI)->shape->color:NaN); break;
			case SCALAR_TIME: sc=scene->time; break;
			default: sc=NaN;
		}
		tr.addPoint(n->pos,sc);
		i++;
	}
}
#endif
