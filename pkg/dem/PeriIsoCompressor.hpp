// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#pragma once

#include<woo/pkg/dem/Particle.hpp>

struct PeriIsoCompressor: public Engine{
	bool acceptsFiled(Field* f){ return dynamic_cast<DemField*>(f); }
	DemField* dem;

	void avgStressIsoStiffness(const Vector3r& cellAreas, Vector3r& stress, Real& stiff);

	Real maxDisplPerStep;
	void run();
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(PeriIsoCompressor,Engine,"Compress/decompress cloud of spheres by controlling periodic cell size until it reaches prescribed average stress, then moving to next stress value in given stress series.",
		((vector<Real>,stresses,,,"Stresses that should be reached, one after another"))
		((Real,charLen,-1.,,"Characteristic length, should be something like mean particle diameter (default -1=invalid value))"))
		((Real,maxSpan,-1.,,"Maximum body span in terms of bbox, to prevent periodic cell getting too small. |ycomp|"))
		((Real,maxUnbalanced,1e-4,,"if actual unbalanced force is smaller than this number, the packing is considered stable,"))
		((int,globalUpdateInt,20,,"how often to recompute average stress, stiffness and unbalanced force"))
		((size_t,state,0,,"Where are we at in the stress series"))
		((string,doneHook,"",,"Python command to be run when reaching the last specified stress"))
		((bool,keepProportions,true,,"Exactly keep proportions of the cell (stress is controlled based on average, not its components"))

		((Real,currUnbalanced,NaN,AttrTrait<Attr::readonly>(),"Current unbalanced force (updated internally)"))
		((Real,avgStiffness,NaN,AttrTrait<Attr::readonly>(),"Value of average stiffness (updated internally)"))
		((Vector3r,sigma,Vector3r(NaN,NaN,NaN),AttrTrait<Attr::readonly>(),"Current value of average stress (update internally)"))
		,
		/*ctor*/
			maxDisplPerStep=-1;
		,
		/*py*/
			.def_readonly("currUnbalanced",&PeriIsoCompressor::currUnbalanced,"Current value of unbalanced force")
			.def_readonly("sigma",&PeriIsoCompressor::sigma,"Current stress value")
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(PeriIsoCompressor);