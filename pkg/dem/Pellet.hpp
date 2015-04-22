#pragma once
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

struct PelletMat: public FrictMat{
	#define woo_dem_PelletMat__CLASS_BASE_DOC_ATTRS_CTOR \
		PelletMat,FrictMat,ClassTrait().doc("Material describing pellet behavior; see :obj:`Law2_L6Geom_PelletPhys_Pellet` for details of the material model.").section("","",{"PelletAgglomerator"}), \
		((Real,normPlastCoeff,0,,"Coefficient $\\alpha$ in the normal yield function; non-positive deactivates.")) \
		((Real,kaDivKn,0,,"Ratio of $\\frac{k_A}{K_N}$ for the adhesion function; non-positive deactivates.")) \
		, /*ctor*/ createIndex();
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_PelletMat__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(PelletMat,FrictMat);
};
WOO_REGISTER_OBJECT(PelletMat);

#if 0
	struct AgglomPelletMat: public PelletMat{
		WOO_CLASS_BASE_DOC_ATTRS_CTOR(AgglomPelletMat,PelletMat,"Pellet material with additional agglomeration parameters",
			/*attrs*/
			,/*ctor*/createIndex();
		);
		REGISTER_CLASS_INDEX(AgglomPelletMat,PelletMat);
	};
	WOO_REGISTER_OBJECT(AgglomPelletMat);
#endif

