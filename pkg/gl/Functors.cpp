#include<yade/pkg/gl/Functors.hpp>
#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,
		(GlShapeFunctor)(GlShapeDispatcher)
		(GlBoundFunctor)(GlBoundDispatcher)
		(GlCPhysFunctor)(GlCPhysDispatcher)
		(GlNodeFunctor)(GlNodeDispatcher)
		//(GlBoundFunctor)(GlCGeomFunctor)(GlStateFunctor)(GlFieldFunctor)(GlNodeFunctor)
		//(GlBoundDispatcher)(GlCGeomDispatcher)(GlStateDispatcher)(GlFieldDispatcher)(GlNodeDispatcher)
	);
#endif
