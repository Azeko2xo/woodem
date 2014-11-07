#pragma once

#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>

struct ConcreteMatState: public MatState {
	#define woo_dem_ConcreteMatState__CLASS_BASE_DOC_ATTRS \
		ConcreteMatState,MatState,"State information about body use by the concrete model. None of that is used for computation (at least not now), only for post-processing.", \
		((Real,epsVolumetric,0,,"Volumetric strain around this body (unused for now)")) \
		((int,numBrokenCohesive,0,,"Number of (cohesive) contacts that damaged completely")) \
		((int,numContacts,0,,"Number of contacts with this body")) \
		((Real,normDmg,0,,"Average damage including already deleted contacts (it is really not damage, but 1-relResidualStrength now)")) \
		((Matrix3r,stress,Matrix3r::Zero(),,"Stress tensor of the spherical particle (under assumption that particle volume = pi*r*r*r*4/3.) for packing fraction 0.62")) \
		((Matrix3r,damageTensor,Matrix3r::Zero(),,"Damage tensor computed with microplane theory averaging. state.damageTensor.trace() = state.normDmg"))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_ConcreteMatState__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(ConcreteMatState);


struct ConcreteMat: public FrictMat {
	//public:
	//	virtual shared_ptr<State> newAssocState() const { return shared_ptr<State>(new ConcreteMatState); }
	//	virtual bool stateTypeOk(State* s) const { return s->isA<ConcreteMatState>(); }
	#define woo_dem_ConcreteMat__CLASS_BASE_DOC_ATTRS_CTOR \
		ConcreteMat,FrictMat,"Concrete material, for use with other Concrete classes.", \
		((Real,sigmaT,NaN,,"Initial cohesion [Pa]")) \
		((bool,neverDamage,false,,"If true, no damage will occur (for testing only).")) \
		((Real,epsCrackOnset,NaN,,"Limit elastic strain [-]")) \
		((Real,relDuctility,NaN,,"relative ductility of bonds in normal direction")) \
		((int,damLaw,1,,"Law for damage evolution in uniaxial tension. 0 for linear stress-strain softening branch, 1 (default) for exponential damage evolution law")) \
		((Real,dmgTau,((void)"deactivated if negative",-1),,"Characteristic time for normal viscosity. [s]")) \
		((Real,dmgRateExp,0,,"Exponent for normal viscosity function. [-]")) \
		((Real,plTau,((void)"deactivated if negative",-1),,"Characteristic time for visco-plasticity. [s]")) \
		((Real,plRateExp,0,,"Exponent for visco-plasticity function. [-]")) \
		((Real,isoPrestress,0,,"Isotropic prestress of the whole specimen. [P a]")), \
		/*ctor*/ createIndex(); density=4800;

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_ConcreteMat__CLASS_BASE_DOC_ATTRS_CTOR);
	REGISTER_CLASS_INDEX(ConcreteMat,FrictMat);
};
WOO_REGISTER_OBJECT(ConcreteMat);


struct ConcretePhys: public FrictPhys {
	static long cummBetaIter, cummBetaCount;

	static Real solveBeta(const Real c, const Real N);
	Real computeDmgOverstress(Real dt);
	Real computeViscoplScalingFactor(Real sigmaTNorm, Real sigmaTYield,Real dt);

	/* damage evolution law */
	static Real funcG(const Real& kappaD, const Real& epsCrackOnset, const Real& epsFracture, const bool& neverDamage, const int& damLaw);
	static Real funcGDKappa(const Real& kappaD, const Real& epsCrackOnset, const Real& epsFracture, const bool& neverDamage, const int& damLaw);
	/* inverse damage evolution law */
	static Real funcGInv(const Real& omega, const Real& epsCrackOnset, const Real& epsFracture, const bool& neverDamage, const int& damLaw);
	void setDamage(Real dmg);
	void setRelResidualStrength(Real r);

