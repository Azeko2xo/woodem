#ifdef WOO_OPENGL

#include<woo/pkg/dem/Tracer.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>

#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/gl/Renderer.hpp> // for displacement scaling
#include<woo/pkg/dem/Gl1_DemField.hpp> // for setOurSceneRanges

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

vector<Vector3r> TraceGlRep::pyPts_get() const{
	size_t i=0; vector<Vector3r> ret;
	Vector3r pt; Real scalar;
	while(getPointData(i++,pt,scalar)) ret.push_back(pt);
	return ret;
}

vector<Real> TraceGlRep::pyScalars_get() const{
	size_t i=0; vector<Real> ret;
	Vector3r pt; Real scalar;
	while(getPointData(i++,pt,scalar)) ret.push_back(scalar);
	return ret;
}


bool TraceGlRep::getPointData(size_t i, Vector3r& pt, Real& scalar) const {
	size_t ix;
	if(flags&FLAG_COMPRESS){
		ix=i;
		if(ix>=writeIx) return false;
	} else { // cycles through the array
		if(i>=pts.size()) return false;
		ix=(writeIx+i)%pts.size();
	}
	pt=pts[ix];
	scalar=scalars[ix];
	return true;
}

void TraceGlRep::render(const shared_ptr<Node>& n, const GLViewInfo* glInfo){
	if(isHidden()) return;
	if(!Tracer::glSmooth) glDisable(GL_LINE_SMOOTH);
	else glEnable(GL_LINE_SMOOTH);
	bool scale=(Renderer::dispScale!=Vector3r::Ones() && Renderer::scaleOn && n->hasData<GlData>());
	glLineWidth(Tracer::glWidth);
	const bool periodic=glInfo->scene->isPeriodic;
	Vector3i prevPeriod=Vector3i::Zero(); // silence gcc warning maybe-uninitialized
	glBegin(GL_LINE_STRIP);
		for(size_t i=0; i<pts.size(); i++){
			size_t ix;
			// FIXME: use getPointData here (copied code)
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
				Vector3r pt(pts[ix]);
				if(periodic){
					// canonicalize the point, store the period
					Vector3i currPeriod;
					pt=glInfo->scene->cell->canonicalizePt(pt,currPeriod);
					// if the period changes between these two points, split the line (and don't render the segment in-between for simplicity)
					if(i>0 && currPeriod!=prevPeriod){
						glEnd();
						glBegin(GL_LINE_STRIP);
					}
					prevPeriod=currPeriod;
				}
				if(!scale) glVertex3v(pt);
				else{
					const auto& gl=n->getData<GlData>();
					// don't scale if refpos is invalid
					if(isnan(gl.refPos.maxCoeff())) glVertex3v(pt); 
					// x+(s-1)*(x-x0)
					else glVertex3v((pt+((Renderer::dispScale-Vector3r::Ones()).array()*(pt-gl.refPos).array()).matrix()).eval());
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
int Tracer::ordinalMod;
Vector2r Tracer::rRange;
int Tracer::num;
int Tracer::scalar;
int Tracer::vecAxis;
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
			/*
				FIXME: there is some bug in boost::python's shared_ptr allocator, so if a node has GlRep
				assigned from Python, we might crash here -- like this:

					#4  <signal handler called>
					#5  0x00000000004845d8 in ?? ()
					#6  0x00007fac585be422 in boost::python::converter::shared_ptr_deleter::operator()(void const*) () from /usr/lib/x86_64-linux-gnu/libboost_python-py27.so.1.53.0
					#7  0x00007fac568cc41e in boost::detail::sp_counted_base::release (this=0x5d6d940) at /usr/include/boost/smart_ptr/detail/sp_counted_base_gcc_x86.hpp:146
					#8  0x00007fac56aaed7b in ~shared_count (this=<optimized out>, __in_chrg=<optimized out>) at /usr/include/boost/smart_ptr/detail/shared_count.hpp:371
					#9  ~shared_ptr (this=<optimized out>, __in_chrg=<optimized out>) at /usr/include/boost/smart_ptr/shared_ptr.hpp:328
					#10 operator=<TraceGlRep> (r=<unknown type in /usr/local/lib/python2.7/dist-packages/woo/_cxxInternal_mt_debug.so, CU 0x13c9b4f, DIE 0x1cc8cb9>, this=<optimized out>) at /usr/include/boost/smart_ptr/shared_ptr.hpp:601
					#11 Tracer::resetNodesRep (this=this@entry=0x5066d20, setupEmpty=setupEmpty@entry=true, includeDead=includeDead@entry=false) at /home/eudoxos/woo/pkg/dem/Tracer.cpp:119

				As workaround, set the tracerSkip flag on the node and the tracer will leave it alone.

			
				CAUSE: see https://svn.boost.org/trac/boost/ticket/8290 and pkg/dem/ParticleContainer.cpp
			*/
			if(n->getData<DemData>().isTracerSkip()) continue;
			GilLock lock; // get GIL to avoid crash when destroying python-instantiated object
			if(setupEmpty){
				n->rep=make_shared<TraceGlRep>();
				auto& tr=n->rep->cast<TraceGlRep>();
				tr.resize(num);
				tr.flags=(compress>0?TraceGlRep::FLAG_COMPRESS:0) | (minDist>0?TraceGlRep::FLAG_MINDIST:0);
			} else {
				n->rep.reset();
			}
		}
	//}
	if(includeDead){
		for(const auto& n: dem.deadNodes){
			if(n->getData<DemData>().isTracerSkip()) continue;
			GilLock lock; // get GIL to avoid crash, see above
			n->rep.reset();
		}
	}
}

void Tracer::showHideRange(bool show){
	// show lineColor
	if(show) Gl1_DemField::setOurSceneRanges(scene,{lineColor},{lineColor});
	// hide lineColor
	else Gl1_DemField::setOurSceneRanges(scene,{lineColor},{});
}

void Tracer::run(){
	if(scalar!=lastScalar){
		resetNodesRep(/*setup empty*/true,/*includeDead*/false);
		lastScalar=scalar;
		lineColor->reset();
	}
	showHideRange(/*show*/true);
	switch(scalar){
		case SCALAR_NONE: lineColor->label="[index]"; break;
		case SCALAR_TIME: lineColor->label="time"; break;
		case SCALAR_VEL: lineColor->label="vel"; break;
		case SCALAR_ANGVEL: lineColor->label="angVel"; break;
		case SCALAR_SIGNED_ACCEL: lineColor->label="signed |accel|"; break;
		case SCALAR_RADIUS: lineColor->label="radius"; break;
		case SCALAR_SHAPE_COLOR: lineColor->label="Shape.color"; break;
		case SCALAR_ORDINAL: lineColor->label="ordinal"+(ordinalMod>1?string(" % "+to_string(ordinalMod)):string());
		case SCALAR_KINETIC: lineColor->label="kinetic energy"; break;
		break;
	}
	// add component to the label
	if(scalar==SCALAR_VEL || scalar==SCALAR_ANGVEL){
		switch(vecAxis){
			case -1: lineColor->label="|"+lineColor->label+"|"; break;
			case 0: lineColor->label+="_x"; break;
			case 1: lineColor->label+="_y"; break;
			case 2: lineColor->label+="_z"; break;
		}
	}
	auto& dem=field->cast<DemField>();
	size_t i=0;
	for(auto& n: dem.nodes){
		const auto& dyn(n->getData<DemData>());
		if(dyn.isTracerSkip()) continue;
		// node added
		if(!n->rep || !n->rep->isA<TraceGlRep>()){
			boost::mutex::scoped_lock lock(dem.nodesMutex);
			n->rep=make_shared<TraceGlRep>();
			auto& tr=n->rep->cast<TraceGlRep>();
			tr.resize(num);
			tr.flags=(compress>0?TraceGlRep::FLAG_COMPRESS:0) | (minDist>0?TraceGlRep::FLAG_MINDIST:0);
		}
		auto& tr=n->rep->cast<TraceGlRep>();
		bool hasP=!dyn.parRef.empty();
		bool hidden=false;
		Real radius=NaN;
		if(modulo[0]>0 && (i+modulo[1])%modulo[0]!=0) hidden=true;
		if(dyn.isClump() && !clumps) hidden=true;
		// if the node has no particle, hide it when radius is the scalar
		if(!hasP && scalar==SCALAR_RADIUS) hidden=true;
		// get redius only when actually needed
		if(!hidden && (scalar==SCALAR_RADIUS || rRange.maxCoeff()>0)){
			const auto& pI(dyn.parRef.begin());
			if(hasP && dynamic_pointer_cast<Sphere>((*pI)->shape)) radius=(*pI)->shape->cast<Sphere>().radius;
			else if(dyn.isClump()) radius=dyn.cast<ClumpData>().equivRad;
			if(rRange.maxCoeff()>0) hidden=(isnan(radius) || (rRange[0]>0 && radius<rRange[0]) || (rRange[1]>0 && radius>rRange[1]));
		}
		if(tr.isHidden()!=hidden) tr.setHidden(hidden);
		Real sc;
		auto vecNormXyz=[&](const Vector3r& v)->Real{ if(vecAxis<0||vecAxis>2) return v.norm(); return v[vecAxis]; };
		switch(scalar){
			case SCALAR_VEL: sc=vecNormXyz(n->getData<DemData>().vel); break;
			case SCALAR_ANGVEL: sc=vecNormXyz(n->getData<DemData>().angVel); break;
			case SCALAR_SIGNED_ACCEL:{
				const auto& dyn=n->getData<DemData>();
				if(dyn.mass==0) sc=NaN;
				else sc=(dyn.vel.dot(dyn.force)>0?1:-1)*dyn.force.norm()/dyn.mass;
				break;
			}
			case SCALAR_RADIUS: sc=radius; break;
			case SCALAR_SHAPE_COLOR: sc=(hasP?(*dyn.parRef.begin())->shape->color:NaN); break;
			case SCALAR_TIME: sc=scene->time; break;
			case SCALAR_ORDINAL: sc=(i%ordinalMod); break;
			case SCALAR_KINETIC: sc=dyn.getEk_any(n,/*trans*/true,/*rot*/true,scene); break;
			default: sc=NaN;
		}
		tr.addPoint(n->pos,sc);
		i++;
	}
}
#endif
