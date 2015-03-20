#pragma once

#include<woo/pkg/dem/Inlet.hpp>
#include<woo/pkg/dem/Clump.hpp>

struct PsdSphereGenerator: public ParticleGenerator{
	WOO_DECL_LOGGER;
	std::tuple<Real,vector<ParticleAndBox>> operator()(const shared_ptr<Material>&m, const Real& time) WOO_CXX11_OVERRIDE;

	static void sanitizePsd(vector<Vector2r>& psdPts, const string& src);

	void postLoad(PsdSphereGenerator&,void*);
	// return radius and bin for the next particle (also used by derived classes)
	std::tuple<Real,int> computeNextRadiusBin();
	// save bookkeeping information once the particle is generated (also used by derived classes)
	void saveBinMassRadiusTime(int bin, Real m, Real r, Real time);
	void revokeLast() WOO_CXX11_OVERRIDE;
	int lastBin; Real lastM;

	void clear() WOO_CXX11_OVERRIDE { ParticleGenerator::clear(); weightTotal=0.; std::fill(weightPerBin.begin(),weightPerBin.end(),0.); }
	Real critDt(Real density, Real young) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return true; }
	py::tuple pyInputPsd(bool normalize, bool cumulative, int num) const;
	Vector2r minMaxDiam() const WOO_CXX11_OVERRIDE;
	Real padDist() const WOO_CXX11_OVERRIDE{ if(psdPts.empty()) return NaN; return psdPts[0][1]/2.; }


	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(PsdSphereGenerator,ParticleGenerator,"Generate spherical particles following a given Particle Size Distribution (PSD)",
		((bool,discrete,false,,"The points on the PSD curve will be interpreted as the only allowed diameter values; if *false*, linear interpolation between them is assumed instead. Do not change once the generator is running."))
		((vector<Vector2r>,psdPts,,AttrTrait<Attr::triggerPostLoad>().buttons({"Plot PSD","import pylab; pylab.plot(*zip(*self.psdPts)); pylab.grid(True); pylab.xlabel('diameter'); pylab.ylabel('Cumulative fraction'); pylab.show()",""},/*showBefore*/false).multiUnit().lenUnit().fractionUnit(),"Points of the PSD curve; the first component is particle diameter [m] (not radius!), the second component is passing percentage. Both diameter and passing values must be increasing (diameters must be strictly increasing). Passing values are normalized so that the last value is 1.0 (therefore, you can enter the values in percents if you like)."))
		((bool,mass,true,,"PSD has mass percentages; if false, number of particles percentages are assumed. Do not change once the generator is running."))
		((vector<Real>,weightPerBin,,AttrTrait<>().noGui().noDump().readonly(),"Keep track of mass/number of particles for each point on the PSD so that we get as close to the curve as possible. Only used for discrete PSD."))
		((Real,weightTotal,,AttrTrait<>().noGui().noDump().readonly(),"Total mass (number, with *discrete*) of of particles generated."))
		, /* ctor */
		, /* py */
			.def("inputPsd",&PsdSphereGenerator::pyInputPsd,(py::arg("normalize")=true,py::arg("cumulative")=true,py::arg("num")=80),"Return input PSD; it will be a staircase function if *discrete* is true, otherwise linearly interpolated. With *normalize*, the maximum is equal to 1.; otherwise, the curve is multiplied with the actually generated mass/numer of particles (depending on whether :obj:`mass` is true or false); the result should then be very similar to the :obj:`woo.dem.ParticleGenerator.psd` output with actually generated spheres. Discrete non-cumulative PSDs are handled specially: discrete distributions return skypline plot with peaks represented as plateaus of the relative width 1/*num*; continuous distributions return ideal histogram computed for relative bin with 1/*num*; thus returned histogram will match non-cummulative histogram returned by :obj:`ParticleGenerator.psd(cumulative=False) <woo.dem.ParticleGenerator.psd>`, provided *num* is the same in both cases.\n\n .. todo:: with ``cumulative=False`` and when the PSD does not :math:`x`-start at 0, the plot is not correct as the hump coming from the jump in the PSD (from 0 to non-zero at the left) is not taken in account.")
	);
};
WOO_REGISTER_OBJECT(PsdSphereGenerator);