	#define woo_dem_ConcretePhys__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		ConcretePhys,FrictPhys,"Representation of a single interaction of the Concrete type: storage for relevant parameters.", \
		((Real,E,NaN,,"normal modulus (stiffness / crossSection) [Pa]")) \
		((Real,G,NaN,,"shear modulus [Pa]")) \
		/*((Real,tanPhi,NaN,,"tangens of internal friction angle [-]"))*/ \
		((Real,coh0,NaN,,"virgin material cohesion [Pa]")) \
		((Real,crossSection,NaN,,"equivalent cross-section associated with this contact [m²]")) \
		((Real,refLength,NaN,,"initial length of interaction [m]")) \
		((Real,refPD,NaN,,"initial penetration depth of interaction [m] (used with ScGeom)")) \
		((Real,epsCrackOnset,NaN,,"strain at which the material starts to behave non-linearly")) \
		/*((Real,relDuctility,NaN,,"Relative ductility of bonds in normal direction"))*/ \
		((Real,epsFracture,NaN,,"strain at which the bond is fully broken [-]")) \
		((Real,dmgTau,-1,,"characteristic time for damage (if non-positive, the law without rate-dependence is used)")) \
		((Real,dmgRateExp,0,,"exponent in the rate-dependent damage evolution")) \
		((Real,dmgStrain,0,,"damage strain (at previous or current step)")) \
		((Real,dmgOverstress,0,,"damage viscous overstress (at previous step or at current step)")) \
		((Real,plTau,-1,,"characteristic time for viscoplasticity (if non-positive, no rate-dependence for shear)")) \
		((Real,plRateExp,0,,"exponent in the rate-dependent viscoplasticity")) \
		((Real,isoPrestress,0,,"\"prestress\" of this link (used to simulate isotropic stress)")) \
		((bool,neverDamage,false,,"the damage evolution function will always return virgin state")) \
		((int,damLaw,1,,"Law for softening part of uniaxial tension. 0 for linear, 1 for exponential (default)")) \
		((bool,isCohesive,false,,"if not cohesive, interaction is deleted when distance is greater than zero.")) \
		((Vector2r,epsT,Vector2r::Zero(),AttrTrait<Attr::noSave|Attr::readonly>(),"Shear strain; updated incrementally from :obj:`L6Geom.vel`.")) \
		\
		((Real,omega,0,AttrTrait<Attr::noSave|Attr::readonly>().startGroup("Auto-computed"),"Damage parameter")) \
		((Real,epsN,0,AttrTrait<Attr::noSave|Attr::readonly>(),"Normal strain")) \
		((Real,sigmaN,0,AttrTrait<Attr::noSave|Attr::readonly>(),"Normal force")) \
		((Vector2r,sigmaT,Vector2r::Zero(),AttrTrait<Attr::noSave|Attr::readonly>(),"Tangential stress")) \
		((Real,epsNPl,0,AttrTrait<Attr::noSave|Attr::readonly>(),"Normal plastic strain")) \
		/*((Vector2r,epsTPl,Vector3r::Zero(),AttrTrait<Attr::noSave|Attr::readonly>(),"Shear plastic strain"))*/ \
		((Real,kappaD,0,AttrTrait<Attr::noSave|Attr::readonly>(),"Value of the kappa function")) \
		((Real,relResidualStrength,1,AttrTrait<Attr::noSave|Attr::readonly>(),"Relative residual strength")) \
		,/*ctor*/ createIndex(); \
		,/*py*/  \
		.def_readonly("cummBetaIter",&ConcretePhys::cummBetaIter,"Cummulative number of iterations inside ConcreteMat::solveBeta (for debugging).") \
		.def_readonly("cummBetaCount",&ConcretePhys::cummBetaCount,"Cummulative number of calls of ConcreteMat::solveBeta (for debugging).") \
		.def("funcG",&ConcretePhys::funcG,(py::arg("kappaD"),py::arg("epsCrackOnset"),py::arg("epsFracture"),py::arg("neverDamage")=false,py::arg("damLaw")=1),"Damage evolution law, evaluating the $\\omega$ parameter. $\\kappa_D$ is historically maximum strain, *epsCrackOnset* ($\\varepsilon_0$) = :yref:`ConcretePhys.epsCrackOnset`, *epsFracture* = :yref:`ConcretePhys.epsFracture`; if *neverDamage* is ``True``, the value returned will always be 0 (no damage).") \
		.staticmethod("funcG") \
		.def("funcGInv",&ConcretePhys::funcGInv,(py::arg("omega"),py::arg("epsCrackOnset"),py::arg("epsFracture"),py::arg("neverDamage")=false,py::arg("damLaw")=1),"Inversion of damage evolution law, evaluating the $\\kappa_D$ parameter. $\\omega$ is damage, for other parameters see funcG function") \
		.staticmethod("funcGInv") \
		.def("setDamage",&ConcretePhys::setDamage,"TODO") \
		.def("setRelResidualStrength",&ConcretePhys::setRelResidualStrength,"TODO")
	
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_ConcretePhys__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	WOO_DECL_LOGGER;
	REGISTER_CLASS_INDEX(ConcretePhys,FrictPhys);
};
WOO_REGISTER_OBJECT(ConcretePhys);

