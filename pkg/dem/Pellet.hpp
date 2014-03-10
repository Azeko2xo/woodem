#pragma once
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

struct PelletMat: public FrictMat{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(PelletMat,FrictMat,"Material describing pellet behavior; see :obj:`Law2_L6Geom_PelletPhys_Pellet` for details of the material model.",
		((Real,normPlastCoeff,0,,"Coefficient $\\alpha$ in the normal yield function; non-positive deactivates."))
		((Real,kaDivKn,0,,"Ratio of $\\frac{k_A}{K_N}$ for the adhesion function; non-positive deactivates."))
		, /*ctor*/ createIndex();
	);
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
			default: return "";
		}
	}
	Real getScalar(int index, const long& step, const Real& smooth=0) WOO_CXX11_OVERRIDE {
		switch(index){
			case 0: return normPlast+shearPlast;
			// invalid value if not yet updated in this step
			case 1: return (step<0||smooth<=0)?agglomRate:pow(smooth,step-stepUpdated)*agglomRate;
			default: return NaN;	
		}
	}
	WOO_CLASS_BASE_DOC_ATTRS(PelletMatState,MatState,"Hold dissipated energy data for this particles, to evaluate wear.",
		((Real,normPlast,0,,"Plastic energy dissipated in the normal sense"))
		((Real,shearPlast,0,,"Plastic energy dissipated in the tangential sense"))
		((Real,agglomRate,NaN,,"Agglomeration speed"))
	);
};
WOO_REGISTER_OBJECT(PelletMatState);

