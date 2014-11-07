#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/Sphere.hpp>


struct Truss: public Shape{
	int numNodes() const WOO_CXX11_OVERRIDE { return 2; }
	enum {CAP_A=1,CAP_B=2};
	// defaults
	void updateMassInertia(const Real& density) const WOO_CXX11_OVERRIDE;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(Truss,Shape,"Describes line element (cylinder) with optional caps and with free or constrained rotations at either end.",
		// ((int,rotMask,0,,"Mask determining whether rotation is free or constrained at endnodes; 0=both free, 1=node0 constrained, 2=node1 constrained, 3=both contrained"))
		((int,caps,3,,"Mask determining whether the cylinder is capped at ends; only useful to regularize collisions at the end."))
		((Real,radius,NaN,,"Radius of the cylinder"))
		((Real,l0,NaN,,"Initial (usually equilibrium) length"))
		((Real,axialStress,0,,"Current normal stress (informative only)"))
		((Real,preStress,0,,"Pre-stress (stress at zero strain)"))
		// ((Quaternionr,axisOri,Quaternionr::Identity(),AttrTrait<Attr::readonly>(),"Orientation of the cylinder axis in undeformed state relative to nodes[0] coordinates"))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(Truss,Shape);
};
WOO_REGISTER_OBJECT(Truss);

struct Cg2_Truss_Sphere_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	virtual bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C);
	virtual bool goReverse(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){ throw std::logic_error("ContactLoop should swap interaction arguments, should be Truss+Sphere, but is "+s1->getClassName()+"+"+s2->getClassName()); }
	#define woo_dem_Cg2_Truss_Sphere_L6Geom__CLASS_BASE_DOC \
		Cg2_Truss_Sphere_L6Geom,Cg2_Any_Any_L6Geom__Base,"Incrementally compute :obj:`L6Geom` for contact between :obj:`Truss` and :obj:`Sphere`. Uses attributes of :obj:`Cg2_Sphere_Sphere_L6Geom`."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cg2_Truss_Sphere_L6Geom__CLASS_BASE_DOC);
	FUNCTOR2D(Truss,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Truss,Sphere);
	WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Truss_Sphere_L6Geom);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Truss: public GlShapeFunctor{
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	FUNCTOR1D(Truss);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Truss,GlShapeFunctor,"Render truss particles",
		((int,slices,12,,"Number of slices, controls quality"))
		((int,stacks,6,,"Number of stacks, controls quality"))
		((bool,wire,false,,"Render all shapes with wireframe only"))
		((bool,colorStress,true,,"Set color based on axial stress rather than :obj:`woo.dem.Shape.color`"))
		((Vector2r,stressRange,Vector2r(-1,1),,"Stress range, to set color appropriately"))
	);
};
WOO_REGISTER_OBJECT(Gl1_Truss);
#endif

struct Bo1_Truss_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Truss);
	WOO_CLASS_BASE_DOC(Bo1_Truss_Aabb,BoundFunctor,"Compute :obj:`woo.dem.Aabb` of a Truss particle")
};
WOO_REGISTER_OBJECT(Bo1_Truss_Aabb);

struct In2_Truss_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(Truss,ElastMat);
	WOO_CLASS_BASE_DOC_ATTRS(In2_Truss_ElastMat,IntraFunctor,"Compute elastic response of cylinder determined by 2 nodes.",
		((bool,setL0,true,,"Automatically set equilibrium length of truss, when first encountered."))
		/*attrs*/
	);
};
WOO_REGISTER_OBJECT(In2_Truss_ElastMat);