struct PsdClumpGenerator: public PsdSphereGenerator {
	WOO_DECL_LOGGER;
	std::tuple<Real,vector<ParticleAndBox>> operator()(const shared_ptr<Material>&m, const Real& time) WOO_CXX11_OVERRIDE;
	void clear() WOO_CXX11_OVERRIDE { genClumpNo.clear(); /*call parent*/ PsdSphereGenerator::clear(); };
	Real critDt(Real density, Real young) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return true; }
	WOO_CLASS_BASE_DOC_ATTRS(PsdClumpGenerator,PsdSphereGenerator,"Generate clump particles following a given Particle Size Distribution. !! FULL DOCUMENTATION IS IN py/_monkey/extraDocs.py !!!",
		((vector<shared_ptr<SphereClumpGeom>>,clumps,,,"Sequence of clump geometry definitions (:obj:`SphereClumpGeom`); for every selected radius from the PSD, clump will be chosen based on the :obj:`SphereClumpGeom.scaleProb` function and scaled to that radius."))
		((vector<int>,genClumpNo,,AttrTrait<>().noGui().readonly(),"If :obj:`save` is set, keeps clump numbers (indices in :obj:`clumps` for each generated clump."))
		((vector<Quaternionr>,oris,,,"Base orientations for clumps, in the same order as :obj:`clumps`. If the sequence is shorter than selected clump configuration, its orientation will be completely random."))
		((vector<Real>,oriFuzz,,,"Fuzz (in radians) for orientation, if set based on :obj:`oris`; if :obj:`oriFuzz` is shorter than clump configuration in question, the base orientation (:obj:`oriFuzz`) is used as-is. Random orientation axis is picked first, then rotation angle is taken from ``〈-oriFuzz[i],+oriFuzz[i]〉`` with uniform probability. Note: this simple algorithm does not generate completely homogeneous probability density in the rotation space."))
	);
};
WOO_REGISTER_OBJECT(PsdClumpGenerator);

#ifndef WOO_NOCAPSULE
struct PsdCapsuleGenerator: public PsdSphereGenerator {
	WOO_DECL_LOGGER;
	std::tuple<Real,vector<ParticleAndBox>> operator()(const shared_ptr<Material>&m, const Real& time) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return false; }
	// clear, critDt: same as for PsdSphereGenerator
	WOO_CLASS_BASE_DOC_ATTRS(PsdCapsuleGenerator,PsdSphereGenerator,"Generate capsules following a given Particle Size Distribution; elongation is chosen randomly using :obj:`shaftRadiusRatio`; orientation is random.",
		((Vector2r,shaftRadiusRatio,Vector2r(.5,1.5),,"Range for :obj:`shaft <Capsule.shaft>` / :obj:`radius <Capsule.radius>`; the ratio is selected uniformly from this range."))
	);
};
WOO_REGISTER_OBJECT(PsdCapsuleGenerator);

struct PharmaCapsuleGenerator: public ParticleGenerator{
	std::tuple<Real,vector<ParticleAndBox>> operator()(const shared_ptr<Material>&m, const Real& time) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return false; }
	Real critDt(Real density, Real young) WOO_CXX11_OVERRIDE;
	Real padDist() const WOO_CXX11_OVERRIDE { return extDiam.minCoeff()/2.; }
	#define woo_dem_PharmaCapsuleGenerator__CLASS_BASE_DOC_ATTRS \
		PharmaCapsuleGenerator,ParticleGenerator,"Generate pharmaceutical capsules of fixed size; they consist of body and cap. two caps of differing diameter and are thus represented as two interpenetrated (clumped) capsules. The default value corresponds to `Human Cap Size 1 <http://www.torpac.com/Reference/sizecharts/Human%20Caps%20Size%20Chart.pdf>`__ from `Torpac <http://www.torpac.com>`.\n\n .. youtube:: kRQt0jxKDG0\n", \
		((Real,len,19.4e-3,,"Total (locked) length of the capsule.")) \
		((Real,capLen,9.78e-3,,"Cut length of the cap.")) \
		((Vector2r,extDiam,Vector2r(6.91e-3,6.63e-3),,"External diameter of the cap and the body.")) \
		((Vector2r,colors,Vector2r(.5,.99),,"Color of body and cap; white and red with the default (coolwarm) colormap.")) \
		((Real,cutCorr,.5,,"Make the cap shorter by this amount relative to the area of outer cap over the inner cap; this is to compensate for the approximation that the cap is not cut sharply."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_PharmaCapsuleGenerator__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(PharmaCapsuleGenerator);

#endif

struct PsdEllipsoidGenerator: public PsdSphereGenerator {
	WOO_DECL_LOGGER;
	std::tuple<Real,vector<ParticleAndBox>> operator()(const shared_ptr<Material>&m, const Real& time) WOO_CXX11_OVERRIDE;
	bool isSpheresOnly() const WOO_CXX11_OVERRIDE { return false; }
	// clear, critDt: same as for PsdSphereGenerator
	WOO_CLASS_BASE_DOC_ATTRS(PsdEllipsoidGenerator,PsdSphereGenerator,"Generate ellipsoids following a given Particle Size Distribution; ratio of :obj:`Ellipsoid.semiAxes` is chosen randomly from the :obj:`semiAxesRatio` range; orientation is random.",
		((Vector2r,axisRatio2,Vector2r(.5,.5),,"Range for second semi-axis ratio."))
		((Vector2r,axisRatio3,Vector2r(.5,.5),,"Range for third semi-axis ratio; if one of the coefficients is non-positive, the thirs semi-axis will be the same as the second one."))
	);
};
WOO_REGISTER_OBJECT(PsdEllipsoidGenerator);