class PelletPhys: public FrictPhys{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(PelletPhys,FrictPhys,"Physical properties of a contact of two :obj:`PelletMat`.",
		((Real,normPlastCoeff,NaN,,"Normal plasticity coefficient."))
		((Real,ka,NaN,,"Adhesive stiffness."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(PelletPhys,CPhys);
};
WOO_REGISTER_OBJECT(PelletPhys);

struct Cp2_PelletMat_PelletPhys: public Cp2_FrictMat_FrictPhys{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(PelletMat,PelletMat);
	WOO_CLASS_BASE_DOC(Cp2_PelletMat_PelletPhys,Cp2_FrictMat_FrictPhys,"Compute :obj:`PelletPhys` given two instances of :ref`PelletMat`. :obj:`PelletMat.normPlastCoeff` is averaged into :obj:`PelletPhys.normPlastCoeff`, while minimum of :obj:`PelletMat.kaDivKn` is taken to compute :obj:`PelletPhys.ka`."
	);
};
WOO_REGISTER_OBJECT(Cp2_PelletMat_PelletPhys);


struct Law2_L6Geom_PelletPhys_Pellet: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	enum { DISSIP_NORM_PLAST=0, DISSIP_SHEAR_PLAST=1};
	void tryAddDissipState(int what, Real E, const shared_ptr<Contact>& C);

	FUNCTOR2D(L6Geom,PelletPhys);
	DECLARE_LOGGER;
	static Real yieldForce          (Real uN, Real d0, Real kn, Real alpha, int yieldFunc=0, Real yf1_beta=0, Real yf1_w=0){
		switch(yieldFunc){
			case 0: return (-kn*d0/alpha)*log(alpha*(-uN/d0)+1);
			case 1:
				if(yf1_w<0.56714329041/alpha) LOG_WARN("yiedFunc==1: w="<<yf1_w<<" < lambertW(1)/alpha=0.5671.../"<<alpha<<"="<<0.56714329041/alpha<<", stiffness is negative at origin!")
				return kn*uN+kn*yf1_w*d0*pow(1-exp(alpha*uN/d0),yf1_beta);
		}
		throw std::runtime_error("Law2_L6Geom_PelletPhys_Pellet.yieldFunc must be 0 or 1 (not "+to_string(yieldFunc)+").");
	}
	static Real yieldForceDerivative(Real uN, Real d0, Real kn, Real alpha, int yieldFunc=0, Real yf1_beta=0, Real yf1_w=0){
		switch(yieldFunc){
			case 0: return kn/(alpha*(-uN/d0)+1);
			case 1: return kn+kn*yf1_w*yf1_beta*pow(1-exp(alpha*uN/d0),yf1_beta-1)*(-exp(alpha*uN/d0)*alpha);
		}
		throw std::runtime_error("Law2_L6Geom_PelletPhys_Pellet.yieldFunc must be 0 or 1 (not "+to_string(yieldFunc)+").");
	}
	static Real adhesionForce       (Real uN, Real uNPl, Real ka){ return -ka*uNPl-4*(-ka/uNPl)*pow(uN-.5*uNPl,2); }
	WOO_CLASS_BASE_DOC_ATTRS_PY(Law2_L6Geom_PelletPhys_Pellet,LawFunctor,"Contact law with friction and plasticity in compression, designed for iron ore pellet behavior. See :ref:`pellet-contact-model` for details."
	,
		((Real,thinRate,0,,"The amount of reducing particle radius (:math:`\\theta_t`), relative to plastic deformation increment (non-positive to disable thinning)"))
		((Real,thinRelRMin,.7,,"Minimum radius reachable with sphere thinning at plastic deformation, relative to initial particle size (:math:`r_{\\min}^{\\mathrm{rel}}`)"))
		((Real,thinExp,-1,,"Exponent for reducing the rate of thinning as the minimum radius is being approached (:math:`\\gamma_t`)"))
		((Real,confSigma,0,,"Confinement stress (acting on :obj:`contact area <L6Geom.contA>`). Negative values will make particles stick together. The strain-stress diagram is shifted vertically with this parameter. The value of confinement can be further scaled with :obj:`confRefRad`.\n\n.. note:: Energy computation might be incorrect with confinement (not yet checked).\n"))
		((Real,confRefRad,0.,AttrTrait<>().lenUnit(),"If positive, scale the confining stress (:obj:`confSigma`) using the value of :math:`\\left(\\frac{A}{\\pi r_{\\rm ref}^2}\\right)^{\\beta_c}`; this allows to introduce confinement which varies depending on particle size."))
		((Real,confExp,1.,,"Dimensionless exponent to be used in conjunction with :obj:`confRefRad`."))
		((int,yieldFunc,0,,"Yield function type: 0=usual formulation (decreasing stiffness); 1=exponential with asymptote."))
		((Real,yf1_beta,2.,,"Exponent for hardening with yieldFunc==1."))
		((Real,yf1_w,.2,,"Shift of the linear asymptote from origin, as fraction of initial contact length."))
		//((Real,alpha,-1.,,"$\\alpha$  coefficient in the yield function; if negative, compressive plasticity is deactivated. This coefficient is dimensionless."))
		//((Real,kADivKn,.1,,"Ratio kA/kN (for adhesion); if non-positive, adhesion is disabled"))
		((bool,plastSplit,false,,"Track energy dissipated in normal and tangential sliding separately"))
		((int,plastIx,-1,AttrTrait<>(),"Index of plastically dissipated energy"))
		((int,normPlastIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy in the normal sense"))
		((int,elastPotIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index for elastic potential energy"))
		, /*py*/
			.def("yieldForce",&Law2_L6Geom_PelletPhys_Pellet::yieldForce,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha"),py::arg("yieldFunc")=0,py::arg("yf1_beta")=2.,py::arg("yf1_w")=.2),"Return yield force for :obj:`alpha` and given parameters.").staticmethod("yieldForce")
			.def("yieldForceDerivative",&Law2_L6Geom_PelletPhys_Pellet::yieldForceDerivative,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha"),py::arg("yieldFunc")=0,py::arg("yf1_beta")=2.,py::arg("yf1_w")=.2),"Return yield force derivative for given parameters.").staticmethod("yieldForceDerivative")
			.def("adhesionForce",&Law2_L6Geom_PelletPhys_Pellet::adhesionForce,(py::arg("uN"),py::arg("uNPl"),py::arg("ka")),"Adhesion force function $h$ evaluated with given parameters").staticmethod("adhesionForce")
	);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_PelletPhys_Pellet);

struct PelletCData: public CData{
	WOO_CLASS_BASE_DOC_ATTRS(PelletCData,CData,"Hold state variables for pellet contact.",
		((Real,uNPl,0.,,"Plastic displacement on the contact."))
	);
};
WOO_REGISTER_OBJECT(PelletCData);

struct PelletAgglomerator: public Engine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void run();
	WOO_CLASS_BASE_DOC_ATTRS(PelletAgglomerator,Engine,"Compute agglomeration of pellets due to contact some special particles, or wearing due to impacts (only applies to particles with :obj:`PelletMat`.",
		((vector<shared_ptr<Particle>>,agglomSrcs,,,"Sources of agglomerating mass; particles in contact with this source will have their radius increased based on their relative angular velocity."))
		((Real,massIncPerRad,NaN,,"Increase of sphere mass per one radian of rolling (radius is increased in such way that mass increase is satisfied)."))
		((Real,dampHalfLife,-10000,,"Half-life for rotation damping (includes both rolling and twist); if negative, relative to the (initial) :obj:`woo.core.Scene.dt`; zero deactivates damping. Half-life is $t_{1/.2}=\\frac{\\ln 2}{\\lambda}$ where $\\lambda$ is decay coefficient applied as $\\d\\omega=-\\lambda\\omega$ (see http://en.wikipedia.org/wiki/Exponential_decay for details)."))
	);
};
WOO_REGISTER_OBJECT(PelletAgglomerator);
