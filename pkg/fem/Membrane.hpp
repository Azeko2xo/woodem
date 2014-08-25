#pragma once
#include<woo/pkg/dem/Facet.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

// #define MEMBRANE_DEBUG_ROT
struct Membrane: public Facet{
	bool hasRefConf() const { return node && refRot.size()==3; }
	bool hasBending(){ return KKdkt.size()>0; }
	void pyReset(){ refRot.clear(); }
	void setRefConf(); // use the current configuration as the referential one
	void ensureStiffnessMatrices(const Real& young, const Real& nu, const Real& thickness, bool bending, const Real& bendThickness);
	void updateNode(); // update local coordinate system
	void computeNodalDisplacements(); 
	// called by functors to initialize (if necessary) and update
	void stepUpdate();
	// called from DynDt for updating internal stiffness for given node
	void addIntraStiffnesses(const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const;

	REGISTER_CLASS_INDEX(Membrane,Facet);
	DECLARE_LOGGER;
	#ifdef MEMBRANE_DEBUG_ROT
		#define woo_dem_Membrane__ATTRS__MEMBRANE_DEBUG_ROT \
			((Vector3r,drill,Vector3r::Zero(),AttrTrait<>().readonly(),"Dirilling rotation (debugging only)")) \
			((vector<Quaternionr>,currRot,,AttrTrait<>().readonly(),"What would be the current value of refRot (debugging only!)")) \
			((VectorXr,uDkt,,,"DKT displacement vector (saved for debugging only)"))
		#else
			#define woo_dem_Membrane__ATTRS__MEMBRANE_DEBUG_ROT
		#endif
	#define woo_dem_Membrane__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		Membrane,Facet,"Facet as triangular element, with 2 translational and 2 rotational degrees of freedom in each node.\n\n* The CST element is implemented using the formulation found in Felippa's `Introduction to FEM, chapter 15 <http://www.colorado.edu/engineering/cas/courses.d/IFEM.d/IFEM.Ch15.d/IFEM.Ch15.pdf>`_ (the $\\mat{B}$ matrix is given in (eq. 15.17)). The displacement vector is accessible as :obj:`uXy`, the stiffness matrix as :obj:`KKcst`.\n\n* The DKT element is implemented following the original paper by Batoz, Bathe and Ho `A Study of three-node triangular plate bending elmenets <http://web.mit.edu/kjb/www/Publications_Prior_to_1998/A_Study_of_Three-Node_Triangular_Plate_Bending_Elements.pdf>`_, section 3.1. DKT displacement vector (with $z$-displacements condensed away) is stored in :obj:`phiXy`, the stiffness matrix in :obj:`KKdkt`.\n\n* Local coordinate system is established using `Best Fit CD Frame <http://www.colorado.edu/engineering/cas/courses.d/NFEM.d/NFEM.AppC.d/NFEM.AppC.pdf>`_ in a non-incremental manner (with a slight improvement), and in the same way, nodal displacements and rotations are computed.\n\nSince positions of nodes determine the element's plane, the $z$ degrees of freedom have zero displacements and are condensed away from the :obj:`KKdkt` matrix (force reaction, however, is nonzero in that direction, so the matrix is not square).\n\nDrilling rotations can be computed, but are ignored; this can lead to instability in some cases -- wobbly rotation of nodes which does not decrease due to non-viscous damping.\n\nThe element is assumed to be under plane-stress conditions.\n\nMass of the element is lumped in to nodes, but this is not automatized in any way; it is your responsibility to assign proper values of :obj:`DemData.mass` and :obj:`DemData.inertia`.", \
		((shared_ptr<Node>,node,make_shared<Node>(),AttrTrait<>().readonly(),"Local coordinate system")) \
		((vector<Quaternionr>,refRot,,AttrTrait<>().readonly(),"Rotation applied to nodes to obtain the local coordinate system, computed in the reference configuration. If this array is empty, it means that reference configuration has not yet been evaluated.")) \
		((Vector6r,refPos,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal coordinates in the local coordinate system, in the reference configuration")) \
		((Vector6r,uXy,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal displacements, stored as ux0, uy0, ux1, uy1, ux1, uy2.")) \
		((Real,surfLoad,0.,AttrTrait<>().pressureUnit(),"Normal load applied to this facet (positive in the direction of the local normal); this value is multiplied by the current facet's area and equally distributed to nodes.")) \
		((Vector6r,phiXy,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal rotations, only including in-plane rotations (drilling DOF not yet implemented)")) \
		((MatrixXr,KKcst,,,"Stiffness matrix of the element (assembled from the reference configuration when needed for the first time)")) \
		((MatrixXr,KKdkt,,,"Bending stiffness matrix of the element (assembled from the reference configuration when needed for the first time).")) \
		woo_dem_Membrane__ATTRS__MEMBRANE_DEBUG_ROT \
		,/*ctor*/ createIndex(); \
		,/*py*/ \
			.def("setRefConf",&Membrane::setRefConf,"Set the current configuration as the reference one.") \
			.def("update",&Membrane::stepUpdate,"Update current configuration; create reference configuration if it does not exist.") \
			.def("reset",&Membrane::pyReset,"Reset reference configuration; this forces using the current config as reference when :obj:`update` is called again.")

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_Membrane__CLASS_BASE_DOC_ATTRS_CTOR_PY);
};
WOO_REGISTER_OBJECT(Membrane);

struct In2_Membrane_ElastMat: public IntraFunctor{
	void addIntraStiffnesses(const shared_ptr<Particle>&, const shared_ptr<Node>&, Vector3r& ktrans, Vector3r& krot) const WOO_CXX11_OVERRIDE;
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&, const bool skipContacts) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(Membrane,ElastMat);
	DECLARE_LOGGER;
	#define woo_dem_In2_Membrane_ElastMat__CLASS_BASE_DOC_ATTRS \
		In2_Membrane_ElastMat,IntraFunctor,"Apply contact forces and compute internal response of a :obj:`Membrane` (so far only the plane stress element has been implemented).", \
		((bool,contacts,true,,"Apply contact forces to facet's nodes (FIXME: very simply distributed in thirds now)")) \
		((Real,nu,.25,,"Poisson's ratio used for assembling the $E$ matrix (Young's modulus is taken from :obj:`ElastMat`). Will be moved to the material class at some point.")) \
		((Real,thickness,NaN,,"Thickness for CST stiffness computation; if NaN, try to use the double of :obj:`Facet.halfThick`.")) \
		((Real,bendThickness,NaN,,"Thickness for CST stiffness computation; if NaN, use :obj:`thickness`.")) \
		((bool,bending,false,,"Consider also bending stiffness of elements (DKT)")) \
		((bool,applyBary,false,,"Distribute force according to barycentric coordinate of the contact point; this is done normally with :obj:`bending` enabled, this forces the same also for particles without bending."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_In2_Membrane_ElastMat__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(In2_Membrane_ElastMat);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Membrane: public Gl1_Facet{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	void drawLocalDisplacement(const Vector2r& nodePt, const Vector2r& xy, const shared_ptr<ScalarRange>& range, bool split, char arrow, int lineWd, const Real z=NaN);
	RENDERS(Membrane);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Membrane,Gl1_Facet,"Renders :obj:`Membrane` object; :obj:`Facet` itself is rendered via :obj:`Gl1_Facet`.",
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
WOO_REGISTER_OBJECT(Gl1_Membrane);
#endif
