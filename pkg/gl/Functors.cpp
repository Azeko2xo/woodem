#include<woo/pkg/gl/Functors.hpp>
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,
		(GlShapeFunctor)(GlShapeDispatcher)
		(GlBoundFunctor)(GlBoundDispatcher)
		(GlCPhysFunctor)(GlCPhysDispatcher)
		(GlNodeFunctor)(GlNodeDispatcher)
		(GlFieldFunctor)(GlFieldDispatcher)
		//(GlCGeomFunctor)(GlCGeomDispatcher)
	);
#endif
