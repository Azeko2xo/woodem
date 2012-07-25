#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/IntraForce.hpp>

struct Facet: public Shape {
	bool numNodesOk() const { return nodes.size()==3; }
	Vector3r getNormal() const;
	// return velocity which is linearly interpolated between velocities of facet nodes, and also angular velocity at that point
	std::tuple<Vector3r,Vector3r> interpolatePtLinAngVel(const Vector3r& x) const;
	std::tuple<Vector3r,Vector3r,Vector3r> getOuterVectors() const;
	vector<Vector3r> outerEdgeNormals() const;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Facet,Shape,"Facet (triangle in 3d) particle.",
		((Vector3r,fakeVel,Vector3r::Zero(),,"Fake velocity when computing contact, in global coordinates (for modeling moving surface modeled using static triangulation); only in-plane velocity is meaningful, but this is not enforced."))
		((Real,halfThick,0.,,"Geometric thickness (added in all directions)"))
		/*attrs*/
		,/*ctor*/ createIndex();
		,/*py*/
			.def("getNormal",&Facet::getNormal,"Return normal vector of the facet")
			.def("outerEdgeNormals",&Facet::outerEdgeNormals,"Return outer edge normal vectors")
	);
	REGISTER_CLASS_INDEX(Facet,Shape);
};
REGISTER_SERIALIZABLE(Facet);

struct Bo1_Facet_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Facet);
	WOO_CLASS_BASE_DOC(Bo1_Facet_Aabb,BoundFunctor,"Creates/updates an :ref:`Aabb` of a :ref:`Facet`.");
};
REGISTER_SERIALIZABLE(Bo1_Facet_Aabb);

#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Facet: public GlShapeFunctor{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	void drawEdges(const Facet& f, const Vector3r& facetNormal, bool wire);
	RENDERS(Facet);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Facet,GlShapeFunctor,"Renders :ref:`Facet` object",
		((bool,wire,false,,"Only show wireframe."))
		((int,slices,8,,"Number of half-cylinder subdivision for rounded edges with halfThick>=0 (for whole circle); if smaller than 4, rounded edges are not drawn."))
		/*attrs*/
	);
};
REGISTER_SERIALIZABLE(Gl1_Facet);
#endif
