#include<yade/pkg/common/GLDrawFunctors.hpp>
#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,
		(GlBoundFunctor)(GlShapeFunctor)(GlIGeomFunctor)(GlIPhysFunctor)(GlStateFunctor)(GlFieldFunctor)(GlNodeFunctor)
		(GlBoundDispatcher)(GlShapeDispatcher)(GlIGeomDispatcher)(GlIPhysDispatcher)(GlStateDispatcher)(GlFieldDispatcher)(GlNodeDispatcher)
	);
#endif
