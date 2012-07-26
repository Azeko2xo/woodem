#pragma once 

#include<woo/core/Field.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/Particle.hpp>

struct TraceGlRep: public NodeGlRep{
	void addPoint(const Vector3r& p, const Real& scalar);
	void compress(int ratio);
	void render(const shared_ptr<Node>&,GLViewInfo*);
	enum{FLAG_COMPRESS=1,FLAG_MINDIST=2};
	WOO_CLASS_BASE_DOC_ATTRS(TraceGlRep,NodeGlRep,"Data with node's position history; create by :ref:`Tracer`.",
		((vector<Vector3r>,pts,,,"History points"))
		((vector<Real>,scalars,,,"History scalars"))
		((size_t,writeIx,0,,"Index where next data will be written"))
		((short,flags,0,,"Flags for this instance"))
	);
};
REGISTER_SERIALIZABLE(TraceGlRep);


struct Tracer: public PeriodicEngine{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual void run();
	enum{SCALAR_NONE=0,SCALAR_TIME,SCALAR_VEL,SCALAR_SIGNED_ACCEL,SCALAR_RADIUS};
	WOO_CLASS_BASE_DOC_STATICATTRS_PY(Tracer,PeriodicEngine,"Save trace of node's movement",
		((int,num,128,,"Number of positions to save (when creating new glyph)"))
		((int,compress,2,,"Ratio by which history is compress when all data slots are filled; if 0, cycle and don't compress."))
		((int,compSkip,0,,"Number of leading points to skip during compression; if negative, the value of *compress* is used."))
		((Real,minDist,0,,"Only add new point when last point is at least minDist away, or no point exists at all."))
		((bool,reset,false,,"Reset traces at the next step"))
		((int,scalar,SCALAR_NONE,AttrTrait<>().choice({{SCALAR_NONE,"none"},{SCALAR_TIME,"time"},{SCALAR_VEL,"|vel|"},{SCALAR_SIGNED_ACCEL,"signed |accel|"},{SCALAR_RADIUS,"radius"}}),"Scalars associated with history points (determine line color)"))
		((shared_ptr<ScalarRange>,lineColor,make_shared<ScalarRange>(),,"Color range for coloring the trace line"))
		((Vector2i,modulo,Vector2i(0,0),,"Only add trace to points which with ordinal number ``(i+modulo[1])%modulo[0]==0``."))
		((Vector2r,rRange,Vector2r(0,0),,"If non-zero, only show traces of spheres of which radius falls into this range."))
		((Vector3r,noneColor,Vector3r(.3,.3,.3),AttrTrait<>().rgbColor(),"Color for traces without scalars, when scalars are saved (e.g. for non-spheres when radius is saved"))
		, /*py*/
			;
			_classObj.attr("scalarNone")=(int)Tracer::SCALAR_NONE;
			_classObj.attr("scalarTime")=(int)Tracer::SCALAR_TIME;
			_classObj.attr("scalarVel")=(int)Tracer::SCALAR_VEL;
			_classObj.attr("scalarSignedAccel")=(int)Tracer::SCALAR_SIGNED_ACCEL;
			_classObj.attr("scalarRadius")=(int)Tracer::SCALAR_RADIUS;
	);
};
REGISTER_SERIALIZABLE(Tracer);


