#pragma once

#include<woo/pkg/dem/Factory.hpp>
#include<woo/pkg/dem/Clump.hpp>

struct PsdSphereGenerator: public ParticleGenerator{
	DECLARE_LOGGER;
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m) WOO_CXX11_OVERRIDE;
	void postLoad(PsdSphereGenerator&,void*);
	// return radius and bin for the next particle (also used by derived classes)
	std::tuple<Real,int> computeNextRadiusBin();
	// save bookkeeping information once the particle is generated (also used by derived classes)
	void saveBinMassRadius(int bin, Real m, Real r);
	void revokeLast() WOO_CXX11_OVERRIDE;
	int lastBin; Real lastM;

	void clear() WOO_CXX11_OVERRIDE { ParticleGenerator::clear(); weightTotal=0.; std::fill(weightPerBin.begin(),weightPerBin.end(),0.); }
	Real critDt(Real density, Real young) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return true; }
	py::tuple pyInputPsd(bool scale, bool cumulative, int num) const;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(PsdSphereGenerator,ParticleGenerator,"Generate spherical particles following a given Particle Size Distribution (PSD)",
		((bool,discrete,false,,"The points on the PSD curve will be interpreted as the only allowed diameter values; if *false*, linear interpolation between them is assumed instead. Do not change once the generator is running."))
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
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m) WOO_CXX11_OVERRIDE;
	void clear() WOO_CXX11_OVERRIDE { genClumpNo.clear(); /*call parent*/ PsdSphereGenerator::clear(); };
	Real critDt(Real density, Real young) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return true; }
	WOO_CLASS_BASE_DOC_ATTRS(PsdClumpGenerator,PsdSphereGenerator,"Generate clump particles following a given Particle Size Distribution. !! FULL DOCUMENTATION IS IN py/_monkey/extraDocs.py !!!",
		((vector<shared_ptr<SphereClumpGeom>>,clumps,,,"Sequence of clump geometry definitions (:obj:`SphereClumpGeom`); for every selected radius from the PSD, clump will be chosen based on the :obj:`SphereClumpGeom.scaleProb` function and scaled to that radius."))
		((vector<int>,genClumpNo,,AttrTrait<>().noGui().readonly(),"If :obj:`save` is set, keeps clump numbers (indices in :obj:`clumps` for each generated clump."))
	);
};
WOO_REGISTER_OBJECT(PsdClumpGenerator);

struct PsdCapsuleGenerator: public PsdSphereGenerator {
	DECLARE_LOGGER;
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return false; }
	// clear, critDt: same as for PsdSphereGenerator
	WOO_CLASS_BASE_DOC_ATTRS(PsdCapsuleGenerator,PsdSphereGenerator,"Generate capsules following a given Particle Size Distribution; elongation is chosen randomly using :obj:`shaftRadiusRatio`; orientation is random.",
		((Vector2r,shaftRadiusRatio,Vector2r(.5,1.5),,"Range for :obj:`shaft <Capsule.shaft>` / :obj:`radius <Capsule.radius>`; the ratio is selected uniformly from this range."))
	);
};
WOO_REGISTER_OBJECT(PsdCapsuleGenerator);

struct PsdEllipsoidGenerator: public PsdSphereGenerator {
	DECLARE_LOGGER;
	vector<ParticleAndBox> operator()(const shared_ptr<Material>&m) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return false; }
	// clear, critDt: same as for PsdSphereGenerator
	WOO_CLASS_BASE_DOC_ATTRS(PsdEllipsoidGenerator,PsdSphereGenerator,"Generate ellipsoids following a given Particle Size Distribution; ratio of :obj:`Ellipsoid.semiAxes` is chosen randomly from the :obj:`semiAxesRatio` range; orientation is random.",
		((Vector2r,axisRatio2,Vector2r(.5,.5),,"Range for second semi-axis ratio."))
		((Vector2r,axisRatio3,Vector2r(.5,.5),,"Range for third semi-axis ratio; if one of the coefficients is non-positive, the thirs semi-axis will be the same as the second one."))
	);
};
WOO_REGISTER_OBJECT(PsdEllipsoidGenerator);
