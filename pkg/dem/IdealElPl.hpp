#pragma once
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

struct Law2_L6Geom_FrictPhys_IdealElPl: public LawFunctor{
	bool go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(L6Geom,FrictPhys);
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS(Law2_L6Geom_FrictPhys_IdealElPl,LawFunctor,"Ideally elastic-plastic behavior.",
		((bool,iniEqlb,false,,"Consider the intial distance as equilibrium distance (saved in contact data, subtracted from L6Geom.uN); enabling during simulation will only affect newly created contacts; disabling will affect all contacts."))
		((Real,relRollStiff,0.,,"Rolling stiffness relative to :obj:`FrictPhys.kn` × ``charLen`` (with w``charLen`` being the sum of :obj:`L6Geom.lens`). If non-positive, there is no rolling/twisting resistance."))
		((Real,relTwistStiff,0.,,"Twisting stiffness relative to rolling stiffness (see :obj:`relRollStiff`)."))
		((Real,rollTanPhi,0.,AttrTrait<>().range(Vector2r(0,M_PI/2)),"Rolling friction angle -- the rolling force will not exceed Fn × rollTanPhi. This value is applied separately to twisting as well. If non-positive, there is no rolling/twisting resistance."))
		//((Real,rollSlip,0.01,AttrTrait<>().angleUnit(),"Angle at which slipping in rolling occurs -- the magnitude of rolling resistance will never exceed 
		((bool,noSlip,false,,"Disable plastic slipping"))
		((bool,noBreak,false,,"Disable removal of contacts when in tension."))
		((bool,noFrict,false,,"Turn off friction computation, it will be always zero regardless of material parameters"))
		((int,plastDissipIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy"))
		((int,elastPotIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for elastic potential energy"))
		((int,brokenIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for energy lost in broken contacts with non-zero force"))
		// unused in the non-debugging version, but keep to not break archive compatibility
		//#ifdef WOO_DEBUG
			((Vector2i,watch,Vector2i(-1,-1),,"Print debug information for this coule of IDs"))
		//#endif
	);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_FrictPhys_IdealElPl);

struct IdealElPlData: public CData{
	WOO_CLASS_BASE_DOC_ATTRS(IdealElPlData,CData,"Hold (optional) state variables for ideally elasto-plastic contacts.",
		((Real,uN0,0,,"Reference (equilibrium) value for uN (normal displacement)."))
	);
};
WOO_REGISTER_OBJECT(IdealElPlData);

struct Law2_L6Geom_FrictPhys_LinEl6: public LawFunctor{
	bool go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE; 
	FUNCTOR2D(L6Geom,FrictPhys);
	WOO_CLASS_BASE_DOC_ATTRS(Law2_L6Geom_FrictPhys_LinEl6,LawFunctor,"Ideally elastic-plastic behavior.",
		((Real,charLen,-1,,"Characteristic length, which is equal to stiffnesses ratio kNormal/kTwist and kShear/kBend. Must be non-negative."))
		((int,elastPotIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for elastic potential energy"))
	);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_FrictPhys_LinEl6);

