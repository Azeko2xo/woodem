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

struct IcePhys: public FrictPhys{
	// bit manipulation for bonds
	bool isBondX(short x) const { return bonds & (1<<x); }
	void setBondX(short x, bool b){ if(!b) bonds&=~(1<<x); else bonds|=(1<<x); }
	bool isBrkX(short x) const { return bonds & (1<<(4+x)); }
	void setBrkX(short x, bool b){ if(!b) bonds&=~(1<<(4+x)); else bonds|=(1<<(4+x)); }
	bool isBrkBondX(short x) const { return bonds & (1<<x) & (1<<(4+x)); }
	void setAllBroken() { bonds&=~( 1<<0 | 1<<1 | 1<<2 | 1<<3 ); }

	#define woo_dem_IcePhys__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		IcePhys,FrictPhys,"Physical properties of a contact of two :obj:`IceMat`.", \
		((Vector2r,kWR,Vector2r(NaN,NaN),,"Twisting and rolling stiffness.")) \
		((Vector2r,brkNT,Vector2r(NaN,NaN),AttrTrait<>().forceUnit(),"Limits of breakage in normal & tangential senses.")) \
		((Vector2r,brkWR,Vector2r(NaN,NaN),AttrTrait<>().torqueUnit(),"Limits of breakage in twisting & rolling senses.")) \
		((Real,mu,NaN,,"Kinetic (rolling) friction coefficient.")) \
		((int,bonds,0,AttrTrait<>().bits({"bondN","bondT","bondW","bondR","brkN","brkT","brkW","brkR"}),"Bits specifying whether the contact is bonded (in 4 senses) and whether it is breakable (in 4 senses).")) \
		((Real,uN0,0,,"Initial value of normal overlap; set automatically by :obj:`Law_L6Geom_IcePhys` when :obj:`~Law_L6Geom_IcePhys.iniEqlb` is true (default).")) \
		, /*ctor*/ createIndex(); \
		, /*py*/ \
			.def("isBondX",&IcePhys::isBondX,"Whether the contact is bonded in the *x* sense (0..3)") \
			.def("isBrkX",&IcePhys::isBrkX,"Whether the contact is breakable in the *x* sense (0..3)") \
			.def("isBrkBondX",&IcePhys::isBrkBondX,"Whether the contact is breakable *and* bonded in the *x* sense (0..3)")
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_IcePhys__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	REGISTER_CLASS_INDEX(IcePhys,CPhys);
};
WOO_REGISTER_OBJECT(IcePhys);

#if 0
struct IceCData: public CData{
#define woo_dem_IceCData__CLASS_BASE_DOC_ATTRS \
	PelletCData,CData,"Hold state variables for pellet contact.", \
		((Real,uNPl,0.,,"Plastic displacement on the contact."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_IceCData__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(IceCData);
#endif


struct Cp2_IceMat_IcePhys: public Cp2_FrictMat_FrictPhys{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(IceMat,IceMat);
	#define woo_dem_Cp2_IceMat_IcePhys__CLASS_BASE_DOC_ATTRS \
		Cp2_IceMat_IcePhys,Cp2_FrictMat_FrictPhys,"Compute :obj:`IcePhys` given two instances of :ref`IceMat`.", \
		((int,bonds0,0,AttrTrait<>().bits({"bondN","bondT","bondW","bondR","brkN","brkT","brkW","brkR"}),"Bonding bits for new contacts, for the initial configuration.")) \
		((int,bonds1,0,AttrTrait<>().bits({"bondN","bondT","bondW","bondR","brkN","brkT","brkW","brkR"}),"Bonding bits for new contacts, for contacts created after the initial configuration.")) \
		((int,step01,3,,":obj:`Step <woo.core.Scene.step>` after which :obj:`bonds1` will be used instead of :obj:`bonds0` for new contacts."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cp2_IceMat_IcePhys__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(Cp2_IceMat_IcePhys);

struct Law2_L6Geom_IcePhys: public LawFunctor{
	bool go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE;

	template<typename ValueT>
	void slipXd(ValueT& x, const Real& norm, const Real& mx, const Real& stiff, Scene* scene, const char* eName, int& eIndex){
		if(norm==0) return;
		Real ratio=mx/norm;
		if(unlikely(scene->trackEnergy) && stiff>0){
			Real dissip=(.5*(norm-mx)+mx)*(norm-mx)/stiff;
			scene->energy->add(dissip,eName,eIndex,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
		}
		x*=ratio;
	}

	Real elastE(const IcePhys& p);
	FUNCTOR2D(L6Geom,IcePhys);
	#define woo_dem_Law2_L6Geom_IcePhys__CLASS_BASE_DOC_ATTRS_PY \
		Law2_L6Geom_IcePhys,LawFunctor,"Contact law implementing :ref:`ice-contact-model`.", \
		/* attrs */ \
		((bool,iniEqlb,true,,"Set the intial distance as equilibrium distance (saved in :obj:`IcePhys.uN0`, subtracted from L6Geom.uN); enabling during simulation will only affect newly created contacts).")) \
		((int,elastIx,-1,AttrTrait<Attr::readonly>(),"Index of elastic energy (cache).")) \
		((int,brokenIx,-1,AttrTrait<Attr::readonly>(),"Index of energy which disappeared when contacts broke (cache).")) \
		((int,plastIx,-1,AttrTrait<Attr::readonly>(),"Index of plastically dissipated energy (cache).")) \
		,/* py */
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Law2_L6Geom_IcePhys__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_IcePhys);
