#pragma once

#include<woo/pkg/dem/Factory.hpp>
#include<woo/pkg/dem/Clump.hpp>

struct PsdSphereGenerator: public ParticleGenerator{
	DECLARE_LOGGER;
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m);
	void postLoad(PsdSphereGenerator&,void*);
	// return radius and bin for the next particle (also used by derived classes)
	std::tuple<Real,int> computeNextRadiusBin();
	// save bookkeeping information once the particle is generated (also used by derived classes)
	void saveBinMassRadius(int bin, Real m, Real r);
	void revokeLast() override;
	int lastBin; Real lastM;

	void clear() override { ParticleGenerator::clear(); weightTotal=0.; std::fill(weightPerBin.begin(),weightPerBin.end(),0.); }
	py::tuple pyInputPsd(bool scale, bool cumulative, int num) const;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(PsdSphereGenerator,ParticleGenerator,"Generate spherical particles following a given Particle Size Distribution (PSD)",
		((bool,discrete,true,,"The points on the PSD curve will be interpreted as the only allowed diameter values; if *false*, linear interpolation between them is assumed instead. Do not change once the generator is running."))
		((vector<Vector2r>,psdPts,,AttrTrait<Attr::triggerPostLoad>(),"Points of the PSD curve; the first component is particle diameter [m] (not radius!), the second component is passing percentage. Both diameter and passing values must be increasing (diameters must be strictly increasing). Passing values are normalized so that the last value is 1.0 (therefore, you can enter the values in percents if you like)."))
		((bool,mass,true,,"PSD has mass percentages; if false, number of particles percentages are assumed. Do not change once the generator is running."))
		((vector<Real>,weightPerBin,,AttrTrait<>().noGui().readonly(),"Keep track of mass/number of particles for each point on the PSD so that we get as close to the curve as possible. Only used for discrete PSD."))
		((Real,weightTotal,,AttrTrait<>().noGui().readonly(),"Total mass (number, with *discrete*) of of particles generated."))
		, /* ctor */
		, /* py */
			.def("inputPsd",&PsdSphereGenerator::pyInputPsd,(py::arg("scale")=false,py::arg("cumulative")=true,py::arg("num")=80),"Return input PSD; it will be a staircase function if *discrete* is true, otherwise linearly interpolated. With *scale*, the curve is multiplied with the actually generated mass/numer of particles (depending on whether *mass* is true or false); the result should then be very similar to the psd() output with actually generated spheres. Discrete non-cumulative PSDs are handled specially: discrete distributions return skypline plot with peaks represented as plateaus of the relative width 1/*num*; continuous distributions return ideal histogram computed for relative bin with 1/*num*; thus returned histogram will match non-cummulative histogram returned by `ParticleGenerator.psd(cumulative=False)`, provided *num* is the same in both cases.")
	);
};
WOO_REGISTER_OBJECT(PsdSphereGenerator);

struct PsdClumpGenerator: public PsdSphereGenerator {
	DECLARE_LOGGER;
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m);
	void clear() override { genClumpNo.clear(); /*call parent*/ PsdSphereGenerator::clear(); };
	WOO_CLASS_BASE_DOC_ATTRS(PsdClumpGenerator,PsdSphereGenerator,"Generate clump particles following a given Particle Size Distribution (PSD).",
		((vector<shared_ptr<SphereClumpGeom>>,clumps,,,"Sequence of clump geometry definitions (:obj:`SphereClumpGeom`); for every selected radius from the PSD, clump will be chosen based on the :obj:`SphereClumpGeom.scaleProb` function and scaled to that radius."))
		((vector<int>,genClumpNo,,AttrTrait<>().noGui().readonly(),"If :obj:`save` is set, keeps clump numbers (indices in :obj:`clumps` for each generated clump."))
	);
};
WOO_REGISTER_OBJECT(PsdClumpGenerator);

