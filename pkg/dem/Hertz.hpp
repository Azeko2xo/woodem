#pragma once
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>

class HertzPhys: public FrictPhys{
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(HertzPhys,FrictPhys,"Physical properties of a contact of two :ref:`FrictMat` with viscous damping enabled (viscosity is currently not provided as material parameter).",
		((Real,kn0,0,,"Constant for computing current normal stiffness."))
		((Real,kt0,0,,"Constant for computing current normal stiffness."))
		((Real,Fa,0,,"Adhesion force for this contact"))
		((Real,alpha_sqrtMK,0,,"Value for computing damping coefficient (see Antypov & Elliott, 'On an analytical solution for the damped Hertzian spring', eq (10)."))
		, /*ctor*/ createIndex();
	);
	REGISTER_CLASS_INDEX(HertzPhys,CPhys);
};
WOO_REGISTER_OBJECT(HertzPhys);

struct Cp2_FrictMat_HertzPhys: public Cp2_FrictMat_FrictPhys{
	void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(FrictMat,FrictMat);
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS(Cp2_FrictMat_HertzPhys,Cp2_FrictMat_FrictPhys,"Compute :ref:`HertzPhys` given two instances of :ref`FrictMat`.",
		((Real,poisson,.2,,"Poisson ratio for computing contact properties (not provided by the material class currently)"))
		((Real,gamma,0.0,,"Surface energy parameter [J/m^2] per each unit contact surface, to derive DMT formulation from HM. If zero, adhesion is disabled."))
		((Real,en,NaN,,"Normal coefficient of restitution (if outside the 0-1 range, there will be no damping, making en effectively equal to one)."))
	);
};
WOO_REGISTER_OBJECT(Cp2_FrictMat_HertzPhys);


// Cp2_FrictMat_HertzPhys

struct Law2_L6Geom_HertzPhys_DMT: public LawFunctor{
	void go(const shared_ptr<CGeom>&, const shared_ptr<CPhys>&, const shared_ptr<Contact>&);
	// fast func for computing x^(i/2)
	static Real pow_i_2(const Real& x, const short& i) { return pow(sqrt(x),i);}
	// fast computation of x^(1/4)
	static Real pow_1_4(const Real& x) { return sqrt(sqrt(x)); }
	// normal elastic energy; see Popov2010, pg 60, eq (5.25)
	inline Real normalElasticEnergy(const Real& kn0, const Real& uN){ return kn0*(2/5.)*pow_i_2(uN,5); }
	FUNCTOR2D(L6Geom,HertzPhys);
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS_PY(Law2_L6Geom_HertzPhys_DMT,LawFunctor,"Law for Hertz contact with optional adhesion (DMT (Derjaguin-Muller-Toporov) :cite:`Derjaguin1975`), non-linear viscosity (:cite:`Antypov2011`) The formulation is taken mainly from :cite:`Johnson1987`. The parameters are given through :obj:`Cp2_FrictMat_HertzPhys`.",
		((bool,noAttraction,true,,"Avoid non-physical normal attraction which may result from viscous effects by making the normal force zero if there is attraction ($F_n>0$). This condition is only applied to elastic and viscous part of the normal force. Adhesion, if present, is not limited. See :cite:`Antypov2012`, the 'Model choice' section (pg. 5), for discussion of this effect.\n.. note:: It is technically not possible to break the contact completely while there is still geometrical overlap, so only force is set to zero but the contact still exists."))
		((int,plastIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of plastically dissipated energy."))
		((int,viscNIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of viscous dissipation in the normal sense."))
		((int,viscTIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index of viscous dissipation in the tangent sense."))
		((int,elastPotIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index for elastic potential energy."))
		((int,dmtIx,-1,AttrTrait<Attr::noSave|Attr::hidden>(),"Index for elastic energy of new/broken contacts."))
		, /*py*/
			#if 0
				.def("yieldForce",&Law2_L6Geom_HertzPhys_DMT::yieldForce,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha")),"Return yield force for :ref:`alpha` and given parameters.").staticmethod("yieldForce")
				.def("yieldForceDerivative",&Law2_L6Geom_HertzPhys_DMT::yieldForceDerivative,(py::arg("uN"),py::arg("d0"),py::arg("kn"),py::arg("alpha")),"Return yield force derivative for given parameters.").staticmethod("yieldForceDerivative")
				.def("adhesionForce",&Law2_L6Geom_HertzPhys_DMT::adhesionForce,(py::arg("uN"),py::arg("uNPl"),py::arg("ka")),"Adhesion force function $h$ evaluated with given parameters").staticmethod("adhesionForce")
			#endif
	);
};
WOO_REGISTER_OBJECT(Law2_L6Geom_HertzPhys_DMT);

