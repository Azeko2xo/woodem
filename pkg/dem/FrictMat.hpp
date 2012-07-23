#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/core/MatchMaker.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>
// class MatchMaker;

class ElastMat: public Material{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(ElastMat,Material,"Elastic material with contact friction. See also :ref:`ElastMat`.",
		((Real,young,1e9,AttrTrait<>().stiffnessUnit(),"Young's modulus"))
		// ((Real,poisson,.2,,"Poisson's ratio; this value should be only used in internal force computation, not for contacts."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(ElastMat,Material);
};
REGISTER_SERIALIZABLE(ElastMat);

class FrictMat: public ElastMat{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(FrictMat,ElastMat,"Elastic material with contact friction. See also :ref:`ElastMat`.",
		((Real,tanPhi,.5,,"Tangent of internal friction angle."))
		((Real,ktDivKn,.2,,"Ratio of tangent and shear modulus on contact."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(FrictMat,ElastMat);
};
REGISTER_SERIALIZABLE(FrictMat);

class FrictPhys: public CPhys{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(FrictPhys,CPhys,"Physical parameters of contact with sliding",
		((Real,tanPhi,NaN,,"Tangent of friction angle"))
		((Real,kn,NaN,,"Normal stiffness"))
		((Real,kt,NaN,,"Tangent stiffness"))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(FrictPhys,CPhys);
};
REGISTER_SERIALIZABLE(FrictPhys);

struct Cp2_FrictMat_FrictPhys: public CPhysFunctor{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	void updateFrictPhys(FrictMat& m1, FrictMat& m2, FrictPhys& ph, const shared_ptr<Contact>& C);
	FUNCTOR2D(FrictMat,FrictMat);
	WOO_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(Cp2_FrictMat_FrictPhys,CPhysFunctor,"TODO",
		((shared_ptr<MatchMaker>,tanPhi,,,"Instance of :ref:`MatchMaker` determining how to compute contact friction angle. If ``None``, minimum value is used."))
		//((Real,ktDivKn,.2,,"Ratio between tangent and normal stiffness on contact."))
		, /*deprec*/
		((ktDivKn,/*given just for syntax reasons*/tanPhi,"! Cp2_FrictMat_FrictPhys.ktDivKn was moved to FrictMat.ktDivKn"))
		,/*init*/,/*ctor*/,/*py*/
	);
};
REGISTER_SERIALIZABLE(Cp2_FrictMat_FrictPhys);


