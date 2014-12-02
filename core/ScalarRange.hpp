#pragma once

#include<woo/lib/base/Types.hpp>
#include<woo/lib/object/Object.hpp>
#include<woo/lib/pyutil/converters.hpp>

#ifdef WOO_OPENGL
	struct QglMovableObject;
#endif


struct ScalarRange: public Object{
	#ifdef WOO_OPENGL
		shared_ptr<QglMovableObject> movablePtr;
	#endif
	void reset();
	Vector3r color(Real v);
	Real maxAbs(){ if(!isOk()) abort(); throw std::runtime_error("ScalarRange::maxAbs(): uninitialized object, call with value to adjust range.");  return max(abs(mnmx[0]),abs(mnmx[1])); }
	Real maxAbs(Real v){ adjust(v); return max(abs(mnmx[0]),abs(mnmx[1])); }
	bool isOk(){ return(mnmx[0]<mnmx[1]); }
	// return value on the range, given normalized value
	Real normInv(Real norm);
	Real norm(Real v);
	void adjust(const Real& v);
	// called only when mnmx is manipulated
	enum{RANGE_LOG=1,RANGE_REVERSED=2,RANGE_SYMMETRIC=4,RANGE_AUTO_ADJUST=8,RANGE_HIDDEN=16};
	void postLoad(const ScalarRange&,void*);

	bool isAutoAdjust() const { return flags&RANGE_AUTO_ADJUST; }
	void setAutoAdjust(bool aa) { if(!aa) flags&=~RANGE_AUTO_ADJUST; else flags|=RANGE_AUTO_ADJUST; }
	bool isSymmetric() const { return flags&RANGE_SYMMETRIC; }
	void setSymmetric(bool s) { if(!s) flags&=~RANGE_SYMMETRIC; else flags|=RANGE_SYMMETRIC; }
	bool isReversed() const { return flags&RANGE_REVERSED; }
	void setReversed(bool s) { if(!s) flags&=~RANGE_REVERSED; else flags|=RANGE_REVERSED; }
	bool isHidden() const { return flags&RANGE_HIDDEN; }
	void setHidden(bool h) { if(!h) flags&=~RANGE_HIDDEN; else flags|=RANGE_HIDDEN; }
	bool isLog() const { return flags&RANGE_LOG; }
	void setLog(bool l) { if(!l) flags&=~RANGE_LOG; else flags|=RANGE_LOG; }
	void cacheLogs(); 

	#define woo_core_ScalarRange__CLASS_BASE_DOC_ATTRS_PY \
		ScalarRange,Object,"Store and share range of scalar values", \
		((Vector2r,mnmx,Vector2r(std::numeric_limits<Real>::infinity(),-std::numeric_limits<Real>::infinity()),AttrTrait<Attr::triggerPostLoad>().buttons({"Reset","self.reset()","Re-initialize range"}),"Packed minimum and maximum values; adjusting from python sets :obj:`autoAdjust` to false automatically.")) \
		((Vector2r,logMnmx,,AttrTrait<Attr::noSave|Attr::hidden>(),"Logs of mnmx values, to avoid computing logarithms all the time; computed via cacheLogs.")) \
		((int,flags,(RANGE_AUTO_ADJUST),AttrTrait<>().bits({"log","reversed","symmetric","autoAdjust","hidden"}),"Flags for this range: autoAdjust, symmetric, reversed, hidden, log.")) \
		((Vector2i,dispPos,Vector2i(-1000,-1000),AttrTrait<>().noGui(),"Where is this range displayed on the OpenGL canvas; initially out of range, will be reset automatically.")) \
		((Real,length,200,AttrTrait<>().noGui(),"Length on the display; if negative, it is fractional relative to view width/height")) \
		((bool,landscape,false,AttrTrait<>().noGui(),"Make the range display with landscape orientation")) \
		((std::string,label,,,"Short name of this range.")) \
		((int,cmap,-1,AttrTrait<>().colormapChoice(),"Colormap index to be used; -1 means to use the default colormap (see *woo.master.cmaps*, *woo.master.cmap*)")) \
		, /* py */ \
			.def("norm",&ScalarRange::norm,"Return value of the argument normalized to 0..1 range; the value is not clamped to 0..1 however: if autoAdjust is false, it can fall outside.") \
			.def("reset",&ScalarRange::reset); \
			woo::converters_cxxVector_pyList_2way<shared_ptr<ScalarRange>>();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_core_ScalarRange__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(ScalarRange);


