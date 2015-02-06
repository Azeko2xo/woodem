#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

struct Tetra: public Shape {
	int numNodes() const WOO_CXX11_OVERRIDE { return 4; }
	void selfTest(const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
	void canonicalizeVertexOrder();
	// Vector3r getNormal() const;
	Vector3r getCentroid() const;
	Real getVolume() const;
	void setFromRaw(const Vector3r& center, const Real& radius, const vector<Real>& raw) WOO_CXX11_OVERRIDE;
	void asRaw(Vector3r& center, Real& radius, vector<Real>& raw) const WOO_CXX11_OVERRIDE;
	#ifdef WOO_OPENGL
		Vector3r getGlNormal(int f) const;
		Vector3r getGlVertex(int i) const;
		Vector3r getGlCentroid() const;
	#endif
	WOO_DECL_LOGGER;
	// return velocity which is linearly interpolated between velocities of tetra nodes, and also angular velocity at that point
	// takes fakeVel in account, with the NaN special case as documented
	// std::tuple<Vector3r,Vector3r> interpolatePtLinAngVel(const Vector3r& x) const;
	// std::tuple<Vector3r,Vector3r,Vector3r> getOuterVectors() const;
	// closest point on the facet
	//Vector3r getNearestPt(const Vector3r& pt) const;
	//	vector<Vector3r> outerEdgeNormals() const;
	// Real getPerimeterSq() const; 
	// generic routine: return nearest point on triangle closes to *pt*, given triangle vertices and its normal
	// static Vector3r getNearestTrianglePt(const Vector3r& pt, const Vector3r& A, const Vector3r& B, const Vector3r& C, const Vector3r& normal);

	void lumpMassInertia(const shared_ptr<Node>&, Real density, Real& mass, Matrix3r& I, bool& rotateOk) WOO_CXX11_OVERRIDE;

	#define woo_fem_Tetra__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		Tetra,Shape,"Tetra (triangle in 3d) particle.", \
		/*((Real,halfThick,0.,,"Geometric thickness (added in all directions)"))*/ \
		,/*ctor*/ createIndex(); canonicalizeVertexOrder(); \
		,/*py*/ \
			/*.def("getNormal",&Tetra::getNormal,"Return normal vector of the facet") */ \
			.def("getCentroid",&Tetra::getCentroid,"Return centroid of the tetrahedron") \
			.def("getVolume",&Tetra::getVolume,"Return volume of the tetrahedron.") \
			.def("canonicalizeVertexOrder",&Tetra::canonicalizeVertexOrder,"Order vertices so that signed volume is positive.") \
			/*.def("outerEdgeNormals",&Tetra::outerEdgeNormals,"Return outer edge normal vectors")*/ \
			/*.def("area",&Tetra::getArea,"Return surface area of the facet") */
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_fem_Tetra__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	REGISTER_CLASS_INDEX(Tetra,Shape);
};
WOO_REGISTER_OBJECT(Tetra);

struct Tet4: public Tetra{
	bool hasRefConf() const { return node && refPos.cols()==4 && refPos.rows()==3; }
	void pyReset(){ node.reset(); refPos=MatrixXr(); }
	void setRefConf();
	void stepUpdate();
	void computeCorotatedFrame();
	void computeNodalDisplacements();

