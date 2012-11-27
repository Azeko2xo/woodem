#pragma once

#include<woo/core/Preprocessor.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/Pellet.hpp>

struct Chute: public Preprocessor{
	shared_ptr<Scene> operator()(){
		auto pre=py::import("woo.pre.Chute_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	void postLoad(Chute&){}

	WOO_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(Chute,Preprocessor,"Preprocessor for conveyor band and chute plate simulation.",
		// general
		((Real,massFlowRate,/*for real length*/200,AttrTrait<>().massFlowRateUnit().prefUnit("t/h").startGroup("General"),"Mass flow rate of particles transported on the conveyor."))
		((Real,convLayerHt,.2,AttrTrait<>().lenUnit(),"Height of trasported layer of material."))
		((Real,time,2,AttrTrait<>().timeUnit(),"Time of the simulation."))

		// conveyor
		((vector<Vector2r>,convPts,vector<Vector2r>({Vector2r(.2,0),Vector2r(.4,.1)}),AttrTrait<>().startGroup("Conveyor").lenUnit(),"Half of the the conveyor cross-section; the x-axis is horizontal, the y-axis vertical. The middle point is automatically at (0,0). The conveyor is always symmetric, reflected around the +y axis. Points must be given from the middle outwards."))
		((vector<Vector2r>,wallPts,vector<Vector2r>({Vector2r(.25,.025),Vector2r(.22,.15),Vector2r(.3,.3)}),AttrTrait<>().lenUnit(),"Side wall cross-section; points are given using the same coordinates as the :ref:`bandPts`."))
		((bool,half,true,,"Only simulate one half of the problem, by making it symmetric around the middle plane."))
		((Real,convWallLen,.5,AttrTrait<>().lenUnit(),"Length of the part of the conveyor convered with wall."))
		((Real,convBareLen,.5,AttrTrait<>().lenUnit(),"Length of the conveyor without wall. In this part, the conveyor shape is interpolated so that it becomes flat at the exit cylinder's top (y=0 in cross-section coordinates)"))
		((Real,exitCylDiam,.2,AttrTrait<>().lenUnit(),"Diameter of the exit cylinder"))
		((Real,convSlope,15*M_PI/180,AttrTrait<>().angleUnit().prefUnit("deg"),"Slope of the conveyor; 0=horizontal, positive = ascending."))

		// particles
		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(.02,0),Vector2r(.03,.2),Vector2r(.04,1.)}),AttrTrait<>().startGroup("Particles").multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of transported particles."))
		((shared_ptr<PelletMat>,material,make_shared<PelletMat>(),,"Material of particles"))
		((shared_ptr<PelletMat>,convMaterial,,,"Material of the conveyor (if not given, material of particles is used)"))

		// outputs
		((string,reportFmt,"/tmp/{tid}.xhtml",AttrTrait<>().startGroup("Outputs"),"Report output format (Scene.tags can be used)."))
		((string,feedCacheDir,".",,"Directory where to store pre-generated feed packings"))
		((string,saveFmt,"/tmp/{tid}-{stage}.bin.gz",,"Savefile format; keys are :ref:`Scene.tags` and additionally ``{stage}`` will be replaced by 'init', 'steady' and 'done'."))
		((int,backupSaveTime,1800,,"How often to save backup of the simulation (0 or negative to disable)"))
		((Real,vtkFreq,4,AttrTrait<>(),"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."))
		((string,vtkPrefix,"/tmp/{tid}-",,"Prefix for saving VtkExport data; formatted with ``format()`` providing :ref:`Scene.tags` as keys."))
		((vector<string>,reportHooks,,AttrTrait<>().noGui(),"Python expressions returning a 3-tuple with 1. raw HTML to be included in the report, 2. list of (figureName,matplotlibFigure) to be included in figures, 3. dictionary to be added to the 'custom' dict saved in the database."))

		// tunables
		((int,factStepPeriod,200,AttrTrait<>().startGroup("Tunables"),"Run factory (and deleters) every *factStepPeriod* steps."))
		((Real,pWaveSafety,.7,AttrTrait<Attr::triggerPostLoad>(),"Safety factor for critical timestep"))
		((Real,gravity,10.,AttrTrait<>().accelUnit(),"Gravity acceleration magnitude"))
		((Real,rateSmooth,.2,,"Smoothing factor for plotting rates in factory and deleters"))
		, /*deprec*/
		, /*init*/
		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e5;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
			material->cast<PelletMat>().normPlastCoeff=50;
			material->cast<PelletMat>().kaDivKn=0.;
		, /*py*/
	);
};
REGISTER_SERIALIZABLE(Chute);

