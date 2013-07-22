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
	static void postLoadStatic(void* attr){
		colorRange=colorRanges[colorBy];
		glyphRange=glyphRanges[glyph];
		doPostLoad=true;
		if(attr==&matStateIx) colorRanges[COLOR_MATSTATE]->reset();
	}
	static void setSceneRange(Scene* scene, const shared_ptr<ScalarRange>& prev, const shared_ptr<ScalarRange>& curr);
	static void setOurSceneRanges(Scene* scene, const vector<shared_ptr<ScalarRange>>& ours, const list<shared_ptr<ScalarRange>>& used);
	static void initAllRanges();

	Vector3r getNodeVel(const shared_ptr<Node>& n) const;
	Vector3r getParticleVel(const shared_ptr<Particle>& n) const;
	Vector3r getNodeAngVel(const shared_ptr<Node>& n) const;

	void postLoad2();

	enum{COLOR_SOLID=0,COLOR_SHAPE,COLOR_RADIUS,COLOR_VEL,COLOR_ANGVEL,COLOR_MASS,COLOR_DISPLACEMENT,COLOR_ROTATION,COLOR_REFPOS,COLOR_MAT_ID,COLOR_MATSTATE,COLOR_SIG_N,COLOR_SIG_T,COLOR_INVISIBLE,/*last*/COLOR_SENTINEL};
	enum{GLYPH_KEEP=0,GLYPH_NONE,GLYPH_FORCE,GLYPH_TORQUE,GLYPH_VEL,/*last*/GLYPH_SENTINEL};
	enum{CNODE_NONE=0,CNODE_GLREP=1,CNODE_LINE=2,CNODE_NODE=4,CNODE_POTLINE=8};
	enum{SHAPE_NONE=0,SHAPE_ALL,SHAPE_SPHERES,SHAPE_NONSPHERES,SHAPE_MASK};
	RENDERS(DemField);
	DECLARE_LOGGER;
	Scene* _lastScene; // to detect changes; never acessed as pointer

	// those are used twice, stuff them into macro
	#define GL1_DEMFIELD_COLORBY_CHOICES \
			{COLOR_SOLID,"solidColor"}, \
			{COLOR_SHAPE,"Shape.color"}, \
			{COLOR_RADIUS,"radius"}, \
			{COLOR_VEL,"velocity"}, \
			{COLOR_ANGVEL,"angular velocity"}, \
			{COLOR_MASS,"mass"}, \
			{COLOR_DISPLACEMENT,"ref. displacement"}, \
			{COLOR_ROTATION,"ref. rotation"}, \
			{COLOR_REFPOS,"refpos coordinate"}, \
			{COLOR_MAT_ID,"material id"}, \
			{COLOR_MATSTATE,"Particle.matState"}, \
			{COLOR_SIG_N,"normal stress"}, \
			{COLOR_SIG_T,"shear stress"}, \
			{COLOR_INVISIBLE,"invisible"}

	WOO_CLASS_BASE_DOC_STATICATTRS_CTOR_PY(Gl1_DemField,GlFieldFunctor,"Render DEM field.",
		((int,shape,SHAPE_ALL,AttrTrait<Attr::triggerPostLoad>().choice({{SHAPE_NONE,"none"},{SHAPE_ALL,"all"},{SHAPE_SPHERES,"spheres"},{SHAPE_NONSPHERES,"non-spheres"},{SHAPE_MASK,"mask"}}).startGroup("Shape"),"Render only particles matching selected filter."))
		((uint,mask,0,,"Only shapes/bounds of particles with this group will be shown; 0 matches all particles."))
		((bool,shape2,true,AttrTrait<Attr::triggerPostLoad>(),"Render also particles not matching :ref:`shape` (using :ref:`colorBy2`)"))
		((bool,wire,false,,"Render all shapes with wire only"))

		((int,colorBy,COLOR_SHAPE,AttrTrait<Attr::triggerPostLoad>().choice({ GL1_DEMFIELD_COLORBY_CHOICES }).buttons({"Reference now","self.updateRefPos=True","use current positions and orientations as reference for showing displacement/rotation"},/*showBefore*/false),"Color particles by"))
		((int,vecAxis,-1,AttrTrait<>().choice({{-1,"norm"},{0,"x"},{1,"y"},{2,"z"}}).hideIf("self.colorBy in (self.colorShape, self.colorRadius, self.colorMatId, self.colorMatState)"),"Axis for colorRefPosCoord"))
		((int,matStateIx,0,AttrTrait<Attr::triggerPostLoad>().hideIf("self.colorBy!=self.colorMatState and self.colorBy2!=self.colorMatState"),"Index for getting :obj:`MatState` scalars."))
		((shared_ptr<ScalarRange>,colorRange,,AttrTrait<>().readonly().hideIf("self.colorBy in (self.colorSolid,self.colorInvisible)"),"Range for particle colors (:ref:`colorBy`)"))

		((int,colorBy2,COLOR_SOLID,AttrTrait<Attr::triggerPostLoad>().choice({ GL1_DEMFIELD_COLORBY_CHOICES }).hideIf("not self.shape2"),"Color for particles with :ref:`shape2`."))
		((shared_ptr<ScalarRange>,colorRange2,,AttrTrait<>().readonly().hideIf("not self.shape2 or self.colorBy2 in (self.colorSolid,self.colorInvisible)"),"Range for particle colors (:ref:`colorBy`)"))
		((Vector3r,solidColor,Vector3r(.3,.3,.3),AttrTrait<>().rgbColor(),"Solid color for particles."))

		((vector<shared_ptr<ScalarRange>>,colorRanges,,AttrTrait<>().readonly().noGui(),"List of color ranges"))

		((bool,bound,false,,"Render particle's :ref:`Bound`"))
		((bool,periodic,false,AttrTrait<>().noGui(),"Automatically shows whether the scene is periodic (to use in hideIf of :obj:`fluct`"))
		((bool,fluct,false,AttrTrait<>().hideIf("not self.periodic or (self.colorBy not in (self.colorVel,self.colorAngVel) and self.glyph not in (self.glyphVel,))"),"With periodic boundaries, show only fluctuation components of velocity."))

		((bool,nodes,false,AttrTrait<>().startGroup("Nodes"),"Render DEM nodes"))
		((int,glyph,GLYPH_KEEP,AttrTrait<Attr::triggerPostLoad>().choice({{GLYPH_KEEP,"keep"},{GLYPH_NONE,"none"},{GLYPH_FORCE,"force"},{GLYPH_TORQUE,"torque"},{GLYPH_VEL,"velocity"}}),"Show glyphs on particles by setting :ref:`GlData` on their nodes."))
		((shared_ptr<ScalarRange>,glyphRange,,AttrTrait<>().readonly(),"Range for glyph colors"))
		((Real,glyphRelSz,.1,,"Maximum glyph size relative to scene radius"))
		((bool,deadNodes,true,,"Show :obj:`DemField.deadNodes <woo.dem.DemField.deadNodes>`."))
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
			_classObj.attr("glyphTorque")=(int)Gl1_DemField::GLYPH_TORQUE;
			_classObj.attr("glyphVel")=(int)Gl1_DemField::GLYPH_VEL;
			_classObj.attr("shapeAll")=(int)Gl1_DemField::SHAPE_ALL;
			_classObj.attr("shapeNone")=(int)Gl1_DemField::SHAPE_NONE;
			_classObj.attr("shapeSpheres")=(int)Gl1_DemField::SHAPE_SPHERES;
			_classObj.attr("shapeNonSpheres")=(int)Gl1_DemField::SHAPE_NONSPHERES;
			_classObj.attr("shapeMask")=(int)Gl1_DemField::SHAPE_MASK;
			_classObj.attr("colorSolid")=(int)Gl1_DemField::COLOR_SOLID;
			_classObj.attr("colorShape")=(int)Gl1_DemField::COLOR_SHAPE;
			_classObj.attr("colorRadius")=(int)Gl1_DemField::COLOR_RADIUS;
			_classObj.attr("colorVel")=(int)Gl1_DemField::COLOR_VEL;
			_classObj.attr("colorAngVel")=(int)Gl1_DemField::COLOR_ANGVEL;
			_classObj.attr("colorDisplacement")=(int)Gl1_DemField::COLOR_DISPLACEMENT;
			_classObj.attr("colorRotation")=(int)Gl1_DemField::COLOR_ROTATION;
			_classObj.attr("colorRefPos")=(int)Gl1_DemField::COLOR_REFPOS;
			_classObj.attr("colorMatId")=(int)Gl1_DemField::COLOR_MAT_ID;
			_classObj.attr("colorMatState")=(int)Gl1_DemField::COLOR_MATSTATE;
			_classObj.attr("colorSigN")=(int)Gl1_DemField::COLOR_SIG_N;
			_classObj.attr("colorSigT")=(int)Gl1_DemField::COLOR_SIG_T;
			_classObj.attr("colorInvisible")=(int)Gl1_DemField::COLOR_INVISIBLE;
			_classObj.attr("cNodeNone")=(int)Gl1_DemField::CNODE_NONE;
			_classObj.attr("cNodeGlRep")=(int)Gl1_DemField::CNODE_GLREP;
			_classObj.attr("cNodeLine")=(int)Gl1_DemField::CNODE_LINE;
			_classObj.attr("cNodeNode")=(int)Gl1_DemField::CNODE_NODE;
			_classObj.attr("cNodePotLine")=(int)Gl1_DemField::CNODE_POTLINE;
	);
};
WOO_REGISTER_OBJECT(Gl1_DemField);
#endif
