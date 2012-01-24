#ifdef YADE_OPENGL
//#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/gl/Functors.hpp>


struct Gl1_DemField: public GlFieldFunctor{
	virtual void go(const shared_ptr<Field>&, GLViewInfo*);
	Renderer* rrr; // will be removed later, once the parameters are local
	GLViewInfo* viewInfo;
	shared_ptr<DemField> dem; // used by do* methods
	void doShape();
	void doBound();
	void doNodes();
	void doContactNodes();
	void doCPhys();
	RENDERS(DemField);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_DemField,GlFieldFunctor,"Render DEM field.",
		 /*
		((int,wd,1,,"Local axes line width in pixels"))
		((Vector2i,wd_range,Vector2i(0,5),Attr::noGui,"Range for width"))
		((Real,len,.05,,"Relative local axes line length in pixels, relative to scene radius; if non-positive, only points are drawn"))
		((Vector2r,len_range,Vector2r(0.,.1),Attr::noGui,"Range for len"))
		*/
		((unsigned int,mask,0,,"Only shapes/bounds of particles with this mask will be displayed; if 0, all particles are shown"))
	);
};
#endif

