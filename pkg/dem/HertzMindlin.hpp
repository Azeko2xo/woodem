// 2010 © Chiara Modenese <c.modenese@gmail.com> 
// 
/*
=== HIGH LEVEL OVERVIEW OF Mindlin ===

Mindlin is a set of classes to include the Hertz-Mindlin formulation for the contact stiffnesses.
The DMT formulation is also considered (for adhesive particles, rigid and small bodies).

*/

#pragma once

#include<yade/pkg/dem/FrictPhys.hpp>
#include<yade/pkg/common/ElastMat.hpp>
#include<yade/pkg/common/Dispatching.hpp>
#include<yade/pkg/dem/ScGeom.hpp>
#include<yade/pkg/common/PeriodicEngines.hpp>
#include<yade/pkg/common/NormShearPhys.hpp>
#include<yade/pkg/common/MatchMaker.hpp>


#include <set>
#include <boost/tuple/tuple.hpp>
#include<yade/lib/base/openmp-accu.hpp>


/******************** MindlinPhys *********************************/
class MindlinPhys: public FrictPhys{
	public:
	virtual ~MindlinPhys();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(MindlinPhys,FrictPhys,"Representation of an interaction of the Hertz-Mindlin type.",
			((Real,kno,0.0,,"Constant value in the formulation of the normal stiffness"))
			((Real,kso,0.0,,"Constant value in the formulation of the tangential stiffness"))
			((Real,kr,0.0,,"Rotational stiffness"))
			((Real,ktw,0.0,,"Rotational stiffness"))
			((Real,maxBendPl,0.0,,"Coefficient to determine the maximum plastic moment to apply at the contact"))
			
			((Vector3r,normalViscous,Vector3r::Zero(),,"Normal viscous component"))
			((Vector3r,shearViscous,Vector3r::Zero(),,"Shear viscous component"))
			((Vector3r,shearElastic,Vector3r::Zero(),,"Total elastic shear force"))
			((Vector3r,usElastic,Vector3r::Zero(),,"Total elastic shear displacement (only elastic part)"))
			((Vector3r,usTotal,Vector3r::Zero(),,"Total elastic shear displacement (elastic+plastic part)"))

			//((Vector3r,prevNormal,Vector3r::Zero(),,"Save previous contact normal to compute relative rotation"))
			((Vector3r,momentBend,Vector3r::Zero(),,"Artificial bending moment to provide rolling resistance in order to account for some degree of interlocking between particles"))
			((Vector3r,momentTwist,Vector3r::Zero(),,"Artificial twisting moment (no plastic condition can be applied at the moment)"))
			//((Vector3r,dThetaR,Vector3r::Zero(),,"Incremental rolling vector"))

			((Real,radius,NaN,,"Contact radius (only computed with :yref:`Law2_ScGeom_MindlinPhys_Mindlin::calcEnergy`)"))

			//((Real,gamma,0.0,"Surface energy parameter [J/m^2] per each unit contact surface, to derive DMT formulation from HM"))
			((Real,adhesionForce,0.0,,"Force of adhesion as predicted by DMT"))
			((bool,isAdhesive,false,,"bool to identify if the contact is adhesive, that is to say if the contact force is attractive"))
			((bool,isSliding,false,,"check if the contact is sliding (useful to calculate the ratio of sliding contacts)"))

			// Contact damping coefficients as for linear elastic contact law
			((Real,betan,0.0,,"Fraction of the viscous damping coefficient (normal direction) equal to $\\frac{c_{n}}{C_{n,crit}}$."))
			((Real,betas,0.0,,"Fraction of the viscous damping coefficient (shear direction) equal to $\\frac{c_{s}}{C_{s,crit}}$."))

			// Contact damping coefficient for non-linear elastic contact law (of Hertz-Mindlin type)
			((Real,alpha,0.0,,"Constant coefficient to define contact viscous damping for non-linear elastic force-displacement relationship."))
			
			// temporary
			((Vector3r,prevU,Vector3r::Zero(),,"Previous local displacement; only used with :yref:`Law2_L3Geom_FrictPhys_HertzMindlin`."))
			((Vector2r,Fs,Vector2r::Zero(),,"Shear force in local axes (computed incrementally)"))
			,
			createIndex());
	REGISTER_CLASS_INDEX(MindlinPhys,FrictPhys);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(MindlinPhys);



/******************** Ip2_FrictMat_FrictMat_MindlinPhys *******/
class Ip2_FrictMat_FrictMat_MindlinPhys: public IPhysFunctor{
	public :
	virtual void go(const shared_ptr<Material>& b1,	const shared_ptr<Material>& b2,	const shared_ptr<Interaction>& interaction);
	FUNCTOR2D(FrictMat,FrictMat);
	YADE_CLASS_BASE_DOC_ATTRS(Ip2_FrictMat_FrictMat_MindlinPhys,IPhysFunctor,"Calculate some physical parameters needed to obtain the normal and shear stiffnesses according to the Hertz-Mindlin's formulation (as implemented in PFC).\n\nViscous parameters can be specified either using coefficients of restitution ($e_n$, $e_s$) or viscous damping coefficient ($\\beta_n$, $\\beta_s$). The following rules apply:\n#. If the $\\beta_n$ ($\\beta_s$) coefficient is given, it is assigned to :yref:`MindlinPhys.betan` (:yref:`MindlinPhys.betas`) directly.\n#. If $e_n$ is given, :yref:`MindlinPhys.betan` is computed using $\\beta_n=-(\\log e_n)/\\sqrt{\\pi^2+(\\log e_n)^2}$. The same applies to $e_s$, :yref:`MindlinPhys.betas`.\n#. It is an error (exception) to specify both $e_n$ and $\\beta_n$ ($e_s$ and $\\beta_s$).\n#. If neither $e_n$ nor $\\beta_n$ is given, zero value for :yref:`MindlinPhys.betan` is used; there will be no viscous effects.\n#.If neither $e_s$ nor $\\beta_s$ is given, the value of :yref:`MindlinPhys.betan` is used for :yref:`MindlinPhys.betas` as well.\n\nThe $e_n$, $\\beta_n$, $e_s$, $\\beta_s$ are :yref:`MatchMaker` objects; they can be constructed from float values to always return constant value.\n\nSee :ysrc:`scripts/test/shots.py` for an example of specifying $e_n$ based on combination of parameters.",
			((Real,gamma,0.0,,"Surface energy parameter [J/m^2] per each unit contact surface, to derive DMT formulation from HM"))
			((Real,eta,0.0,,"Coefficient to determine the plastic bending moment"))
			((Real,krot,0.0,,"Rotational stiffness for moment contact law"))
			((Real,ktwist,0.0,,"Torsional stiffness for moment contact law"))
			((shared_ptr<MatchMaker>,en,,,"Normal coefficient of restitution $e_n$."))
			((shared_ptr<MatchMaker>,es,,,"Shear coefficient of restitution $e_s$."))
			((shared_ptr<MatchMaker>,betan,,,"Normal viscous damping coefficient $\\beta_n$."))
			((shared_ptr<MatchMaker>,betas,,,"Shear viscous damping coefficient $\\beta_s$."))
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ip2_FrictMat_FrictMat_MindlinPhys);


class Law2_ScGeom_MindlinPhys_MindlinDeresiewitz: public LawFunctor{
	public:
		virtual void go(shared_ptr<IGeom>&, shared_ptr<IPhys>&, Interaction*);
		FUNCTOR2D(ScGeom,MindlinPhys);
		YADE_CLASS_BASE_DOC_ATTRS(Law2_ScGeom_MindlinPhys_MindlinDeresiewitz,LawFunctor,
			"Hertz-Mindlin contact law with partial slip solution, as described in [Thornton1991]_.",
		);
};
REGISTER_SERIALIZABLE(Law2_ScGeom_MindlinPhys_MindlinDeresiewitz);

class Law2_ScGeom_MindlinPhys_HertzWithLinearShear: public LawFunctor{
	public:
		virtual void go(shared_ptr<IGeom>&, shared_ptr<IPhys>&, Interaction*);
		FUNCTOR2D(ScGeom,MindlinPhys);
		YADE_CLASS_BASE_DOC_ATTRS(Law2_ScGeom_MindlinPhys_HertzWithLinearShear,LawFunctor,
			"Constitutive law for the Hertz formulation (using :yref:`MindlinPhys.kno`) and linear beahvior in shear (using :yref:`MindlinPhys.kso` for stiffness and :yref:`FrictPhys.tangensOfFrictionAngle`). \n\n.. note:: No viscosity or damping. If you need those, look at  :yref:`Law2_ScGeom_MindlinPhys_Mindlin`, which also includes non-linear Mindlin shear.",
				((int,nonLin,0,,"Shear force nonlinearity (the value determines how many features of the non-linearity are taken in account). 1: ks as in HM 2: shearElastic increment computed as in HM 3. granular ratcheting disabled."))
		);
};
REGISTER_SERIALIZABLE(Law2_ScGeom_MindlinPhys_HertzWithLinearShear);


/******************** Law2_ScGeom_MindlinPhys_Mindlin *********/
class Law2_ScGeom_MindlinPhys_Mindlin: public LawFunctor{
		// just for the deprecation warning, can be removed later
		Real _beta_parameters_of_Ip2_FrictMat_FrictMat_MindlinPhys; 
		bool _nothing;
	public:

