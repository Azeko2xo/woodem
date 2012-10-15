#pragma once 

#include<woo/core/Preprocessor.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

struct BinSeg: public Preprocessor {
	shared_ptr<Scene> operator()(){
		auto pre=py::import("woo.pre.BinSeg_");
		return py::call<shared_ptr<Scene>>(py::getattr(pre,"run").ptr(),boost::ref(*this));
	}
	void postLoad(BinSeg&){
		if(!psd.empty() && material){
			ccaDt=pWaveSafety*.5*psd[0][0]/sqrt(material->young/material->density);
		} else {
			ccaDt=NaN;
		}
	}

	WOO_CLASS_BASE_DOC_ATTRS_CTOR(BinSeg,Preprocessor,"Preprocessor for the bin segregation simulation.",
		((Vector3r,size,Vector3r(1,1,.2),AttrTrait<>().lenUnit().prefUnit("mm").startGroup("Geometry"),"Size of the bin: width, height and depth (thickness). Width is real horizontal dimension of the bin. Height is height of the left boundary (which is different from the right one, if :ref:`inclination` is non-zero)."))
		((Real,inclination,0,AttrTrait<>().angleUnit().prefUnit("deg"),"Inclination of the bottom of the bin. Positive angle makes the right hole higher than the left one."))
		((Vector2r,hole1,Vector2r(.1,.08),AttrTrait<>().lenUnit().prefUnit("mm"),"Distance from the left wall and width of the first hole. Lengths are measured in the bottom (possibly inclined) plane."))
		((Vector2r,hole2,Vector2r(.25,.08),AttrTrait<>().lenUnit().prefUnit("mm"),"Distance from the right wall and width of the second hole. Lengths are measured in the bottom (possibly inclined) plane."))
		((Real,holeLedge,.03,AttrTrait<>().lenUnit().prefUnit("mm"),"Distance between front wall and hole, and backwall and hole"))
		((Vector2r,feedPos,Vector2r(.1,.15),AttrTrait<>().lenUnit().prefUnit("mm"),"Distance of the feed from the right wall and its width (vertical lengths)"))
		((Real,halfThick,0.,AttrTrait<>().lenUnit().prefUnit("mm"),"Half-thickenss of boundary plates; all dimensions are given as inner sizes (including hole widths), there is no influence of halfThick on geometry"))
		((bool,halfDp,true,,"Only simulate half of the bin, by putting frictionless wall in the middle"))

		((vector<Vector2r>,psd,vector<Vector2r>({Vector2r(0.015,.0),Vector2r(.025,.2),Vector2r(.03,1.)}),AttrTrait<>().startGroup("Feed").triggerPostLoad().multiUnit().lenUnit().prefUnit("mm").fractionUnit().prefUnit("%"),"Particle size distribution of generated particles: first value is diameter, second value is cummulative fraction"))
		((shared_ptr<FrictMat>,material,make_shared<FrictMat>(),AttrTrait<Attr::triggerPostLoad>(),"Material of particles"))
		((Real,feedRate,1.,,"Feed rate"))
		((shared_ptr<FrictMat>,wallMaterial,,,"Material of walls (if not given, material for particles is used)"))
		((string,loadSpheresFrom,"",,"Load initial packing from file (format x,y,z,radius)"))
		((Vector2r,startStopMass,Vector2r(NaN,NaN),,"Mass for starting and stopping PSD measuring. Once the first mass is reached, PSD counters are engaged; once the second value is reached, report is generated and simulation saved and stopped. Negative values are relative to initial total particles mass. 0 or NaN deactivates; both limits must be active or both inactive."))

		// Estimates
		((Real,ccaDt   ,,AttrTrait<>().readonly().timeUnit().startGroup("Estimates"),"Δt"))
		((long,ccaSteps,,AttrTrait<>().readonly(),"number of steps"))

		// Outputs
		((string,reportFmt,"/tmp/{tid}.xhtml",AttrTrait<>().startGroup("Outputs"),"Report output format (Scene.tags can be used)."))
		((string,feedCacheDir,".",,"Directory where to store pre-generated feed packings"))
		((string,saveFmt,"/tmp/{tid}-{stage}.bin.gz",,"Savefile format; keys are :ref:`Scene.tags` and additionally ``{stage}`` will be replaced by 'init', 'backup-[step]'."))
		((int,backupSaveTime,1800,,"How often to save backup of the simulation (0 or negative to disable)"))
		//((Real,vtkFreq,4,AttrTrait<>(),"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."))
		//((string,vtkPrefix,"/tmp/{tid}-",,"Prefix for saving VtkExport data; formatted with ``format()`` providing :ref:`Scene.tags` as keys."))

		// Tunables
		((Real,gravity,10.,AttrTrait<>().accelUnit().startGroup("Tunables"),"Gravity acceleration magnitude"))
		((Real,damping,.4,,"Non-viscous damping coefficient"))
		((Real,pWaveSafety,.7,AttrTrait<Attr::triggerPostLoad>(),"Safety factor for critical timestep"))
		((Real,rateSmooth,.2,,"Smoothing factor for plotting rates in factory and deleters"))
		((Real,feedAdjustCoeff,.3,,"Coefficient for adjusting feed rate"))
		((int,factStep,100,,"Interval at which particle factory and deleters are run"))
		((vector<Vector2r>,deadTriangles,,,"Triangles given as sequence of points in the xz plane, where spheres will be removed from *loadSpheres* from. 2*rMax from the edge, they will be created as fixed-position"))
		((Real,deadFixedRel,1.,,"Thickness of fixed boundary between live and dead regions, relative to maximum radius of the PSD"))

		// internal variables for saving state in simulation
		((vector<Particle::id_t>,hole1ids,,AttrTrait<>().noGui(),"Ids of hole 1 particles (for later removal)"))
		((vector<Particle::id_t>,hole2ids,,AttrTrait<>().noGui(),"Ids of hole 2 particles (for later removal)"))
		((int,holeMask,,AttrTrait<>().noGui().noDump(),"Mask of hole particles (used for ParticleContainer.reappear)"))
		, /*ctor*/
			material->density=3200;
			material->cast<FrictMat>().young=1e5;
			material->cast<FrictMat>().ktDivKn=.2;
			material->cast<FrictMat>().tanPhi=tan(.5);
	);
};
REGISTER_SERIALIZABLE(BinSeg);

