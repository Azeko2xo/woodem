#pragma once
#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/pkg/dem/L6Geom.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>

class Law2_L6Geom_FrictPhys_IdealElPl: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	FUNCTOR2D(L6Geom,FrictPhys);
	YADE_CLASS_BASE_DOC_ATTRS(Law2_L6Geom_FrictPhys_IdealElPl,LawFunctor,"Ideally elastic-plastic behavior.",
		((bool,noSlip,false,,"Disable plastic slipping"))
		((bool,noBreak,false,,"Disable removal of contacts when in tension."))
		((int,plastDissipIx,-1,(Attr::noSave|Attr::hidden),"Index of plastically dissipated energy"))
		((int,elastPotIx,-1,(Attr::hidden|Attr::noSave),"Index for elastic potential energy"))
		((int,brokenIx,-1,(Attr::hidden|Attr::noSave),"Index for energy lost in broken contacts with non-zero force"))
		#ifdef YADE_DEBUG
			((Vector2i,watch,Vector2i(-1,-1),,"Print debug information for this coule of IDs"))
		#endif
	);
};
REGISTER_SERIALIZABLE(Law2_L6Geom_FrictPhys_IdealElPl);

class Law2_L6Geom_FrictPhys_LinEl6: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	FUNCTOR2D(L6Geom,FrictPhys);
	YADE_CLASS_BASE_DOC_ATTRS(Law2_L6Geom_FrictPhys_LinEl6,LawFunctor,"Ideally elastic-plastic behavior.",
		((Real,charLen,-1,,"Characteristic length, which is equal to stiffnesses ratio kNormal/kTwist and kShear/kBend. Must be non-negative."))
		((int,elastPotIx,-1,(Attr::hidden|Attr::noSave),"Index for elastic potential energy"))
	);
};
REGISTER_SERIALIZABLE(Law2_L6Geom_FrictPhys_LinEl6);


