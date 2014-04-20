#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/Sphere.hpp>


struct Facet: public Shape {
	bool numNodesOk() const { return nodes.size()==3; }
	void selfTest(const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
	Vector3r getNormal() const;
	Vector3r getCentroid() const;
	#ifdef WOO_OPENGL
		Vector3r getGlNormal() const;
		Vector3r getGlVertex(int i) const;
		Vector3r getGlCentroid() const;
	#endif
	DECLARE_LOGGER;
	// return velocity which is linearly interpolated between velocities of facet nodes, and also angular velocity at that point
	// takes fakeVel in account, with the NaN special case as documented
	std::tuple<Vector3r,Vector3r> interpolatePtLinAngVel(const Vector3r& x) const;
	std::tuple<Vector3r,Vector3r,Vector3r> getOuterVectors() const;
	// closest point on the facet
	Vector3r getNearestPt(const Vector3r& pt) const;
	vector<Vector3r> outerEdgeNormals() const;
	Real getArea() const;
	Real getPerimeterSq() const; 
	// generic routine: return nearest point on triangle closes to *pt*, given triangle vertices and its normal
	static Vector3r getNearestTrianglePt(const Vector3r& pt, const Vector3r& A, const Vector3r& B, const Vector3r& C, const Vector3r& normal);
	// defaults
	void updateMassInertia(const Real& density) const WOO_CXX11_OVERRIDE;
	//
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Facet,Shape,"Facet (triangle in 3d) particle.",
		((Vector3r,fakeVel,Vector3r::Zero(),,"Fake velocity when computing contact, in global coordinates (for modeling moving surface modeled using static triangulation); only in-plane velocity is meaningful, but this is not enforced.\n\n.. note:: If the x-component is NaN, the meaning is special: :obj:`fakeVel` is taken as zero vector and, in addition, local in-plane facet's linear velocity at the contact is taken as zero (rather than linearly interpolated between velocity of nodes).\n"))
		((Real,halfThick,0.,,"Geometric thickness (added in all directions)"))
		/*attrs*/
		,/*ctor*/ createIndex();
		,/*py*/
			.def("getNormal",&Facet::getNormal,"Return normal vector of the facet")
			.def("outerEdgeNormals",&Facet::outerEdgeNormals,"Return outer edge normal vectors")
			.def("area",&Facet::getArea,"Return surface area of the facet")
	);
	REGISTER_CLASS_INDEX(Facet,Shape);
};
WOO_REGISTER_OBJECT(Facet);

struct Bo1_Facet_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Facet);
	WOO_CLASS_BASE_DOC(Bo1_Facet_Aabb,BoundFunctor,"Creates/updates an :obj:`Aabb` of a :obj:`Facet`.");
};
WOO_REGISTER_OBJECT(Bo1_Facet_Aabb);

struct Cg2_Facet_Sphere_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Facet+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	void setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE { C->minDist00Sq=-1; }
	#define woo_dem_Cg2_Facet_Sphere_L6Geom__CLASS_BASE_DOC \
		Cg2_Facet_Sphere_L6Geom,Cg2_Any_Any_L6Geom__Base,"Incrementally compute :obj:`L6Geom` for contact between :obj:`Facet` and :obj:`Sphere`. Uses attributes of :obj:`Cg2_Sphere_Sphere_L6Geom`."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Sphere_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Facet,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Facet,Sphere);
	DECLARE_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Facet_Sphere_L6Geom);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Facet: public GlShapeFunctor{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	void drawEdges(const Facet& f, const Vector3r& facetNormal, const Vector3r& shift, bool wire);
	void glVertex(const Facet& f, int i);
	RENDERS(Facet);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Facet,GlShapeFunctor,"Renders :obj:`Facet` object",
		((bool,wire,false,AttrTrait<>().buttons({"All facets solid","import woo\nfor p in woo.master.scene.dem.par:\n\tif isinstance(p.shape,woo.dem.Facet): p.shape.wire=False\n","","All facets wire","import woo\nfor p in woo.master.scene.dem.par:\n\tif isinstance(p.shape,woo.dem.Facet): p.shape.wire=True\n",""}),"Only show wireframe."))
		((int,slices,8,AttrTrait<>().range(Vector2i(-1,16)),"Number of half-cylinder subdivision for rounded edges with halfThick>=0 (for whole circle); if smaller than 4, rounded edges are not drawn; if negative, only mid-plane is drawn."))
		((Real,fastDrawLim,1e-3,,"If performing fast draw (during camera manipulation) and the facet's perimeter is smaller than *fastDrawLim* times scene radius, skip rendering of that facet."))
		#if 0
			((Vector2r,fuzz,Vector2r(1e-3,17),,"When drawing wire, move it by (fuzz[0]/fuzz[1])x(some dim)x(addr%fuzz[1]) along the normal. That way, facets are randomly (based on object address) and consistently displaced, avoiding the z-fighting problem leading to flickering."))
		#endif
		((int,wd,1,AttrTrait<>().range(Vector2i(1,20)),"Line width when drawing with wireframe (only applies to the triangle, not to rounded corners)"))
	);
};
WOO_REGISTER_OBJECT(Gl1_Facet);
#endif
