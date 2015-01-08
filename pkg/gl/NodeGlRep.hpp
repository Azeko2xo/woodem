#ifdef WOO_OPENGL

#pragma once

#include<woo/core/Field.hpp>
#include<woo/core/ScalarRange.hpp>

struct LabelGlRep: public NodeVisRep{
	void render(const shared_ptr<Node>&, const GLViewInfo*);
	#define woo_gl_LabelGlRep__CLASS_BASE_DOC_ATTRS \
		LabelGlRep,NodeVisRep,"Render scalar value at associated node", \
		((string,text,"",,"Text to be rendered at the node's position")) \
		((Vector3r,color,Vector3r(1,1,1),AttrTrait<>().rgbColor(),"Color for rendering the text")) \
		((bool,center,false,,"Whether the text should be centered around the node")) 
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_gl_LabelGlRep__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(LabelGlRep);

struct ScalarGlRep: public NodeVisRep{
	void render(const shared_ptr<Node>&, const GLViewInfo*);
	#define woo_gl_ScalarGlRep__CLASS_BASE_DOC_ATTRS \
		ScalarGlRep,NodeVisRep,"Render scalar value at associated node", \
		((Real,val,0,,"Value to be rendered")) \
		((int,how,0,,"Different ways to render given value; 0=number, 1=colored point, 2=colored sphere")) \
		((int,prec,5,,"Precision for rendering numbers")) \
		((Real,relSz,.05,,"Size of rendered spheres (if selected), relative to scene radius")) \
		((shared_ptr<ScalarRange>,range,,,"Extrema values for the scalar, to determine colors."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_gl_ScalarGlRep__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(ScalarGlRep);

struct VectorGlRep: public NodeVisRep{
	void render(const shared_ptr<Node>&, const GLViewInfo*);
	#define woo_gl_VectorGlRep__CLASS_BASE_DOC_ATTRS \
		VectorGlRep,NodeVisRep,"Render vector value at associated node, as an arrow", \
		((Vector3r,val,Vector3r::Zero(),,"Value to be rendered")) \
		((Real,relSz,.2,,"Size of maximum-length arrows, relative to scene radius")) \
		((Real,scaleExp,1,,"Exponent for scaling arrow size as ``vector_norm^scaleExp``. NaN disables scaling (all arrows the same size).")) \
		((shared_ptr<ScalarRange>,range,,,"Extrema values for vector norm, to determine colors."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_gl_VectorGlRep__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(VectorGlRep);

struct ActReactGlRep: public VectorGlRep{
	void render(const shared_ptr<Node>&, const GLViewInfo*);
	void renderDoubleArrow(const Vector3r& pos, const Vector3r& arr, bool posStart, const Vector3r& offset, const Vector3r& color);
	#define woo_gl_ActReactGlRep__CLASS_BASE_DOC_ATTRS \
		ActReactGlRep,VectorGlRep,"Render action and reaction vectors as opposing arrows, with offset and optionally separate normal/shear components. The value is always given in node-local coordinates!", \
		((int,comp,3,,"Which components of the force to show 0: x-only, 1: yz-only, 2: both as separate arrows, 3: both as one arrow.")) \
		((Vector2i,comp_range,Vector2i(0,3),AttrTrait<>().noGui(),"Range for *comp*")) \
		((Real,relOff,.01,,"Offset from the node in the sense of local x-axis, relative to scene radius")) \
		((shared_ptr<ScalarRange>,shearRange,,,"Optional range for shear foces; if not defined range (for normal force) is used instead."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_gl_ActReactGlRep__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(ActReactGlRep);

struct TensorGlRep: public NodeVisRep{
	void render(const shared_ptr<Node>&, const GLViewInfo*);
	void postLoad(TensorGlRep&,void*);
	#define woo_gl_TensorGlRep__CLASS_BASE_DOC_ATTRS \
		TensorGlRep,NodeVisRep,"Render tensor (given as 3x3 matrix) as its principal components.", \
		((Matrix3r,val,Matrix3r::Zero(),AttrTrait<Attr::triggerPostLoad>(),"Value to be rendered.")) \
		((Matrix3r,eigVec,Matrix3r::Zero(),AttrTrait<Attr::noSave|Attr::readonly>(),"eigenvectors as columns, updated in postLoad.")) \
		((Vector3r,eigVal,Vector3r::Zero(),AttrTrait<Attr::noSave|Attr::readonly>(),"eigenvalues of corresponding eigenvectors, updated in postLoad.")) \
		((Vector3r,skew,Vector3r::Zero(),AttrTrait<Attr::noSave>(),"skew (asymmetric) components of the tensor")) \
		((Real,relSz,.1,,"Size of maximum-length arrows, relative to scene radius")) \
		((Real,skewRelSz,-1,,"Size of maximum-length skew curved arrows; if negative, use relSz instead.")) \
		((Real,scaleExp,1,,"Exponent for scaling arrow sizem kuje wutg VectorGlRep. NaN disables scaling, making all arrows the same size.")) \
		((shared_ptr<ScalarRange>,range,,,"Extrema values for symmetric components.")) \
		((shared_ptr<ScalarRange>,skewRange,,,"Extrema values for skew components"))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_gl_TensorGlRep__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(TensorGlRep);

struct CylGlRep: public NodeVisRep{
	void render(const shared_ptr<Node>&, const GLViewInfo*);
	//void postLoad(CylGlRep&,void*);
	#define woo_gl_CylGlRep__CLASS_BASE_DOC_ATTRS \
		CylGlRep,NodeVisRep,"Render cylinder aligned with local x-axis, with color and radius given by val (and optionally val2).", \
		((Real,rad,NaN,,"Scalar determining radius; 1 if NaN")) \
		((Real,col,NaN,,"Scalar determining color; *rad* is used if NaN.")) \
		((Vector2r,xx,Vector2r(0,0),,"End positions on the local x-axis")) \
		((Real,relSz,.05,,"Maximum cylinder radius, relative to scene radius")) \
		((shared_ptr<ScalarRange>,rangeRad,,,"Range for rad (only used if rad is not NaN)")) \
		((shared_ptr<ScalarRange>,rangeCol,,,"Range for col (or for rad, if *col* is NaN)"))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_gl_CylGlRep__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(CylGlRep);


#endif
