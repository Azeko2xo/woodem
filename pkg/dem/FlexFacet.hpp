#pragma once
#include<woo/pkg/dem/Facet.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

// #define FLEXFACET_DEBUG_ROT
struct FlexFacet: public Facet{
	bool hasRefConf() const { return node && refRot.size()==3; }
	bool hasBending(){ return KKdkt.size()>0; }
	void pyReset(){ refRot.clear(); }
	void setRefConf(); // use the current configuration as the referential one
	void ensureStiffnessMatrices(const Real& young, const Real& nu, const Real& thickness, bool bending, const Real& bendThickness);
	void updateNode(); // update local coordinate system
	void computeNodalDisplacements(); 
	// called by functors to initialize (if necessary) and update
	void stepUpdate();
	REGISTER_CLASS_INDEX(FlexFacet,Facet);
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(FlexFacet,Facet,"Facet as triangular element, with 2 translational and 2 (or 3) rotational degrees of freedom in each node. Local coordinate system is established using `Best Fit CD Frame <http://www.colorado.edu/engineering/cas/courses.d/NFEM.d/NFEM.AppC.d/NFEM.AppC.pdf>`_ in a non-incremental manner, and in the same way, nodal displacements and rotations are computed.",
		((shared_ptr<Node>,node,make_shared<Node>(),AttrTrait<>().readonly(),"Local coordinate system"))
		((vector<Quaternionr>,refRot,,AttrTrait<>().readonly(),"Rotation applied to nodes to obtain the local coordinate system, computed in the reference configuration. If this array is empty, it means that reference configuration has not yet been evaluated."))
		((Vector6r,refPos,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal coordinates in the local coordinate system, in the reference configuration"))
		((Vector6r,uXy,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal displacements, stored as ux0, uy0, ux1, uy1, ux1, uy2."))
		((Real,surfLoad,0.,AttrTrait<>().pressureUnit(),"Normal load applied to this facet (positive in the direction of the local normal); this value is multiplied by the current facet's area and equally distributed to nodes."))
		((Vector6r,phiXy,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal rotations, only including in-plane rotations (drilling DOF not yet implemented)"))
		#ifdef FLEXFACET_DEBUG_ROT
			((Vector3r,drill,Vector3r::Zero(),AttrTrait<>().readonly(),"Dirilling rotation (debugging only)"))
			((vector<Quaternionr>,currRot,,AttrTrait<>().readonly(),"What would be the current value of refRot (debugging only!)"))
			((VectorXr,uDkt,,,"DKT displacement vector (saved for debugging only)"))
		#endif
		((MatrixXr,KKcst,,,"Stiffness matrix of the element (assembled from the reference configuration when needed for the first time)"))
		((MatrixXr,KKdkt,,,"Bending stiffness matrix of the element (assembled from the reference configuration when needed for the first time)."))
		,/*ctor*/ createIndex();
		,/*py*/
			.def("setRefConf",&FlexFacet::setRefConf,"Set the current configuration as the reference one.")
			.def("update",&FlexFacet::stepUpdate,"Update current configuration; create reference configuration if it does not exist.")
			.def("reset",&FlexFacet::pyReset,"Reset reference configuration; this forces using the current config as reference when :obj:`update` is called again.") 
	);
};
REGISTER_SERIALIZABLE(FlexFacet);

struct In2_FlexFacet_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&);
	FUNCTOR2D(FlexFacet,ElastMat);
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS(In2_FlexFacet_ElastMat,IntraFunctor,"Apply contact forces and compute internal response of a :obj:`FlexFacet` (so far only the plane stress element has been implemented).",
		((bool,contacts,true,,"Apply contact forces to facet's nodes (FIXME: very simply distributed in thirds now)"))
		((Real,nu,.25,,"Poisson's ratio used for assembling the $E$ matrix (Young's modulus is taken from :obj:`ElastMat`). Will be moved to the material class at some point."))
		((Real,thickness,NaN,,"Thickness for CST stiffness computation; if NaN, try to use the double of :obj:`Facet.halfThick`."))
		((Real,bendThickness,NaN,,"Thickness for CST stiffness computation; if NaN, use :obj:`thickness`."))
		((bool,bending,false,,"Consider also bending stiffness of elements (DKT)"))
	);
};
REGISTER_SERIALIZABLE(In2_FlexFacet_ElastMat);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_FlexFacet: public Gl1_Facet{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	void drawLocalDisplacement(const Vector2r& nodePt, const Vector2r& xy, const shared_ptr<ScalarRange>& range, bool split, char arrow, int lineWd, const Real z=NaN);
	RENDERS(FlexFacet);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_FlexFacet,Gl1_Facet,"Renders :ref:`FlexFacet` object; :obj:`Facet` itself is rendered via :obj:`Gl1_Facet`.",
		((bool,node,false,,"Show local frame node"))
		((bool,refConf,true,,"Show reference configuration, rotated to the current local frame"))
		((Vector3r,refColor,Vector3r(0,.5,0),AttrTrait<>().rgbColor(),"COlor for the reference shape"))
		((int,refWd,1,,"Line width for the reference shape"))
		((Real,uScale,1.,,"Scale of displacement lines (zero to disable)"))
		((int,uWd,2,,"Width of displacement lines"))
		((bool,uSplit,false,,"Show x and y displacement components separately"))
		((Real,relPhi,.2,,"Length of unit rotation (one radian), relative to scene radius (zero to disable)"))
		((int,phiWd,2,,"Width of rotation lines"))
		((bool,phiSplit,true,,"Show x and y displacement components separately"))
		((bool,arrows,false,,"Show displacements and rotations as arrows rather than lines"))
		((shared_ptr<ScalarRange>,uRange,make_shared<ScalarRange>(),,"Range for displacements (colors only)"))
		((shared_ptr<ScalarRange>,phiRange,make_shared<ScalarRange>(),,"Range for rotations (colors only)"))
	);
};
REGISTER_SERIALIZABLE(Gl1_FlexFacet);
#endif
