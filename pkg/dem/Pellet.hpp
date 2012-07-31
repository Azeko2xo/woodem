#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

struct PelletMat: public FrictMat{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(PelletMat,FrictMat,"Material describing pellet behavior; see :ref:`Law2_L6Geom_PelletPhys_Pellet` for details of the material model.",
		((Real,normPlastCoeff,0,,"Coefficient $\\alpha$ in the normal yield function; non-positive deactivates."))
		((Real,kaDivKn,0,,"Ratio of $\\frac{k_A}{K_N}$ for the adhesion function; non-positive deactivates."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(PelletMat,FrictMat);
};
REGISTER_SERIALIZABLE(PelletMat);

class PelletPhys: public FrictPhys{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(PelletPhys,FrictPhys,"Physical properties of a contact of two :ref:`PelletMat`.",
		((Real,normPlastCoeff,NaN,,"Normal plasticity coefficient."))
		((Real,ka,NaN,,"Adhesive stiffness."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(PelletPhys,CPhys);
};
REGISTER_SERIALIZABLE(PelletPhys);

struct Cp2_PelletMat_PelletPhys: public Cp2_FrictMat_FrictPhys{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(PelletMat,PelletMat);
	WOO_CLASS_BASE_DOC(Cp2_PelletMat_PelletPhys,Cp2_FrictMat_FrictPhys,"Compute :ref:`PelletPhys` given two instances of :ref`PelletMat`. :ref:`PelletMat.normPlastCoeff` is averaged into :ref:`PelletPhys.normPlastCoeff`, while minimum of :ref:`PelletMat.kaDivKn` is taken to compute :ref:`PelletPhys.ka`."
	);
};
REGISTER_SERIALIZABLE(Cp2_PelletMat_PelletPhys);


struct Law2_L6Geom_PelletPhys_Pellet: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	FUNCTOR2D(L6Geom,PelletPhys);
	DECLARE_LOGGER;
	static Real yieldForce          (Real uN, Real d0, Real kn, Real alpha){return (-kn*d0/alpha)*log(alpha*(-uN/d0)+1); }
	static Real yieldForceDerivative(Real uN, Real d0, Real kn, Real alpha){ return kn/(alpha*(-uN/d0)+1); }
	static Real adhesionForce       (Real uN, Real uNPl, Real ka){ return -ka*uNPl-4*(-ka/uNPl)*pow(uN-.5*uNPl,2); }
	WOO_CLASS_BASE_DOC_ATTRS_PY(Law2_L6Geom_PelletPhys_Pellet,LawFunctor,"Contact law with friction and plasticity in compression, designed for iron ore pellet behavior. The normal behavior is determined by yield force $F_N^y=-\\frac{k_N d_0}{\\alpha}\\log(\\alpha\\frac{-u_N}{d_0}+1),$ where $k_N$ is :ref:`contact stiffness<FrictPhys.kn>` $u_N$ is :ref:`normal displacement<L6Geom.uN>`, $d_0$ is :ref:`initial contact length<L6Geom.lens>`. :math:`F_N^y'(u_N=0)=k_N`, which means the contact has initial stiffness $k_N$, but accumulates plastic displacement $u_N^{\\rm pl}$. Trial force is computed as :math`F_N_T=k_N(u_N+u_N^{\\rm pl})`.\n\n"
	"If $F_N_^T<0$ (compression) and $F_N^T<F_N^y$, we update plastic displacement to $u_N^{\\rm pl}=u_N-F_N^y/k_N$ and set $F_N=F_N^y$; otherwise (elastic regime), $F_N=F_N^T$ is used.\n\n"
	"If $F_N^T\\geq0$, we evaluate adhesion function $h(u_N)=-k_A u_N^{\\rm pl}-4\\frac{-k_A}{u_N^{\\rm pl}}\\left(u_N-\\frac{u_N^{\\rm pl}}{2}\\right)^2$ and set $F_N=\\min(F_N^T,h(u_N))$. The function $h$ is parabolic, is zero for $u_N\\in\\{u_N^{\\rm pl},0\\}$ and reaches its maximum for $h\\left(u_N=\\frac{u_N^{\\rm pl}}{2}\\right)=-k_A u_N^{\\rm pl}$, where $k_A$ is the adhesion modulus (positive value), computed from :ref:`kaDivKn`."
	,
		((Real,thinningFactor,0,,"The amount of reducing particle radius, relative to plastic deformation increment (non-positive to disable thinning)"))
		((Real,rMinFrac,.7,,"Minimum radius reachable with sphere thinning at plastic deformation"))
		//((Real,alpha,-1.,,"$\\alpha$  coefficient in the yield function; if negative, compressive plasticity is deactivated. This coefficient is dimensionless."))
		//((Real,kADivKn,.1,,"Ratio kA/kN (for adhesion); if non-positive, adhesion is disabled"))
		((bool,plastSplit,false,,"Track energy dissipated in normal and tangential sliding separately"))
		((int,plastIx,-1,AttrTrait<>(),"Index of plastically dissipated energy"))
		((int,normPlastIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy in the normal sense"))
		((int,elastPotIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index for elastic potential energy"))
		, /*py*/
			.def("yieldForce",&Law2_L6Geom_PelletPhys_Pellet::yieldForce,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha")),"Return yield force for :ref:`alpha` and given parameters.").staticmethod("yieldForce")
			.def("yieldForceDerivative",&Law2_L6Geom_PelletPhys_Pellet::yieldForceDerivative,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha")),"Return yield force derivative for given parameters.").staticmethod("yieldForceDerivative")
			.def("adhesionForce",&Law2_L6Geom_PelletPhys_Pellet::adhesionForce,(py::arg("uN"),py::arg("uNPl"),py::arg("ka")),"Adhesion force function $h$ evaluated with given parameters").staticmethod("adhesionForce")
	);
};
REGISTER_SERIALIZABLE(Law2_L6Geom_PelletPhys_Pellet);

struct PelletCData: public CData{
	WOO_CLASS_BASE_DOC_ATTRS(PelletCData,CData,"Hold state variables for pellet contact.",
		((Real,uNPl,0.,,"Plastic displacement on the contact."))
	);
};
REGISTER_SERIALIZABLE(PelletCData);

