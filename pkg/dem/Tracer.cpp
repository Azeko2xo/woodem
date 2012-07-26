#include<woo/pkg/dem/Tracer.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>

#include<woo/pkg/dem/Sphere.hpp>

WOO_PLUGIN(gl,(TraceGlRep));
WOO_PLUGIN(dem,(Tracer));

void TraceGlRep::compress(int ratio){
	int i;
	int skip=(Tracer::compSkip<0?ratio:Tracer::compSkip);
	for(i=0; ratio*i+skip<pts.size(); i++){
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

void TraceGlRep::render(const shared_ptr<Node>& n, GLViewInfo*){
	glDisable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_STRIP);
		for(size_t i=0; i<pts.size(); i++){
			size_t ix; Real relColor;
			// compressed start from the very beginning, till they reach the write position
			if(flags&FLAG_COMPRESS){
				ix=i;
				relColor=i*1./writeIx;
				if(ix>=writeIx) break;
			// cycle through the array
			} else {
				ix=(writeIx+i)%pts.size();
				relColor=i*1./pts.size();
			}
			if(!isnan(pts[ix][0])){
				if(isnan(scalars[ix])){
					// if there is no scalar and no scalar should be saved, color by history position
					if(Tracer::scalar==Tracer::SCALAR_NONE) glColor3v(Tracer::lineColor->color((flags&FLAG_COMPRESS ? i*1./writeIx : i*1./pts.size())));
					// if other scalars are saved, use noneColor to not destroy Tracer::lineColor range by auto-adjusting to bogus
					else glColor3v(Tracer::noneColor);
				}
				else glColor3v(Tracer::lineColor->color(scalars[ix]));
				glVertex3v(pts[ix]);
			}
		}
	glEnd();
}

Vector2i Tracer::modulo;
Vector2r Tracer::rRange;
int Tracer::num;
int Tracer::scalar;
int Tracer::compress;
int Tracer::compSkip;
Vector3r Tracer::noneColor;
Real Tracer::minDist;
bool Tracer::reset;
shared_ptr<ScalarRange> Tracer::lineColor;

void Tracer::run(){
	auto& dem=field->cast<DemField>();
	for(const auto& p: *dem.particles){
		size_t i=p->id;
		// delete from all
		bool doReset=reset;
		if(!doReset && modulo[0]>0 && (i+modulo[1])%modulo[0]!=0) doReset=true;
		if(!doReset && rRange.maxCoeff()>0
			&& (!dynamic_pointer_cast<Sphere>(p->shape)
				|| (rRange[0]>0 && p->shape->cast<Sphere>().radius<rRange[0])
				|| (rRange[1]>0 && p->shape->cast<Sphere>().radius>rRange[1]))
		) doReset=true;
		if(doReset){
			for(const auto& n: p->shape->nodes) n->rep.reset();
		} else {
			for(const auto& n: p->shape->nodes){
				if(!n->rep || !dynamic_pointer_cast<TraceGlRep>(n->rep)){
					n->rep=make_shared<TraceGlRep>();
					n->rep->cast<TraceGlRep>().pts.resize(num,Vector3r(NaN,NaN,NaN));
					n->rep->cast<TraceGlRep>().scalars.resize(num,NaN);
					if(compress>0) n->rep->cast<TraceGlRep>().flags|=TraceGlRep::FLAG_COMPRESS;
					if(minDist>0) n->rep->cast<TraceGlRep>().flags|=TraceGlRep::FLAG_MINDIST;
				}
				TraceGlRep& tr(n->rep->cast<TraceGlRep>());
				Real sc;
				switch(scalar){
					case SCALAR_VEL: sc=n->getData<DemData>().vel.norm(); break;
					case SCALAR_SIGNED_ACCEL:{
						const auto& dyn=n->getData<DemData>();
						if(dyn.mass==0) sc=NaN;
						else sc=(dyn.vel.dot(dyn.force)>0?1:-1)*dyn.force.norm()/dyn.mass;
						break;
					}
					case SCALAR_RADIUS: sc=(dynamic_pointer_cast<Sphere>(p->shape)?p->shape->cast<Sphere>().radius:NaN); break;
					case SCALAR_TIME: sc=scene->time; break;
					default: sc=NaN;
				}
				tr.addPoint(n->pos,sc);
			}
		}
	}
	switch(scalar){
		case SCALAR_NONE: lineColor->label="[index]"; break;
		case SCALAR_TIME: lineColor->label="time"; break;
		case SCALAR_VEL: lineColor->label="|vel|"; break;
		case SCALAR_SIGNED_ACCEL: lineColor->label="signed |accel|"; break;
		case SCALAR_RADIUS: lineColor->label="radius"; break;
	}
	reset=false;
}
