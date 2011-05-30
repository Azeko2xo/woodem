#pragma once

#include<yade/core/Bound.hpp>

/*! Representation of bound by min and max points.

This class is redundant, since it has no data members; don't delete it, though,
as Bound::{min,max} might move here one day.

*/
class Aabb: public Bound{
	public: virtual ~Aabb();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Aabb,Bound,"Axis-aligned bounding box, for use with :yref:`InsertionSortCollider`. (This class is quasi-redundant since min,max are already contained in :yref:`Bound` itself. That might change at some point, though.)",/*attrs*/,/*ctor*/createIndex(););
	REGISTER_CLASS_INDEX(Aabb,Bound);
};
REGISTER_SERIALIZABLE(Aabb);

#ifdef YADE_OPENGL
#include<yade/pkg/common/GLDrawFunctors.hpp>
struct Gl1_Aabb: public GlBoundFunctor{
	virtual void go(const shared_ptr<Bound>&);
	RENDERS(Aabb);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_Aabb,GlBoundFunctor,"Render Axis-aligned bounding box (:yref:`Aabb`).",
		((Vector3r,color,Vector3r(1,1,0),,"Color or rendered wire boxes"))
	);
};
REGISTER_SERIALIZABLE(Gl1_Aabb);
#endif