	void ensureStiffnessMatrix(Real young, Real nu);
	void addIntraStiffnesses(const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const;
	Matrix3r getStressTensor() const;

	#define woo_fem_Tet4__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		Tet4,Tetra,"4-node linear interpolation tetrahedron element with best-fit co-rotated coordinates.", \
		((shared_ptr<Node>,node,,AttrTrait<>().readonly(),"Local coordinate system")) \
		((MatrixXr,refPos,,AttrTrait<>().readonly(),"Reference nodal positions in local coordinates")) \
		((VectorXr,uXyz,,AttrTrait<>().readonly(),"Nodal displacements in local coordinates")) \
		((MatrixXr,KK,,AttrTrait<>().readonly().noGui(),"Stiffness matrix")) \
		((MatrixXr,EB,,AttrTrait<>().readonly().noGui(),":math:`E B` matrix, used to compute stresses from displacements.")) \
		,/*ctor*/ createIndex();\
		,/*py*/.def("setRefConf",&Tet4::setRefConf,"Set the current configuration as the reference one") \
			.def("ensureStiffnessMatrix",&Tet4::ensureStiffnessMatrix,(py::arg("young"),py::arg("nu")),"Ensure that stiffness matrix is initialized; internally also sets reference configuration. The *young* parameter should match :obj:`woo.dem.ElastMat.young` attached to the particle.") \
			.def("update",&Tet4::stepUpdate,"Update current configuration; creates reference configuration if not existing") \
			.def("reset",&Tet4::pyReset) \
			.def("getStressTensor",&Tet4::getStressTensor)

	REGISTER_CLASS_INDEX(Tet4,Shape);
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_fem_Tet4__CLASS_BASE_DOC_ATTRS_CTOR_PY);
};
WOO_REGISTER_OBJECT(Tet4);

struct Bo1_Tetra_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Tetra);
	#define woo_fem_Bo1_Tetra_Aabb__CLASS_BASE_DOC Bo1_Tetra_Aabb,BoundFunctor,"Creates/updates an :obj:`Aabb` of a :obj:`Tetra`."
	WOO_DECL__CLASS_BASE_DOC(woo_fem_Bo1_Tetra_Aabb__CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(Bo1_Tetra_Aabb);


struct In2_Tet4_ElastMat: public IntraFunctor{
	void addIntraStiffnesses(const shared_ptr<Particle>&, const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const WOO_CXX11_OVERRIDE;
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(Tet4,ElastMat);
	WOO_DECL_LOGGER;
	#define woo_dem_In2_Tet4_ElastMat__CLASS_BASE_DOC_ATTRS \
		In2_Tet4_ElastMat,IntraFunctor,"Apply contact forces and compute internal response of a :obj:`Tet4`.", \
		((bool,contacts,false,AttrTrait<>().readonly(),"Apply contact forces to :obj:`Tetra` nodes (not yet implemented)")) \
		((Real,nu,.25,,"Poisson's ratio used for assembling the $E$ matrix (Young's modulus is taken from :obj:`ElastMat`). Will be moved to the material class at some point."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_In2_Tet4_ElastMat__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(In2_Tet4_ElastMat);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Tetra: public GlShapeFunctor{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	//void drawEdges(const Tetra& f, const Vector3r& facetNormal, const Vector3r shifts[3], bool wire);
	//void glVertex(const Tetra& f, int i);
	RENDERS(Tetra);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Tetra,GlShapeFunctor,"Renders :obj:`Tetra` object",
		((bool,wire,false,AttrTrait<>().buttons({"All tetrahedra solid","import woo\nfor p in woo.master.scene.dem.par:\n\tif isinstance(p.shape,woo.fem.Tetra): p.shape.wire=False\n","","All tetrahedra wire","import woo\nfor p in woo.master.scene.dem.par:\n\tif isinstance(p.shape,woo.fem.Tetra): p.shape.wire=True\n",""}),"Only show wireframe."))
		((int,wd,1,AttrTrait<>().range(Vector2i(1,20)),"Line width when drawing with wireframe (only applies to the triangle, not to rounded corners)"))
		((Real,fastDrawLim,1e-3,,"If performing fast draw (during camera manipulation) and the distance of centroid to the first node is smaller than *fastDrawLim* times scene radius, skip rendering of that tetra."))
	);
};
WOO_REGISTER_OBJECT(Gl1_Tetra);

struct Gl1_Tet4: public Gl1_Tetra{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	RENDERS(Tet4);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Tet4,Gl1_Tetra,"Renders :obj:`Tet4` object; :obj:`Tetra` itself is rendered via :obj:`Gl1_Tetra`.",
		((bool,node,false,,"Show local frame node"))
		((bool,rep,true,,"Show GlRep of the frame node (without showing the node itself)"))
		((bool,refConf,false,,"Show reference configuration, rotated to the current local frame"))
		((Vector3r,refColor,Vector3r(0,.5,0),AttrTrait<>().rgbColor(),"Color for the reference shape"))
		((int,refWd,1,,"Line width for the reference shape"))
		((int,uWd,2,,"Width of displacement lines"))
	);
};
WOO_REGISTER_OBJECT(Gl1_Tet4);
		

#endif
