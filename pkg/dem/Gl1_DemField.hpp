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
		((bool,wire,false,,"Render all bodies with wire only"))
		// ((bool,id,false,,"Show particle id's"))
		((bool,bound,false,,"Render particle's :yref:`Bound`"))
		((bool,shape,true,,"Render particle's :yref:`Shape`"))
		((bool,nodes,false,,"Render DEM nodes"))
		((int,cNodes,-1,,"Render contact's nodes (-1=nothing, 0=rep only, 2=line between particles, 2=nodes, 3=both"))
		((Vector2i,cNodes_range,Vector2i(-1,3),Attr::noGui,"Range for cNodes"))
		((bool,potWire,false,,"Render potential contacts as line between particles"))
		((bool,cPhys,false,,"Render contact's nodes"))
		 /*
		((int,wd,1,,"Local axes line width in pixels"))
		((Vector2i,wd_range,Vector2i(0,5),Attr::noGui,"Range for width"))
		((Real,len,.05,,"Relative local axes line length in pixels, relative to scene radius; if non-positive, only points are drawn"))
		((Vector2r,len_range,Vector2r(0.,.1),Attr::noGui,"Range for len"))
		*/
		((uint,mask,0,,"Only shapes/bounds of particles with this mask will be displayed; if 0, all particles are shown"))
	);
};
#endif

