#include<yade/pkg/common/GLDrawFunctors.hpp>
#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,
		(GlBoundFunctor)(GlShapeFunctor)(GlCGeomFunctor)(GlCPhysFunctor)(GlStateFunctor)(GlFieldFunctor)(GlNodeFunctor)
		(GlBoundDispatcher)(GlShapeDispatcher)(GlCGeomDispatcher)(GlCPhysDispatcher)(GlStateDispatcher)(GlFieldDispatcher)(GlNodeDispatcher)
	);
#endif
