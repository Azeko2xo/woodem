#pragma once
#ifdef WOO_OPENGL
//#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/gl/Functors.hpp>


struct Gl1_DemField: public GlFieldFunctor{
	virtual void go(const shared_ptr<Field>&, GLViewInfo*);
	GLViewInfo* viewInfo;
	shared_ptr<DemField> dem; // used by do* methods
	void doShape();
	void doBound();
	void doNodes();
	void doContactNodes();
	void doCPhys();
	static void postLoadStatic(){ doPostLoad=true; }
	void postLoad(Gl1_DemField&);

	enum{COLOR_NONE=0,COLOR_RADIUS,COLOR_VEL,COLOR_MASS,COLOR_DISPLACEMENT,COLOR_ROTATION};
	enum{GLYPH_KEEP=0,GLYPH_NONE,GLYPH_FORCE,GLYPH_VEL};
	RENDERS(DemField);
	DECLARE_LOGGER;
	Scene* _lastScene; // to detect changes; never acessed as pointer
	WOO_CLASS_BASE_DOC_STATICATTRS_PY(Gl1_DemField,GlFieldFunctor,"Render DEM field.",
		((bool,doPostLoad,false,AttrTrait<>().hidden(),"Run initialization routine when called next time (set from postLoadStatic)"))
		((bool,wire,false,,"Render all bodies with wire only"))
		// ((bool,id,false,,"Show particle id's"))
		((bool,bound,false,,"Render particle's :ref:`Bound`"))
		((bool,shape,true,,"Render particle's :ref:`Shape`"))
		((bool,nodes,false,,"Render DEM nodes"))
		((int,cNodes,-1,AttrTrait<>().choice({{-1,"nothing"},{0,"Node.rep"},{1,"Node.rep + line"},{2,"Node.rep + Node"},{3,"Node.rep + Node + line"}}),"Render contact's nodes"))
		((Vector2i,cNodes_range,Vector2i(-1,3),AttrTrait<>().noGui(),"Range for cNodes"))
		((bool,potWire,false,,"Render potential contacts as line between particles"))
		((bool,cPhys,false,,"Render contact's nodes"))
		((int,glyph,0,AttrTrait<Attr::triggerPostLoad>().choice({{GLYPH_KEEP,"keep"},{GLYPH_NONE,"none"},{GLYPH_FORCE,"force"},{GLYPH_VEL,"velocity"}}),"Show glyphs on particles by setting :ref:`GlData` on their nodes."))
		((Real,glyphRelSz,.2,,"Maximum glyph size relative to scene radius"))
		((shared_ptr<ScalarRange>,glyphRange,make_shared<ScalarRange>(),AttrTrait<>().readonly(),"Range for glyph colors"))
		((int,colorBy,COLOR_NONE,AttrTrait<Attr::triggerPostLoad>().choice({{COLOR_NONE,"nothing"},{COLOR_RADIUS,"radius"},{COLOR_VEL,"velocity"},{COLOR_MASS,"mass"},{COLOR_DISPLACEMENT,"displacement"},{COLOR_ROTATION,"rotation"}}),"Color particles by"))
		((shared_ptr<ScalarRange>,colorRange,make_shared<ScalarRange>(),AttrTrait<>().readonly(),"Range for particle colors"))
		((bool,updateRefPos,false,,"Update reference positions of all nodes in the next step"))
		 /*
		((int,wd,1,,"Local axes line width in pixels"))
		((Vector2i,wd_range,Vector2i(0,5),AttrTrait<>().noGui(),"Range for width"))
		((Real,len,.05,,"Relative local axes line length in pixels, relative to scene radius; if non-positive, only points are drawn"))
		((Vector2r,len_range,Vector2r(0.,.1),AttrTrait<>().noGui(),"Range for len"))
		*/
		((uint,mask,0,,"Only shapes/bounds of particles with this mask will be displayed; if 0, all particles are shown"))
		, /*py*/
			;
			_classObj.attr("glyphKeep")=(int)Gl1_DemField::GLYPH_KEEP;
			_classObj.attr("glyphNone")=(int)Gl1_DemField::GLYPH_NONE;
			_classObj.attr("glyphForce")=(int)Gl1_DemField::GLYPH_FORCE;
			_classObj.attr("glyphVel")=(int)Gl1_DemField::GLYPH_VEL;
			_classObj.attr("colorNone")=(int)Gl1_DemField::COLOR_NONE;
			_classObj.attr("colorRadius")=(int)Gl1_DemField::COLOR_RADIUS;
			_classObj.attr("colorVel")=(int)Gl1_DemField::COLOR_VEL;
			_classObj.attr("colorDisplacement")=(int)Gl1_DemField::COLOR_DISPLACEMENT;
			_classObj.attr("colorRotation")=(int)Gl1_DemField::COLOR_ROTATION;
	);
};
REGISTER_SERIALIZABLE(Gl1_DemField);
#endif