struct PelletMatState: public MatState{
	string getScalarName(int index) WOO_CXX11_OVERRIDE {
		switch(index){
			case 0: return "normal+shear dissipation";
			case 1: return "agglom. rate [kg/s]";
			case 2: return "cum. agglom. [kg]";
			case 3: return "cum. agglom. angle [rad]";
			default: return "";
		}
	}
	Real getScalar(int index, const long& step, const Real& smooth=0) WOO_CXX11_OVERRIDE {
		switch(index){
			case 0: return normPlast+shearPlast;
			// invalid value if not yet updated in this step
			case 1: return (step<0||smooth<=0)?agglomRate:pow(smooth,step-stepAgglomUpdated)*agglomRate;
			case 2: return cumAgglomMass;
			case 3: return cumAgglomAngle;
			default: return NaN;	
		}
	}
	#define woo_dem_PelletMatState__CLASS_BASE_DOC_ATTRS \
		PelletMatState,MatState,"Hold dissipated energy data for this particles, to evaluate wear.", \
		((Real,normPlast,0,,"Plastic energy dissipated in the normal sense")) \
		((Real,shearPlast,0,,"Plastic energy dissipated in the tangential sense")) \
		((Real,agglomRate,NaN,,"Agglomeration speed")) \
		((long,stepAgglomUpdated,-1,,"Step in which the agglomeration speed was updated for the last time.")) \
		((Real,cumAgglomMass,0,,"Cumulative mass agglomerated by this particle.")) \
		((Real,cumAgglomAngle,0,,"Cumulative length of rolling when agglomeration was active."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_PelletMatState__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(PelletMatState);

class PelletPhys: public FrictPhys{
	#define woo_dem_PelletPhys_CLASS_BASE_DOC_ATTRS_CTOR \
		PelletPhys,FrictPhys,"Physical properties of a contact of two :obj:`PelletMat`.", \
		((Real,normPlastCoeff,NaN,,"Normal plasticity coefficient.")) \
		((Real,ka,NaN,,"Adhesive stiffness.")) \
		, /*ctor*/ createIndex();
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_PelletPhys_CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(PelletPhys,CPhys);
};
WOO_REGISTER_OBJECT(PelletPhys);

struct Cp2_PelletMat_PelletPhys: public Cp2_FrictMat_FrictPhys{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(PelletMat,PelletMat);
	#define woo_dem_Cp2_PelletMat_PelletPhys_CLASS_BASE_DOC \
		Cp2_PelletMat_PelletPhys,Cp2_FrictMat_FrictPhys,"Compute :obj:`PelletPhys` given two instances of :ref`PelletMat`. :obj:`PelletMat.normPlastCoeff` is averaged into :obj:`PelletPhys.normPlastCoeff`, while minimum of :obj:`PelletMat.kaDivKn` is taken to compute :obj:`PelletPhys.ka`."
	WOO_DECL__CLASS_BASE_DOC(woo_dem_Cp2_PelletMat_PelletPhys_CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(Cp2_PelletMat_PelletPhys);


struct Law2_L6Geom_PelletPhys_Pellet: public LawFunctor{
	bool go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE;
	enum { DISSIP_NORM_PLAST=0, DISSIP_SHEAR_PLAST=1};
	void tryAddDissipState(int what, Real E, const shared_ptr<Contact>& C);

	FUNCTOR2D(L6Geom,PelletPhys);
	WOO_DECL_LOGGER;
	static Real yieldForce (Real uN, Real d0, Real kn, Real alpha){ return (-kn*d0/alpha)*log(alpha*(-uN/d0)+1); }
	static Real yieldForceDerivative(Real uN, Real d0, Real kn, Real alpha){ return kn/(alpha*(-uN/d0)+1); }
	static Real adhesionForce       (Real uN, Real uNPl, Real ka){ return -ka*uNPl-4*(-ka/uNPl)*pow(uN-.5*uNPl,2); }

	#define woo_dem_Law2_L6Geom_PelletPhys_Pellet__CLASS_BASE_DOC_ATTRS_PY \
		Law2_L6Geom_PelletPhys_Pellet,LawFunctor,"Contact law with friction and plasticity in compression, designed for iron ore pellet behavior. See :ref:`pellet-contact-model` for details.", \
		((Real,thinRate,0,,"The amount of reducing particle radius (:math:`\\theta_t`), relative to plastic deformation increment (non-positive to disable thinning)")) \
		((Real,thinRelRMin,.7,,"Minimum radius reachable with sphere thinning at plastic deformation, relative to initial particle size (:math:`r_{\\min}^{\\mathrm{rel}}`)")) \
		((Real,thinExp,-1,,"Exponent for reducing the rate of thinning as the minimum radius is being approached (:math:`\\gamma_t`)")) \
		((Real,thinRefRad,0.,AttrTrait<>().lenUnit(),"Reference radius for thinning; if positive, :obj:`thinRateExp` and :obj:`thinMinExp` are in effect.")) \
		((Real,thinMinExp,0,,"Multiply minimum radius :math:`r_0` by :math:`\\left(\\frac{r}{r_{\\rm thinRefRad}}\\right)^{\\rm thinMinExp}` (size-dependent minimum radius).")) \
		((Real,thinRateExp,0,,"Multiply thinning rate :math:`\\theta_t` (:obj:`thinRate`) by :math:`\\left(\\frac{r}{r_{\\rm thinRefRad}}\\right)^{\\rm thinRateExp}` (size-dependent minimum radius).")) \
		((Real,confSigma,0,,"Confinement stress (acting on :obj:`contact area <L6Geom.contA>`). Negative values will make particles stick together. The strain-stress diagram is shifted vertically with this parameter. The value of confinement can be further scaled with :obj:`confRefRad`.\n\n.. note:: Energy computation might be incorrect with confinement (not yet checked).\n")) \
		((Real,confRefRad,0.,AttrTrait<>().lenUnit(),"If positive, scale the confining stress (:obj:`confSigma`) using the value of :math:`\\left(\\frac{A}{\\pi r_{\\rm ref}^2}\\right)^{\\beta_c}`; this allows to introduce confinement which varies depending on particle size.")) \
		((Real,confExp,1.,,"Dimensionless exponent to be used in conjunction with :obj:`confRefRad`.")) \
		((bool,plastSplit,false,,"Track energy dissipated in normal and tangential sliding separately")) \
		((int,plastIx,-1,AttrTrait<>(),"Index of plastically dissipated energy")) \
		((int,normPlastIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy in the normal sense")) \
		((int,elastPotIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index for elastic potential energy")) \
		, /*py*/ \
			.def("yieldForce",&Law2_L6Geom_PelletPhys_Pellet::yieldForce,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha")),"Return yield force for :obj:`alpha` and given parameters.").staticmethod("yieldForce") \
			.def("yieldForceDerivative",&Law2_L6Geom_PelletPhys_Pellet::yieldForceDerivative,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha")),"Return yield force derivative for given parameters.").staticmethod("yieldForceDerivative") \
			.def("adhesionForce",&Law2_L6Geom_PelletPhys_Pellet::adhesionForce,(py::arg("uN"),py::arg("uNPl"),py::arg("ka")),"Adhesion force function $h$ evaluated with given parameters").staticmethod("adhesionForce")
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Law2_L6Geom_PelletPhys_Pellet__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_PelletPhys_Pellet);

struct PelletCData: public CData{
	#define woo_dem_PelletCData__CLASS_BASE_DOC_ATTRS \
		PelletCData,CData,"Hold state variables for pellet contact.", \
		((Real,uNPl,0.,,"Plastic displacement on the contact."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_PelletCData__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(PelletCData);

struct PelletAgglomerator: public Engine{
	WOO_DECL_LOGGER;
	bool acceptsField(Field* f) WOO_CXX11_OVERRIDE { return dynamic_cast<DemField*>(f); }
	void run() WOO_CXX11_OVERRIDE;
	#define woo_dem_PelletAgglomerator__CLASS_BASE_DOC_ATTRS \
		PelletAgglomerator,Engine,"Compute agglomeration of pellets due to contact some special particles, or wearing due to impacts (only applies to particles with :obj:`PelletMat`.", \
		((vector<shared_ptr<Particle>>,agglomSrcs,,,"Sources of agglomerating mass; particles in contact with this source will have their radius increased based on their relative angular velocity.")) \
		((Real,massIncPerRad,NaN,,"Increase of sphere mass per one radian of rolling (radius is increased in such way that mass increase is satisfied).")) \
		((Real,dampHalfLife,-10000,,"Half-life for rotation damping (includes both rolling and twist); if negative, relative to the (initial) :obj:`woo.core.Scene.dt`; zero deactivates damping. Half-life is $t_{1/.2}=\\frac{\\ln 2}{\\lambda}$ where $\\lambda$ is decay coefficient applied as $\\d\\omega=-\\lambda\\omega$ (see http://en.wikipedia.org/wiki/Exponential_decay for details)."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_PelletAgglomerator__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(PelletAgglomerator);
