#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/IntraForce.hpp>

struct Facet: public Shape {
	bool numNodesOk() const { return nodes.size()==3; }
	Vector3r getNormal() const;
	// return velocity which is linearly interpolated between velocities of facet nodes, and also angular velocity at that point
	std::tuple<Vector3r,Vector3r> interpolatePtLinAngVel(const Vector3r& x) const;
	std::tuple<Vector3r,Vector3r,Vector3r> getOuterVectors() const;
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Facet,Shape,"Facet (triangle in 3d) particle.",
		((Vector3r,fakeVel,Vector3r::Zero(),,"Fake velocity when computing contact, in global coordinates (for modeling moving surface modeled using static triangulation); only in-plane velocity is meaningful, but this is not enforced."))
		/*attrs*/
		,/*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(Facet,Shape);
};
REGISTER_SERIALIZABLE(Facet);

struct Bo1_Facet_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Facet);
	YADE_CLASS_BASE_DOC(Bo1_Facet_Aabb,BoundFunctor,"Creates/updates an :yref:`Aabb` of a :yref:`Facet`.");
};
REGISTER_SERIALIZABLE(Bo1_Facet_Aabb);

#ifdef YADE_OPENGL
#include<yade/pkg/gl/Functors.hpp>
struct Gl1_Facet: public GlShapeFunctor{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	RENDERS(Facet);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_Facet,GlShapeFunctor,"Renders :yref:`Facet` object",
		((bool,wire,false,,"Only show wireframe."))
		/*attrs*/
	);
};
REGISTER_SERIALIZABLE(Gl1_Facet);
#endif
