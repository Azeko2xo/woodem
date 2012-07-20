#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>

struct Law2_L6Geom_FrictPhys_Pellet: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	FUNCTOR2D(L6Geom,FrictPhys);
	DECLARE_LOGGER;
	Real yieldForce          (Real uN, Real kn, Real d0){return (-kn*d0/alpha)*log(alpha*(-uN/d0)+1); }
	Real yieldForceDerivative(Real uN, Real kn, Real d0){ return kn/(alpha*(-uN/d0)+1); }
	WOO_CLASS_BASE_DOC_ATTRS_PY(Law2_L6Geom_FrictPhys_Pellet,LawFunctor,"Contact law with friction and plasticity in compression, designed for iron ore pellet behavior. THe normal behavior is determined by yield force $F_N^y=-\\frac{k_N d_0}{\\alpha}\\log(\\alpha\\frac{-u_N}{d_0}+1),$ where $k_N$ is :ref:`contact stiffness<FrictPhys.kn>` $u_N$ is :ref:`normal displacement<L6Geom.uN>`, $d_0$ is :ref:`initial contact length<L6Geom.lens>`. $F_N^y'(u_N=0)=k_N$, which means the contact has initial stiffness $k_N$, but accumulates plastic displacement $u_N^{\\rm pl}$. Trial force is computed as $\\F_N_T=k_N(u_N+u_N^{\\rm pl}$).$ If $F_N^T\\geq0$, $F_N=0$ is set (no adhesion allowed); If $F_N^T<F_N^y$, we update plastic displacement to $u_N^{\\rm pl}=u_N-F_N^y/k_N$ and set $F_N=F_N^y$; otherwise (elastic regime), $F_N=F_N^T$ is used.",
		((Real,alpha,-1.,,"$\\alpha$  coefficient in the yield function; if negative, compressive plasticity is deactivated. This coefficient is dimensionless."))
		((bool,plastSplit,false,,"Track energy dissipated in normal and tangential sliding separately"))
		((int,plastIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy"))
		((int,normPlastIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy in the normal sense"))
		((int,elastPotIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for elastic potential energy"))
		, /*py*/
			.def("yieldForce",&Law2_L6Geom_FrictPhys_Pellet::yieldForce,(py::arg("uN"),py::arg("kN"),py::arg("d0")),"Return yield force for :ref:`alpha` and given parameters.")
			.def("yieldForceDerivative",&Law2_L6Geom_FrictPhys_Pellet::yieldForceDerivative,(py::arg("uN"),py::arg("kN"),py::arg("d0")),"Return yield force derivative for :ref:`alpha` and given parameters.")
	);
};
REGISTER_SERIALIZABLE(Law2_L6Geom_FrictPhys_Pellet);

struct PelletCData: public CData{
	WOO_CLASS_BASE_DOC_ATTRS(PelletCData,CData,"Hold state variables for pellet contact.",
		((Real,uNPl,0.,,"Plastic displacement on the contact."))
	);
};
REGISTER_SERIALIZABLE(PelletCData);

