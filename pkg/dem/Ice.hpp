#pragma once
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

struct IceMat: public FrictMat{
	#define woo_dem_IceMat__CLASS_BASE_DOC_ATTRS_CTOR \
		IceMat,FrictMat,ClassTrait().doc("Ice material; see :ref:`ice-contact-model` for details.").section("","",{/*"IceAgglomerator"*/}), \
		((Real,breakN,1e-4,,"Normal strain where cohesion stress is reached.")) \
		((Vector2r,alpha,Vector2r(1.,1.),,"Factors :math:`(\\alpha_w, \\alpha_t)` to compute twisting/rolling stiffnesses from :math:`k_n` and :math:`k_t`.")) \
		((Vector3r,beta,Vector3r(.1,.1,.1),,"Factors :math:`(\\beta_t,\\beta_w,\\beta_r)` for computing cohesion from normal cohesion :math:`c_n`")) \
		((Real,mu,.05,,"Kinetic (rolling) friction coefficient.")) \
		, /*ctor*/ createIndex();
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_IceMat__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(IceMat,FrictMat);
};
WOO_REGISTER_OBJECT(IceMat);

class IcePhys: public FrictPhys{
	#define woo_dem_IcePhys__CLASS_BASE_DOC_ATTRS_CTOR \
		IcePhys,FrictPhys,"Physical properties of a contact of two :obj:`IceMat`.", \
		((Vector2r,kWR,Vector2r(NaN,NaN),,"Twisting and rolling stiffness.")) \
		((Vector4r,brLim,Vector4r(NaN,NaN,NaN,NaN),,"Limits of breakage in all 4 senses (mixed force/torque values).")) \
		((Real,mu,NaN,,"Kinetic (rolling) friction coefficient.")) \
		((int,bonds,0,AttrTrait<>().bits({"bondN","bondT","bondW","bondR","brkN","brkT","brkW","brkR"}),"Bits specifying whether the contact is bonded (in 4 senses) and whether it is breakable (in 4 senses).")) \
		, /*ctor*/ createIndex();
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_IcePhys__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(IcePhys,CPhys);
};
WOO_REGISTER_OBJECT(IcePhys);

struct Cp2_IceMat_IcePhys: public Cp2_FrictMat_FrictPhys{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(IceMat,IceMat);
	#define woo_dem_Cp2_IceMat_IcePhys__CLASS_BASE_DOC_ATTRS \
		Cp2_IceMat_IcePhys,Cp2_FrictMat_FrictPhys,"Compute :obj:`IcePhys` given two instances of :ref`IceMat`.", \
		((int,bonds,0,AttrTrait<>().bits({"bondN","bondT","bondW","bondR","brkN","brkT","brkW","brkR"}),"Bonding bits for new contacts."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cp2_IceMat_IcePhys__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(Cp2_IceMat_IcePhys);

struct Law2_L6Geom_IcePhys: public LawFunctor{
	bool go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE;
	#define woo_dem_Law2_L6Geom_IcePhys__CLASS_BASE_DOC_ATTRS_PY \
		Law2_L6Geom_IcePhys,LawFunctor,"Contact law implementing :ref:`ice-contact-model`.", \
		/* attrs */ \
		,/* py */
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Law2_L6Geom_IcePhys__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_IcePhys);
