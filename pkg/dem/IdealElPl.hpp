#pragma once
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

struct Law2_L6Geom_FrictPhys_IdealElPl: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	FUNCTOR2D(L6Geom,FrictPhys);
	WOO_CLASS_BASE_DOC_ATTRS(Law2_L6Geom_FrictPhys_IdealElPl,LawFunctor,"Ideally elastic-plastic behavior.",
		((bool,iniEqlb,false,,"Consider the intial distance as equilibrium distance (saved in contact data, subtracted from L6Geom.uN); enabling during simulation will only affect newly created contacts; disabling will affect all contacts."))
		((bool,noSlip,false,,"Disable plastic slipping"))
		((bool,noBreak,false,,"Disable removal of contacts when in tension."))
		((int,plastDissipIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy"))
		((int,elastPotIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for elastic potential energy"))
		((int,brokenIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for energy lost in broken contacts with non-zero force"))
		// unused in the non-debugging version, but keep to not break archive compatibility
		//#ifdef WOO_DEBUG
			((Vector2i,watch,Vector2i(-1,-1),,"Print debug information for this coule of IDs"))
		//#endif
	);
};
REGISTER_SERIALIZABLE(Law2_L6Geom_FrictPhys_IdealElPl);

struct IdealElPlData: public CData{
	WOO_CLASS_BASE_DOC_ATTRS(IdealElPlData,CData,"Hold (optional) state variables for ideally elasto-plastic contacts.",
		((Real,uN0,0,,"Reference (equilibrium) value for uN (normal displacement)."))
	);
};
REGISTER_SERIALIZABLE(IdealElPlData);

struct Law2_L6Geom_FrictPhys_LinEl6: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	FUNCTOR2D(L6Geom,FrictPhys);
	WOO_CLASS_BASE_DOC_ATTRS(Law2_L6Geom_FrictPhys_LinEl6,LawFunctor,"Ideally elastic-plastic behavior.",
		((Real,charLen,-1,,"Characteristic length, which is equal to stiffnesses ratio kNormal/kTwist and kShear/kBend. Must be non-negative."))
		((int,elastPotIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for elastic potential energy"))
	);
};
REGISTER_SERIALIZABLE(Law2_L6Geom_FrictPhys_LinEl6);

