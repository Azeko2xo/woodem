#pragma once
#include<woo/lib/object/Object.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

struct Truss: public Shape{
	bool numNodesOk() const { return nodes.size()==2; }
	enum {CAP_A=1,CAP_B=2};
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Truss,Shape,"Describes line element (cylinder) with optional caps and with free or constrained rotations at either end.",
		// ((int,rotMask,0,,"Mask determining whether rotation is free or constrained at endnodes; 0=both free, 1=node0 constrained, 2=node1 constrained, 3=both contrained"))
		((int,caps,3,,"Mask determining whether the cylinder is capped at ends; only useful to regularize collisions at the end."))
		((Real,radius,NaN,,"Radius of the cylinder"))
		((Real,l0,NaN,,"Initial (usually equilibrium) length"))
		((Real,axialStress,0,,"Current normal stress (informative only)"))
		// ((Quaternionr,axisOri,Quaternionr::Identity(),AttrTrait<Attr::readonly>(),"Orientation of the cylinder axis in undeformed state relative to nodes[0] coordinates"))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(Truss,Shape);
};
REGISTER_SERIALIZABLE(Truss);

#ifdef YADE_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Truss: public GlShapeFunctor{
	virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	FUNCTOR1D(Truss);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_Truss,GlShapeFunctor,"Render truss particles",
		((int,slices,12,,"Number of slices, controls quality"))
		((int,stacks,6,,"Number of stacks, controls quality"))
		((bool,wire,false,,"Render all shapes with wireframe only"))
		((bool,colorStress,true,,"Set color based on axial stress rather than :yref:`Shape.color`"))
		((Vector2r,stressRange,Vector2r(-1,1),,"Stress range, to set color appropriately"))
	);
};
#endif

struct Bo1_Truss_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Truss);
	YADE_CLASS_BASE_DOC(Bo1_Truss_Aabb,BoundFunctor,"Compute :yref:`Aabb` of a Truss particle")
};
REGISTER_SERIALIZABLE(Bo1_Truss_Aabb);

struct In2_Truss_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&);
	FUNCTOR2D(Truss,ElastMat);
	YADE_CLASS_BASE_DOC_ATTRS(In2_Truss_ElastMat,IntraFunctor,"Compute elastic response of cylinder determined by 2 nodes.",
		((bool,setL0,true,,"Automatically set equilibrium length of truss, when first encountered."))
		/*attrs*/
	);
};
REGISTER_SERIALIZABLE(In2_Truss_ElastMat);