#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/core/MatchMaker.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

class ElastMat: public Material{
	#define woo_dem_ElastMat__CLASS_BASE_DOC_ATTRS_CTOR \
		ElastMat,Material,"Elastic material with contact friction. See also :obj:`ElastMat`.", \
		((Real,young,1e9,AttrTrait<>().stiffnessUnit(),"Young's modulus")) \
		/* ((Real,poisson,.2,,"Poisson's ratio; this value should be only used in internal force computation, not for contacts.")) */ \
		, /*ctor*/ createIndex();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_ElastMat__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(ElastMat,Material);
};
WOO_REGISTER_OBJECT(ElastMat);

class FrictMat: public ElastMat{
	#define woo_dem_FrictMat__CLASS_BASE_DOC_ATTRS_CTOR \
		FrictMat,ElastMat,"Elastic material with contact friction. See also :obj:`ElastMat`.", \
		((Real,tanPhi,.5,,"Tangent of internal friction angle.")) \
		((Real,ktDivKn,.2,,"Ratio of tangent and shear modulus on contact.")) \
		, /*ctor*/ createIndex(); 

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_FrictMat__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(FrictMat,ElastMat);
};
WOO_REGISTER_OBJECT(FrictMat);

class FrictPhys: public CPhys{
	#define woo_dem_FrictPhys__CLASS_BASE_DOC_ATTRS_CTOR \
		FrictPhys,CPhys,"Physical parameters of contact with sliding", \
		((Real,tanPhi,NaN,,"Tangent of friction angle")) \
		((Real,kn,NaN,,"Normal stiffness")) \
		((Real,kt,NaN,,"Tangent stiffness")) \
		, /*ctor*/ createIndex();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_FrictPhys__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(FrictPhys,CPhys);
};
WOO_REGISTER_OBJECT(FrictPhys);

struct Cp2_FrictMat_FrictPhys: public CPhysFunctor{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE;
	void updateFrictPhys(FrictMat& m1, FrictMat& m2, FrictPhys& ph, const shared_ptr<Contact>& C);
	FUNCTOR2D(FrictMat,FrictMat);
	#define woo_dem_Cp2_FrictMat_FrictPhys__CLASS_BASE_DOC_ATTRS \
		Cp2_FrictMat_FrictPhys,CPhysFunctor,"TODO", \
		((shared_ptr<MatchMaker>,tanPhi,,,"Instance of :obj:`MatchMaker` determining how to compute contact friction angle. If ``None``, minimum value is used."))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cp2_FrictMat_FrictPhys__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(Cp2_FrictMat_FrictPhys);