struct Cp2_ConcreteMat_ConcretePhys: public CPhysFunctor{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(ConcreteMat,ConcreteMat);
	WOO_DECL_LOGGER;
	#define woo_dem_Cp2_ConcreteMat_ConcretePhys__CLASS_BASE_DOC_ATTRS \
		Cp2_ConcreteMat_ConcretePhys,CPhysFunctor,"Compute :obj:`ConcretePhys` from two :obj:`ConcreteMat` instances. Uses simple (arithmetic) averages if material are different. Simple copy of parameters is performed if the instance of :obj:`ConcreteMat` is shared.", \
		((long,cohesiveThresholdStep,10,,"Should new contacts be cohesive? They will before this iter#, they will not be afterwards. If 0, they will never be. If negative, they will always be created as cohesive (10 by default)."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cp2_ConcreteMat_ConcretePhys__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(Cp2_ConcreteMat_ConcretePhys);


struct Law2_L6Geom_ConcretePhys: public LawFunctor{
	// Real elasticEnergy();
	Real yieldSigmaTMagnitude(Real sigmaN, Real omega, Real coh0, Real tanPhi) {
		switch(yieldSurfType){
			case 0: return coh0*(1-omega)*-sigmaN*tanPhi;
			case 1: return sqrt(pow(coh0,2)-2*coh0*tanPhi*sigmaN)-coh0*omega;
			case 2: return ( (sigmaN/(coh0*yieldLogSpeed))>=1 ? 0 : coh0*((1-omega)+(tanPhi*yieldLogSpeed)*log(-sigmaN/(coh0*yieldLogSpeed)+1)));
			case 3:
				if(sigmaN>0) return coh0*(1-omega)*-sigmaN*tanPhi;
				return ((sigmaN/(coh0*yieldLogSpeed))>=1 ? 0 : coh0*((1-omega)+(tanPhi*yieldLogSpeed)*log(-sigmaN/(coh0*yieldLogSpeed)+1)));
			case 4:
			case 5:
				if(yieldSurfType==4 && sigmaN>0) return coh0*(1-omega)*-sigmaN*tanPhi;
				return (pow(sigmaN-yieldEllipseShift,2)/(-yieldEllipseShift*coh0/((1-omega)*tanPhi)+pow(yieldEllipseShift,2)))>=1 ? 0 : sqrt((-coh0*((1-omega)*tanPhi)*yieldEllipseShift+pow(coh0,2))*(1-pow(sigmaN-yieldEllipseShift,2)/(-yieldEllipseShift*coh0/((1-omega)*tanPhi)+pow(yieldEllipseShift,2))))-omega*coh0;
			default: throw std::logic_error("Law2_L6Geom_ConcretePhys::yieldSigmaTMagnitude: invalid value of yieldSurfType="+to_string(yieldSurfType));
		}
	}

	bool go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(L6Geom,ConcretePhys);
	#define woo_dem_Law2_L6Geom_ConcretePhys__CLASS_BASE_DOC_ATTRS_PY \
		Law2_L6Geom_ConcretePhys,LawFunctor,"Constitutive law for concrete.", \
		((int,yieldSurfType,2,,"yield function: 0: mohr-coulomb (original); 1: parabolic; 2: logarithmic, 3: log+lin_tension, 4: elliptic, 5: elliptic+log")) \
		((Real,yieldLogSpeed,.1,,"scaling in the logarithmic yield surface (should be <1 for realistic results; >=0 for meaningful results)")) \
		((Real,yieldEllipseShift,NaN,,"horizontal scaling of the ellipse (shifts on the +x axis as interactions with +y are given)")) \
		((Real,omegaThreshold,((void)">=1. to deactivate, i.e. never delete any contacts",1.),,"damage after which the contact disappears (<1), since omega reaches 1 only for strain →+∞")) \
		((Real,epsSoft,((void)"approximates confinement -20MPa precisely, -100MPa a little over, -200 and -400 are OK (secant)",-3e-3),,"Strain at which softening in compression starts (non-negative to deactivate)")) \
		((Real,relKnSoft,.3,,"Relative rigidity of the softening branch in compression (0=perfect elastic-plastic, <0 softening, >0 hardening)")) \
		, /*py*/.def("yieldSigmaTMagnitude",&Law2_L6Geom_ConcretePhys::yieldSigmaTMagnitude,(py::arg("sigmaN"),py::arg("omega"),py::arg("coh0"),py::arg("tanPhi")),"Return radius of yield surface for given material and state parameters; uses attributes of the current instance (*yieldSurfType* etc), change them before calling if you need that.") \
		// .def("elasticEnergy",&Law2_L6Geom_ConcretePhys::elasticEnergy,"Compute and return the total elastic energy in all :obj:`ConcretePhys` contacts")
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Law2_L6Geom_ConcretePhys__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_ConcretePhys);


