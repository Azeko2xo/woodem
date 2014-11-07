// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#pragma once

#include<woo/pkg/dem/Particle.hpp>

struct PeriIsoCompressor: public Engine{
	bool acceptsFiled(Field* f){ return dynamic_cast<DemField*>(f); }
	DemField* dem;

	void avgStressIsoStiffness(const Vector3r& cellAreas, Vector3r& stress, Real& stiff);

	Real maxDisplPerStep;
	void run();
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(PeriIsoCompressor,Engine,ClassTrait().doc("Compress/decompress cloud of spheres by controlling periodic cell size until it reaches prescribed average stress, then moving to next stress value in given stress series.").section("Stress/strain control","TODO",{"WeirdTriaxControl"}),
		((vector<Real>,stresses,,,"Stresses that should be reached, one after another"))
		((Real,charLen,-1.,,"Characteristic length, should be something like mean particle diameter (default -1=invalid value))"))
		((Real,maxSpan,-1.,AttrTrait<Attr::readonly>(),"Maximum body span in terms of bbox, to prevent periodic cell getting too small."))
		((Real,maxUnbalanced,1e-4,,"if actual unbalanced force is smaller than this number, the packing is considered stable,"))
		((int,globalUpdateInt,20,,"how often to recompute average stress, stiffness and unbalanced force"))
		((size_t,state,0,,"Where are we at in the stress series"))
		((string,doneHook,"",,"Python command to be run when reaching the last specified stress"))
		((bool,keepProportions,true,,"Exactly keep proportions of the cell (stress is controlled based on average, not its components"))

		((Real,currUnbalanced,NaN,AttrTrait<Attr::readonly>(),"Current unbalanced force (updated internally)"))
		((Real,avgStiffness,NaN,AttrTrait<Attr::readonly>(),"Value of average stiffness (updated internally)"))
		((Vector3r,sigma,Vector3r(NaN,NaN,NaN),AttrTrait<Attr::readonly>(),"Current value of average stress (update internally)"))
		, /*ctor*/ maxDisplPerStep=-1;
		, /* py */
	);
	WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(PeriIsoCompressor);

struct WeirdTriaxControl: public Engine{
	bool acceptsFiled(Field* f){ return dynamic_cast<DemField*>(f); }
	DemField* dem;
	void run();
	//	void strainStressStiffUpdate();
	WOO_CLASS_BASE_DOC_ATTRS(WeirdTriaxControl,Engine,"Engine for independently controlling stress or strain in periodic simulations.\n\n``strainStress`` contains absolute values for the controlled quantity, and ``stressMask`` determines meaning of those values (0 for strain, 1 for stress): e.g. ``( 1<<0 | 1<<2 ) = 1 | 4 = 5`` means that ``strainStress[0]`` and ``strainStress[2]`` are stress values, and ``strainStress[1]`` is strain. \n\nSee scripts/test/periodic-triax.py for a simple example.",
		((Vector3r,goal,Vector3r::Zero(),,"Desired stress or strain values (depending on stressMask), strains defined as ``strain(i)=log(Fii)``.\n\n.. warning:: Strains are relative to the :obj:`woo.core.Scene.cell.refSize` (reference cell size), not the current one (e.g. at the moment when the new strain value is set)."))
		((int,stressMask,((void)"all strains",0),,"mask determining strain/stress (0/1) meaning for goal components"))
		((Vector3r,maxStrainRate,Vector3r(1,1,1),,"Maximum strain rate of the periodic cell."))
		((Real,maxUnbalanced,1e-4,,"maximum unbalanced force."))
		((Real,absStressTol,1e3,,"Absolute stress tolerance"))
		((Real,relStressTol,3e-5,,"Relative stress tolerance; if negative, it is relative to the largest stress value along all axes, where strain is prescribed."))
		((Real,maxStrainedStress,NaN,AttrTrait<Attr::readonly>(),"Current abs-maximum stress in strain-controlled directions; useda as reference when :obj:`relStressTol` is negative."))
		((Real,growDamping,.25,,"Damping of cell resizing (0=perfect control, 1=no control at all)."))
		((Real,relVol,1.,,"For stress computation, use volume of the periodic cell multiplied by this constant."))
		((int,globUpdate,5,,"How often to recompute average stress, stiffness and unbalaced force."))
		((string,doneHook,,,"python command to be run when the desired state is reached"))
		((Matrix3r,stress,Matrix3r::Zero(),,"Stress tensor"))
		((Vector3r,strain,Vector3r::Zero(),,"cell strain, updated automatically"))
		((Vector3r,strainRate,Vector3r::Zero(),,"cell strain rate, updated automatically"))
		//((Vector3r,stiff,Vector3r::Zero(),,"average stiffness (only every globUpdate steps recomputed from interactions)"))
		((Real,currUnbalanced,NaN,,"current unbalanced force (updated every globUpdate)"))
		((Vector3r,prevGrow,Vector3r::Zero(),,"previous cell grow"))
		((Real,mass,NaN,,"mass of the cell (user set); if not set, it will be computed as sum of masses of all particles."))
		((Real,externalWork,0,,"Work input from boundary controller."))
		((int,gradVWorkIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for work done by velocity gradient, if tracking energy"))
		((bool,leapfrogChecked,false,AttrTrait<Attr::hidden>(),"Whether we already checked that we come before :obj:`Leapfrog` (otherwise setting nextGradV will have no effect."))
	);
	WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(WeirdTriaxControl);
