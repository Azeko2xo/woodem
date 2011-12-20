#include<yade/pkg/gl/Functors.hpp>
#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,
		(GlShapeFunctor)(GlShapeDispatcher)
		(GlBoundFunctor)(GlBoundDispatcher)
		(GlCPhysFunctor)(GlCPhysDispatcher)
		(GlNodeFunctor)(GlNodeDispatcher)
		(GlFieldFunctor)(GlFieldDispatcher)
		//(GlCGeomFunctor)(GlCGeomDispatcher)
	);
#endif
