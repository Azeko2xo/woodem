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

	enum{COLOR_NONE=0,COLOR_RADIUS,COLOR_VEL,COLOR_MASS,COLOR_DISPLACEMENT,COLOR_ROTATION,COLOR_MAT_ID,COLOR_MAT_PTR};
	enum{GLYPH_KEEP=0,GLYPH_NONE,GLYPH_FORCE,GLYPH_VEL};
	enum{CNODE_NONE=0,CNODE_GLREP=1,CNODE_LINE=2,CNODE_NODE=4,CNODE_POTLINE=8};
	RENDERS(DemField);
	DECLARE_LOGGER;
	Scene* _lastScene; // to detect changes; never acessed as pointer
	WOO_CLASS_BASE_DOC_STATICATTRS_PY(Gl1_DemField,GlFieldFunctor,"Render DEM field.",
		((int,colorBy,COLOR_NONE,AttrTrait<Attr::triggerPostLoad>().choice({{COLOR_NONE,"nothing"},{COLOR_RADIUS,"radius"},{COLOR_VEL,"velocity"},{COLOR_MASS,"mass"},{COLOR_DISPLACEMENT,"displacement"},{COLOR_ROTATION,"rotation"},{COLOR_MAT_ID,"material id"},{COLOR_MAT_PTR,"material pointer"}}),"Color particles by"))
		((int,glyph,0,AttrTrait<Attr::triggerPostLoad>().choice({{GLYPH_KEEP,"keep"},{GLYPH_NONE,"none"},{GLYPH_FORCE,"force"},{GLYPH_VEL,"velocity"}}),"Show glyphs on particles by setting :ref:`GlData` on their nodes."))
		((shared_ptr<ScalarRange>,colorRange,make_shared<ScalarRange>(),AttrTrait<>().readonly(),"Range for particle colors"))
		((shared_ptr<ScalarRange>,glyphRange,make_shared<ScalarRange>(),AttrTrait<>().readonly(),"Range for glyph colors"))
		((bool,doPostLoad,false,AttrTrait<>().hidden(),"Run initialization routine when called next time (set from postLoadStatic)"))
		((bool,colorSpheresOnly,true,,"If :ref:`colorBy` is active, use automatic color for spheres only; for non-spheres, use :ref:`nonSphereColor`"))
		((Vector3r,nonSphereColor,Vector3r(.3,.3,.3),AttrTrait<>().rgbColor(),"Color to use for non-spherical particles when :ref:`colorBy` is not :ref:`colorNone`."))
		((bool,wire,false,,"Render all bodies with wire only"))
		// ((bool,id,false,,"Show particle id's"))
		((bool,bound,false,,"Render particle's :ref:`Bound`"))
		((bool,shape,true,AttrTrait<Attr::triggerPostLoad>(),"Render particle's :ref:`Shape`"))
		//((bool,trace,false,AttrTrait<Attr::triggerPostLoad>(),"Render trace of particle's motion (parameters in the Trace engine, which will be automatically added to the simulation)"))
		//((bool,_hadTrace,false,AttrTrait<>().hidden(),"Whether tracing was enabled previously"))
		((bool,nodes,false,,"Render DEM nodes"))
		((int,cNode,CNODE_NONE,AttrTrait<>().bits({"GlRep","line","node","pot. line"}),"What should be shown for contact nodes"))
		((bool,cPhys,false,,"Render contact's nodes"))
		((Real,glyphRelSz,.2,,"Maximum glyph size relative to scene radius"))
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
			_classObj.attr("colorMatId")=(int)Gl1_DemField::COLOR_MAT_ID;
			_classObj.attr("colorMatPtr")=(int)Gl1_DemField::COLOR_MAT_PTR;
			_classObj.attr("cNodeNone")=(int)Gl1_DemField::CNODE_NONE;
			_classObj.attr("cNodeGlRep")=(int)Gl1_DemField::CNODE_GLREP;
			_classObj.attr("cNodeLine")=(int)Gl1_DemField::CNODE_LINE;
			_classObj.attr("cNodeNode")=(int)Gl1_DemField::CNODE_NODE;
			_classObj.attr("cNodePotLine")=(int)Gl1_DemField::CNODE_POTLINE;
	);
};
REGISTER_SERIALIZABLE(Gl1_DemField);
#endif
