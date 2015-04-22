#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/Sphere.hpp>


struct Rod: public Shape{
	int numNodes() const WOO_CXX11_OVERRIDE { return 2; }
	void lumpMassInertia(const shared_ptr<Node>&, Real density, Real& mass, Matrix3r& I, bool& rotateOk) WOO_CXX11_OVERRIDE;
	#define woo_dem_Rod__CLASS_BASE_DOC_ATTRS_CTOR \
		Rod,Shape,"Line element without internal forces, wyth circular cross-section and hemi-spherical caps at both ends. Geometrically the same :obj:`Capsule`, but with 2 nodes.", \
		((Real,radius,NaN,,"Radius of the rod.")) \
		,/*ctor*/createIndex();
	
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_Rod__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(Rod,Shape);
};
WOO_REGISTER_OBJECT(Rod);

struct Truss: public Rod{
	// defaults
	#define woo_dem_Truss__CLASS_BASE_DOC_ATTRS_CTOR \
		Truss,Rod,"Describes line element (cylinder) with optional caps and with free or constrained rotations at either end.", \
		((Real,l0,NaN,,"Initial (usually equilibrium) length")) \
		((Real,axialStress,0,,"Current normal stress (informative only)")) \
		((Real,preStress,0,,"Pre-stress (stress at zero strain)")) \
		, /*ctor*/ createIndex();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_Truss__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(Truss,Rod);
};
WOO_REGISTER_OBJECT(Truss);

struct Cg2_Rod_Sphere_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Rod_Sphere_L6Geom__CLASS_BASE_DOC \
		Cg2_Rod_Sphere_L6Geom,Cg2_Any_Any_L6Geom__Base,"Incrementally compute :obj:`L6Geom` for contact between :obj:`Rod` and :obj:`Sphere`. Uses attributes of :obj:`Cg2_Sphere_Sphere_L6Geom`."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Rod_Sphere_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Rod,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Rod,Sphere);
	WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Rod_Sphere_L6Geom);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Rod: public GlShapeFunctor{
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&) WOO_CXX11_OVERRIDE;
	FUNCTOR1D(Rod);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Rod,GlShapeFunctor,"Render truss particles",
		((int,slices,12,,"Number of slices, controls quality"))
		((int,stacks,6,,"Number of stacks, controls quality"))
		((bool,wire,false,,"Render all shapes with wireframe only"))
		((bool,colorStress,true,,"Set color based on axial stress rather than :obj:`woo.dem.Shape.color`"))
		((Vector2r,stressRange,Vector2r(-1,1),,"Stress range, to set color appropriately"))
	);
};
WOO_REGISTER_OBJECT(Gl1_Rod);
#endif

struct Bo1_Rod_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&) WOO_CXX11_OVERRIDE;
	FUNCTOR1D(Rod);
	WOO_CLASS_BASE_DOC(Bo1_Rod_Aabb,BoundFunctor,"Compute :obj:`woo.dem.Aabb` of a :obj:`Rod` particle")
};
WOO_REGISTER_OBJECT(Bo1_Rod_Aabb);

struct In2_Truss_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(Truss,ElastMat);
	void addIntraStiffnesses(const shared_ptr<Particle>&, const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const WOO_CXX11_OVERRIDE;
	WOO_CLASS_BASE_DOC_ATTRS(In2_Truss_ElastMat,IntraFunctor,"Compute elastic response of cylinder determined by 2 nodes.",
		((bool,setL0,true,,"Automatically set equilibrium length of truss, when first encountered."))
		/*attrs*/
	);
};
WOO_REGISTER_OBJECT(In2_Truss_ElastMat);
