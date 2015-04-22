#pragma once 

#include<woo/core/Field.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/Particle.hpp>

struct TraceVisRep: public NodeVisRep{
	WOO_DECL_LOGGER;
	void addPoint(const Vector3r& p, const Real& scalar);
	void compress(int ratio);
	#ifdef WOO_OPENGL
		void render(const shared_ptr<Node>&,const GLViewInfo*) WOO_CXX11_OVERRIDE;
	#endif
	void setHidden(bool hidden){ if(!hidden)flags&=~FLAG_HIDDEN; else flags|=FLAG_HIDDEN; }
	bool isHidden() const { return flags&FLAG_HIDDEN; }
	void resize(size_t size);
	// set pt and scalar for point indexed i
	// return true if the point is defined, false if not
	bool getPointData(size_t i, Vector3r& pt, Real& scalar) const ;
	// returns the number of point data (by traversal); the number returned is like size(), the value minus one is the last valid index where getPointData returns true
	size_t countPointData() const;

	void consolidate();
	vector<Vector3r> pyPts_get() const;
	vector<Real> pyScalars_get() const;
	enum{FLAG_COMPRESS=1,FLAG_MINDIST=2,FLAG_HIDDEN=4};
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(TraceVisRep,NodeVisRep,"Data with node's position history; created by :obj:`Tracer`.",
		((vector<Vector3r>,pts,,AttrTrait<>().noGui().readonly(),"History points"))
		((vector<Real>,scalars,,AttrTrait<>().noGui().readonly(),"History scalars"))
		((size_t,writeIx,0,,"Index where next data will be written"))
		((short,flags,0,,"Flags for this instance"))
		,/*ctor*/
		,/*py*/
			.def("consolidate",&TraceVisRep::consolidate,"Make :obj:`pts` sequential (normally, the data are stored as circular buffer, with next write position at :obj:`writeIx`, so that they are ordered temporally.")
			.add_property("pts",&TraceVisRep::pyPts_get,"History points (read-only from python, as a copy of internal data is returned).")
			.add_property("scalars",&TraceVisRep::pyScalars_get,"History scalars (read-only from python, as a copy of internal data is returned).")
	);
};
WOO_REGISTER_OBJECT(TraceVisRep);


struct Tracer: public PeriodicEngine{
	bool acceptsField(Field* f) WOO_CXX11_OVERRIDE { return dynamic_cast<DemField*>(f); }
	void resetNodesRep(bool setupEmpty=false, bool includeDead=true);
	// bool notifyDead() WOO_CXX11_OVERRIDE(){ showHideRange(!dead); }
	#ifdef WOO_OPENGL
		void showHideRange(bool show);
	#endif

	virtual void run() WOO_CXX11_OVERRIDE;
	enum{SCALAR_NONE=0,SCALAR_TIME,SCALAR_VEL,SCALAR_ANGVEL,SCALAR_SIGNED_ACCEL,SCALAR_RADIUS,SCALAR_SHAPE_COLOR,SCALAR_KINETIC,SCALAR_ORDINAL};
	WOO_CLASS_BASE_DOC_STATICATTRS_PY(Tracer,PeriodicEngine,"Save trace of node's movement",
		((int,num,50,,"Number of positions to save (when creating new glyph)"))
		((int,compress,2,,"Ratio by which history is compress when all data slots are filled; if 0, cycle and don't compress."))
		((int,compSkip,2,,"Number of leading points to skip during compression; if negative, the value of *compress* is used."))
		((Real,minDist,0,,"Only add new point when last point is at least minDist away, or no point exists at all."))
		//((bool,reset,false,,"Reset traces at the next step"))
		((int,scalar,SCALAR_NONE,AttrTrait<Attr::namedEnum>().namedEnum({{SCALAR_NONE,{"none","-",""}},{SCALAR_TIME,{"time","t"}},{SCALAR_VEL,{"velocity","vel","v"}},{SCALAR_ANGVEL,{"angular velocity","angVel","angvel"}},{SCALAR_SIGNED_ACCEL,{"signed |accel|"}},{SCALAR_RADIUS,{"radius","rad","r"}},{SCALAR_SHAPE_COLOR,{"Shape.color","color"}},{SCALAR_ORDINAL,{"ordinal (+ordinalMod)","ordinal","ord"}},{SCALAR_KINETIC,{"kinetic energy","Ek"}}}),"Scalars associated with history points (determine line color)"))
		((int,vecAxis,-1,AttrTrait<Attr::namedEnum>().namedEnum({{-1,{"norm"}},{0,{"x"}},{1,{"y"}},{2,{"z"}}}).hideIf("self.scalar not in ('velocity','angular velocity')"),"Scalar to use for vector values."))
		((int,ordinalMod,5,AttrTrait<>().hideIf("self.scalar!='ordinal (+ordinalMod)'"),"Modulo value when :obj:`scalar` is :obj:`scalarOrdinal`."))
		((int,lastScalar,SCALAR_NONE,AttrTrait<Attr::hidden>(),"Keep track of last scalar value"))
		((shared_ptr<ScalarRange>,lineColor,make_shared<ScalarRange>(),AttrTrait<>().readonly(),"Color range for coloring the trace line"))
		((Vector2i,modulo,Vector2i(0,0),,"Only add trace to nodes with ordinal number such that ``(i+modulo[1])%modulo[0]==0``."))
		((Vector2r,rRange,Vector2r(0,0),,"If non-zero, only show traces of spheres of which radius falls into this range. (not applicable to clumps); traces of non-spheres are not shown in this case."))
		((Vector3r,noneColor,Vector3r(.3,.3,.3),AttrTrait<>().rgbColor(),"Color for traces without scalars, when scalars are saved (e.g. for non-spheres when radius is saved"))
		((bool,clumps,true,,"Also make traces for clumps (for the central node, not for clumped nodes"))
		((bool,glSmooth,false,,"Render traced lines with GL_LINE_SMOOTH"))
		((int,glWidth,1,AttrTrait<>().range(Vector2i(1,10)),"Width of trace lines in pixels"))
		, /*py*/
			.def("resetNodesRep",&Tracer::resetNodesRep,(py::arg("setupEmpty")=false,py::arg("includeDead")=true),"Reset :obj:`woo.core.Node.rep` on all :obj:`woo.dem.DemField.nodes`. With *setupEmpty*, create new instances of :obj:`TraceVisRep`. With *includeDead*, :obj:`woo.core.Node.rep` on all :obj:`woo.dem.DemField.deadNodes` is also cleared (new are not created, even with *setupEmpty*).")
			;
			_classObj.attr("scalarNone")=(int)Tracer::SCALAR_NONE;
			_classObj.attr("scalarTime")=(int)Tracer::SCALAR_TIME;
			_classObj.attr("scalarVel")=(int)Tracer::SCALAR_VEL;
			_classObj.attr("scalarAngVel")=(int)Tracer::SCALAR_ANGVEL;
			_classObj.attr("scalarSignedAccel")=(int)Tracer::SCALAR_SIGNED_ACCEL;
			_classObj.attr("scalarRadius")=(int)Tracer::SCALAR_RADIUS;
			_classObj.attr("scalarShapeColor")=(int)Tracer::SCALAR_SHAPE_COLOR;
			_classObj.attr("scalarOrdinal")=(int)Tracer::SCALAR_ORDINAL;
			_classObj.attr("scalarKinetic")=(int)Tracer::SCALAR_KINETIC;
	);
};
WOO_REGISTER_OBJECT(Tracer);