		virtual void go(shared_ptr<IGeom>& _geom, shared_ptr<IPhys>& _phys, Interaction* I);
		Real normElastEnergy();
		Real adhesionEnergy(); 	
		
		Real getfrictionDissipation();
		Real getshearEnergy();
		Real getnormDampDissip();
		Real getshearDampDissip();
		Real contactsAdhesive();
		Real ratioSlidingContacts();

		FUNCTOR2D(ScGeom,MindlinPhys);
		YADE_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(Law2_ScGeom_MindlinPhys_Mindlin,LawFunctor,"Constitutive law for the Hertz-Mindlin formulation. It includes non linear elasticity in the normal direction as predicted by Hertz for two non-conforming elastic contact bodies. In the shear direction, instead, it reseambles the simplified case without slip discussed in Mindlin's paper, where a linear relationship between shear force and tangential displacement is provided. Finally, the Mohr-Coulomb criterion is employed to established the maximum friction force which can be developed at the contact. Moreover, it is also possible to include the effect of linear viscous damping through the definition of the parameters $\\beta_{n}$ and $\\beta_{s}$.",
			((bool,preventGranularRatcheting,true,,"bool to avoid granular ratcheting"))
			((bool,includeAdhesion,false,,"bool to include the adhesion force following the DMT formulation. If true, also the normal elastic energy takes into account the adhesion effect."))
			((bool,calcEnergy,false,,"bool to calculate energy terms (shear potential energy, dissipation of energy due to friction and dissipation of energy due to normal and tangential damping)"))
			((bool,includeMoment,false,,"bool to consider rolling resistance (if :yref:`Ip2_FrictMat_FrictMat_MindlinPhys::eta` is 0.0, no plastic condition is applied.)"))
			//((bool,LinDamp,true,,"bool to activate linear viscous damping (if false, en and es have to be defined in place of betan and betas)"))

			((OpenMPAccumulator<Real>,frictionDissipation,,Attr::noSave,"Energy dissipation due to sliding"))		
			((OpenMPAccumulator<Real>,shearEnergy,,Attr::noSave,"Shear elastic potential energy"))
			((OpenMPAccumulator<Real>,normDampDissip,,Attr::noSave,"Energy dissipated by normal damping"))
			((OpenMPAccumulator<Real>,shearDampDissip,,Attr::noSave,"Energy dissipated by tangential damping"))						
			
			, /*deprec*/
			((betan,_beta_parameters_of_Ip2_FrictMat_FrictMat_MindlinPhys,"!Moved to MindlinPhys, where the value is assigned by the appropriate Ip2 functor."))
			((betas,_beta_parameters_of_Ip2_FrictMat_FrictMat_MindlinPhys,"!Moved to MindlinPhys, where the value is assigned by the appropriate Ip2 functor."))
			((useDamping,_nothing,"Damping is now turned on automatically if either of MindlinPhys.betan or MindlinPhys.betas or MindlinPhys.alpha are non-zero."))
			, /* init */
			, /* ctor */
			, /* py */
			.def("contactsAdhesive",&Law2_ScGeom_MindlinPhys_Mindlin::contactsAdhesive,"Compute total number of adhesive contacts.")
			.def("ratioSlidingContacts",&Law2_ScGeom_MindlinPhys_Mindlin::ratioSlidingContacts,"Return the ratio between the number of contacts sliding to the total number at a given time.")
			.def("normElastEnergy",&Law2_ScGeom_MindlinPhys_Mindlin::normElastEnergy,"Compute normal elastic potential energy. It handles the DMT formulation if :yref:`Law2_ScGeom_MindlinPhys_Mindlin::includeAdhesion` is set to true.")	
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Law2_ScGeom_MindlinPhys_Mindlin);









