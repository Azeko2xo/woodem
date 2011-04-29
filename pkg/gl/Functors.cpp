#include<yade/pkg/gl/Functors.hpp>
#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,
		(GlShapeFunctor)(GlShapeDispatcher)
		(GlBoundFunctor)(GlBoundDispatcher)
		(GlNodeFunctor)(GlNodeDispatcher)
		//(GlBoundFunctor)(GlIGeomFunctor)(GlIPhysFunctor)(GlStateFunctor)(GlFieldFunctor)(GlNodeFunctor)
		//(GlBoundDispatcher)(GlIGeomDispatcher)(GlIPhysDispatcher)(GlStateDispatcher)(GlFieldDispatcher)(GlNodeDispatcher)
	);
#endif
