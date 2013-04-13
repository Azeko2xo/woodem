#pragma once
#include<woo/pkg/dem/Facet.hpp>

// #define FLEXFACET_DEBUG_ROT
struct FlexFacet: public Facet{
	bool hasRefConf() const { return node && refRot.size()==3; }
	void pyReset(){ refRot.clear(); }
	void setRefConf(); // use the current configuration as the referential one
	void updateNode(); // update local coordinate system
	void computeNodalDisplacements(); 
	void pyUpdate();
	// transform quaternion *q* so that it is expressed in local frame given by *cs*
	// TODO: this should be doable with quaternions only?!
	// http://stackoverflow.com/questions/15984713/rotation-in-local-frame-expressed-as-quaternion
	Quaternionr quatDiffInNodeCS(const Quaternionr& q){
		//AngleAxisr aa(q);
		//return Quaternionr(AngleAxisr(aa.angle(),cs*aa.axis()));
		//return q.conjugate()*cs;
		return node->ori*q.conjugate();
	}
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(FlexFacet,Facet,"Facet as triangular element, with 2 translational and 2 (or 3) rotational degrees of freedom in each node. Local coordinate system is established using `Best Fit CD Frame <http://www.colorado.edu/engineering/cas/courses.d/NFEM.d/NFEM.AppC.d/NFEM.AppC.pdf>`_ in a non-incremental manner, and in the same way, nodal displacements and rotations are computed.",
		((shared_ptr<Node>,node,make_shared<Node>(),AttrTrait<>().readonly(),"Local coordinate system"))
		((vector<Quaternionr>,refRot,,AttrTrait<>().readonly(),"Rotation applied to nodes to obtain the local coordinate system, computed in the reference configuration. If this array is empty, it means that reference configuration has not yet been evaluated."))
		((Vector6r,refPos,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal coordinates in the local coordinate system, in the reference configuration"))
		((Vector6r,uXy,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal displacements, stored as ux0, uy0, ux1, uy1, ux1, uy2."))
		((Vector6r,phiXy,Vector6r::Zero(),AttrTrait<>().readonly(),"Nodal rotations, only including in-plane rotations (drilling DOF not yet implemented)"))
		#ifdef FLEXFACET_DEBUG_ROT
			((Vector3r,drill,Vector3r::Zero(),AttrTrait<>().readonly(),"Dirilling rotation (debugging only)"))
			((vector<Quaternionr>,currRot,,AttrTrait<>().readonly(),"What would be the current value of refRot (debugging only!)"))
		#endif
		,/*ctor*/
		,/*py*/
			.def("setRefConf",&FlexFacet::setRefConf,"Set the current configuration as the reference one.")
			.def("update",&FlexFacet::pyUpdate,"Update current configuration; create reference configuration if it does not exist.")
			.def("reset",&FlexFacet::pyReset,"Reset reference configuration; this forces using the current config as reference when :obj:`update` is called again.") 
	);
};
REGISTER_SERIALIZABLE(FlexFacet);

#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_FlexFacet: public Gl1_Facet{	
	void go(const shared_ptr<Shape>&, const Vector3r&, bool, const GLViewInfo&);
	void drawLocalDisplacement(const Vector2r& nodePt, const Vector2r& xy, const shared_ptr<ScalarRange>& range, bool split, char arrow, int lineWd, const Real z=NaN);
	RENDERS(FlexFacet);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_FlexFacet,Gl1_Facet,"Renders :ref:`FlexFacet` object; :obj:`Facet` itself is rendered via :obj:`Gl1_Facet`.",
		((bool,node,true,,"Show local frame node"))
		((bool,refConf,true,,"Show reference configuration, rotated to the current local frame"))
		((Vector3r,refColor,Vector3r(0,.5,0),AttrTrait<>().rgbColor(),"COlor for the reference shape"))
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
