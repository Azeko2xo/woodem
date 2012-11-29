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
	void doNodes(const vector<shared_ptr<Node>>& nodeContainer);
	void doContactNodes();
	void doCPhys();
	static void postLoadStatic(){
		colorRange=colorRanges[colorBy];
		glyphRange=glyphRanges[glyph];
		doPostLoad=true;
	}
	static void setSceneRange(Scene* scene, const shared_ptr<ScalarRange>& prev, const shared_ptr<ScalarRange>& curr);
	static void initAllRanges();

	void postLoad2();

	enum{COLOR_SHAPE=0,COLOR_RADIUS,COLOR_VEL,COLOR_ANGVEL,COLOR_MASS,COLOR_DISPLACEMENT,COLOR_ROTATION,COLOR_REFPOS,COLOR_MAT_ID,COLOR_MATSTATE,/*last*/COLOR_SENTINEL};
	enum{GLYPH_KEEP=0,GLYPH_NONE,GLYPH_FORCE,GLYPH_VEL,/*last*/GLYPH_SENTINEL};
	enum{CNODE_NONE=0,CNODE_GLREP=1,CNODE_LINE=2,CNODE_NODE=4,CNODE_POTLINE=8};
	RENDERS(DemField);
	DECLARE_LOGGER;
	Scene* _lastScene; // to detect changes; never acessed as pointer
	WOO_CLASS_BASE_DOC_STATICATTRS_CTOR_PY(Gl1_DemField,GlFieldFunctor,"Render DEM field.",
		((bool,shape,true,AttrTrait<Attr::triggerPostLoad>().startGroup("Shape"),"Render particle's :ref:`Shape`"))
		((int,colorBy,COLOR_SHAPE,AttrTrait<Attr::triggerPostLoad>().choice({
			{COLOR_SHAPE,"Shape.color"},
			{COLOR_RADIUS,"radius"},
			{COLOR_VEL,"velocity"},
			{COLOR_ANGVEL,"angular velocity"},
			{COLOR_MASS,"mass"},
			{COLOR_DISPLACEMENT,"ref. displacement"},
			{COLOR_ROTATION,"ref. rotation"},
			{COLOR_REFPOS,"refpos coordinate"},
			{COLOR_MAT_ID,"material id"},
			{COLOR_MATSTATE,"Particle.matState"}
		}).buttons({"Reference now","self.updateRefPos=True","use current positions and orientations as reference for showing displacement/rotation"},/*showBefore*/false),"Color particles by"))
		((int,vecAxis,-1,AttrTrait<>().choice({{-1,"norm"},{0,"x"},{1,"y"},{2,"z"}}),"Axis for colorRefPosCoord"))
		((bool,wire,false,,"Render all shapes with wire only"))
		((bool,colorSpheresOnly,true,,"If :ref:`colorBy` is active, use automatic color for spheres only; for non-spheres, use :ref:`fallbackColor`"))
		((Vector3r,fallbackColor,Vector3r(.3,.3,.3),AttrTrait<>().rgbColor(),"Color to use for non-spherical particles when :ref:`colorBy` is not :ref:`colorShape`."))
		((int,prevColorBy,COLOR_SHAPE,AttrTrait<>().hidden(),"previous value of colorBy (to know which colorrange to remove from the scene)"))
		((shared_ptr<ScalarRange>,colorRange,,AttrTrait<>().readonly(),"Range for particle colors"))
		((vector<shared_ptr<ScalarRange>>,colorRanges,,AttrTrait<>().readonly().noGui(),"List of color ranges"))
		((bool,bound,false,,"Render particle's :ref:`Bound`"))
		((uint,mask,0,,"Only shapes/bounds of particles with this mask will be displayed; if 0, all particles are shown"))

		((bool,nodes,false,AttrTrait<>().startGroup("Nodes"),"Render DEM nodes"))
		((int,glyph,GLYPH_KEEP,AttrTrait<Attr::triggerPostLoad>().choice({{GLYPH_KEEP,"keep"},{GLYPH_NONE,"none"},{GLYPH_FORCE,"force"},{GLYPH_VEL,"velocity"}}),"Show glyphs on particles by setting :ref:`GlData` on their nodes."))
		((int,prevGlyph,GLYPH_KEEP,AttrTrait<>().hidden(),"previous value of glyph"))
		((shared_ptr<ScalarRange>,glyphRange,,AttrTrait<>().readonly(),"Range for glyph colors"))
		((Real,glyphRelSz,.1,,"Maximum glyph size relative to scene radius"))
		((vector<shared_ptr<ScalarRange>>,glyphRanges,,AttrTrait<>().readonly().noGui(),"List of glyph ranges"))

		// ((bool,id,false,,"Show particle id's"))
		((int,cNode,CNODE_NONE,AttrTrait<>().bits({"GlRep","line","node","pot. line"}).startGroup("Contact nodes"),"What should be shown for contact nodes"))
		((bool,cPhys,false,,"Render contact's nodes"))

		((bool,doPostLoad,false,AttrTrait<>().hidden(),"Run initialization routine when called next time (set from postLoadStatic)"))
		((bool,updateRefPos,false,,"Update reference positions of all nodes in the next step"))
		((int,guiEvery,100,,"Process GUI events once every *guiEvery* objects are painted, to keep the ui responsive. Set to 0 to make rendering blocking."))
		, /*ctor*/
			initAllRanges();
		, /*py*/
			;
			_classObj.attr("glyphKeep")=(int)Gl1_DemField::GLYPH_KEEP;
			_classObj.attr("glyphNone")=(int)Gl1_DemField::GLYPH_NONE;
			_classObj.attr("glyphForce")=(int)Gl1_DemField::GLYPH_FORCE;
			_classObj.attr("glyphVel")=(int)Gl1_DemField::GLYPH_VEL;
			_classObj.attr("colorShape")=(int)Gl1_DemField::COLOR_SHAPE;
			_classObj.attr("colorRadius")=(int)Gl1_DemField::COLOR_RADIUS;
			_classObj.attr("colorVel")=(int)Gl1_DemField::COLOR_VEL;
			_classObj.attr("colorAngVel")=(int)Gl1_DemField::COLOR_ANGVEL;
			_classObj.attr("colorDisplacement")=(int)Gl1_DemField::COLOR_DISPLACEMENT;
			_classObj.attr("colorRotation")=(int)Gl1_DemField::COLOR_ROTATION;
			_classObj.attr("colorRefPos")=(int)Gl1_DemField::COLOR_REFPOS;
			_classObj.attr("colorMatId")=(int)Gl1_DemField::COLOR_MAT_ID;
			_classObj.attr("colorMatState")=(int)Gl1_DemField::COLOR_MATSTATE;
			_classObj.attr("cNodeNone")=(int)Gl1_DemField::CNODE_NONE;
			_classObj.attr("cNodeGlRep")=(int)Gl1_DemField::CNODE_GLREP;
			_classObj.attr("cNodeLine")=(int)Gl1_DemField::CNODE_LINE;
			_classObj.attr("cNodeNode")=(int)Gl1_DemField::CNODE_NODE;
			_classObj.attr("cNodePotLine")=(int)Gl1_DemField::CNODE_POTLINE;
	);
};
REGISTER_SERIALIZABLE(Gl1_DemField);
#endif
