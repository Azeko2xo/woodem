// 2008 © Václav Šmilauer <eudoxos@arcig.cz> 

/*

=== HIGH LEVEL OVERVIEW OF CPM ===

Concrete Particle Model (ConcretePM, Cpm) is a set of classes for modelling
mechanical behavior of concrete. Several classes are needed for Cpm.

1. CpmMat (Cpm material) deriving from BodyMacroParameters, which additionally has
   some information about damage on the body, cummulative plastic strain etc.

2.	Ip2_CpmMat_CpmMat_CpmPhys is 2-ary functor for creating CpmPhys from CpmMat's of
	2 bodies that collide. Some parameters of the CpmPhys created are computed from
	CpmMat's, others are passed as parameters of the functor.

3. CpmPhys (Cpm (interaction)Physics) holds various parameters as well as internal
   variables of the contact that can change as result of plasticity, damage, viscosity.

4. Law2_Dem3Dof_CpmPhys_Cpm is constitutive law that takes geometry of the interaction
	(Dem3Dof, which can be either Dem3Dof_SphereSphere or Dem3Dof_FacetSphere) and
	CpmPhys, computing forces on both bodies and updating contact variables.

	The model itself is defined in the macro CPM_MATERIAL_MODEL, but due to 
	commercial reasons, those about 30 lines of code cannot be disclosed now and the macro
	is defined in an external file. The model will be, however, described in enough detail
	in my thesis (once it is written), along
	with calibration procedures; it features damage, plasticity and viscosity
	and is quite tunable (rigidity, poisson's	ratio, compressive/tensile strength
	ratio, fracture energy, behavior under confinement, rate-dependence).

There are other classes, which are not strictly necessary:

 * CpmGlobalCharacteristics computes a few information about individual bodies based on
   interactions they are involved in. It is probably quite useless now since volumetricStrain
	is not used in the constitutive law anymore.

 * Gl1_CpmPhys draws interaction physics (color for damage and a few other); rarely used, though.

 * CpmStateUpdater changes bodies' colors depending on average damage of their interactions
   and number of interactions that were already fully broken and have disappeared. This engine
	contains its own loop (2 loops, more precisely) over all bodies and is run periodically
	to update colors.

*/

#pragma once

#include<yade/pkg-common/ElastMat.hpp>
#include<yade/pkg-common/InteractionPhysicsFunctor.hpp>
#include<yade/pkg-dem/ScGeom.hpp>
#include<yade/pkg-common/PeriodicEngines.hpp>
#include<yade/pkg-common/NormShearPhys.hpp>
#include<yade/pkg-common/LawFunctor.hpp>

namespace py=boost::python;

/* Cpm state information about each body.
None of that is used for computation (at least not now), only for post-processing.
*/
class CpmState: public State {
	YADE_CLASS_BASE_DOC_ATTRS(CpmState,State,"State information about body use by :ref:`cpm-model`.\n\nNone of that is used for computation (at least not now), only for post-processing.",
		((Real,epsVolumetric,0,"Volumetric strain around this body (unused for now)"))
		((int,numBrokenCohesive,0,"Number of (cohesive) contacts that damaged completely"))
		((int,numContacts,0,"Number of contacts with this body"))
		((Real,normDmg,0,"Average damage including already deleted contacts (it is really not damage, but 1-relResidualStrength now)"))
		((Real,epsPlBroken,0,"Plastic strain on contacts already deleted (bogus values)"))
		((Real,normEpsPl,0,"Sum of plastic strains normalized by number of contacts (bogus values)"))
		((Vector3r,sigma,Vector3r::Zero(),"Normal stresses on the particle"))
		((Vector3r,tau,Vector3r::Zero(),"Shear stresses on the particle."))
	);
};
REGISTER_SERIALIZABLE(CpmState);

/* This class holds information associated with each body */
class CpmMat: public FrictMat {
	public:
		virtual shared_ptr<State> newAssocState() const { return shared_ptr<State>(new CpmState); }
		virtual bool stateTypeOk(State* s) const { return (bool)dynamic_cast<CpmState*>(s); }

	YADE_CLASS_BASE_DOC_ATTRS_CTOR(CpmMat,FrictMat,"Concrete material, for use with other Cpm classes. \n\n.. note::\n\t:yref:`Density<Material::density>` is initialized to 4800 kgm⁻³automatically, which gives approximate 2800 kgm⁻³ on 0.5 density packing.\n\n.. _cpm-model::\n\n Concrete Particle Model (CPM)\n\n\n :yref:`CpmMat` is particle material, :yref:`Ip2_CpmMat_CpmMat_CpmPhys` averages two particles' materials, creating :yref:`CpmPhys`, which is then used in interaction resultion by :yref:`Law2_Dem3DofGeom_CpmPhys_Cpm`. :yref:`CpmState` is associated to :yref:`CpmMat` and keeps state defined on particles rather than interactions (such as number of completely damaged interactions).\n\nThe model is contained in externally defined macro CPM_MATERIAL_MODEL, which features damage in tension, plasticity in shear and compression and rate-dependence. For commercial reasons, rate-dependence and compression-plasticity is not present in reduced version of the model, used when CPM_MATERIAL_MODEL is not defined. The full model will be described in detail in my (Václav Šmilauer) thesis along with calibration procedures (rigidity, poisson's ratio, compressive/tensile strength ratio, fracture energy, behavior under confinement, rate-dependent behavior).\n\nEven the public model is useful enough to run simulation on concrete samples, such as :ysrc:`uniaxial tension-compression test<examples/concrete/uniax.py>`.",
		((Real,G_over_E,NaN,"Ratio of normal/shear stiffness at interaction level [-]"))
		((Real,sigmaT,NaN,"Initial cohesion [Pa]"))
		((bool,neverDamage,false,"If true, no damage will occur (for testing only)."))
		((Real,epsCrackOnset,NaN,"Limit elastic strain [-]"))
		((Real,relDuctility,NaN,"Relative ductility, for damage evolution law peak right-tangent. [-]"))
		((Real,dmgTau,((void)"deactivated if negative",-1),"Characteristic time for normal viscosity. [s]"))
		((Real,dmgRateExp,0,"Exponent for normal viscosity function. [-]"))
		((Real,plTau,((void)"deactivated if negative",-1),"Characteristic time for visco-plasticity. [s]"))
		((Real,plRateExp,0,"Exponent for visco-plasticity function. [-]"))
		((Real,isoPrestress,0,"Isotropic prestress of the whole specimen. [Pa]")),
		createIndex(); density=4800;
	);
	REGISTER_CLASS_INDEX(CpmMat,FrictMat);
};
REGISTER_SERIALIZABLE(CpmMat);


/*! @brief representation of a single interaction of the CPM type: storage for relevant parameters.
 *
 * Evolution of the contact is governed by Law2_Dem3DofGeom_CpmPhys_Cpm:
 * that includes damage effects and chages of parameters inside CpmPhys.
 *
 */
class CpmPhys: public NormShearPhys {
	public:
		static long cummBetaIter, cummBetaCount;
		/*! auxiliary variable for visualization, recalculated in Law2_Dem3DofGeom_CpmPhys_Cpm at every iteration */
		// Fn and Fs are also stored as Vector3r normalForce, shearForce in NormShearPhys 
		Real omega, Fn, sigmaN, epsN, relResidualStrength; Vector3r epsT, sigmaT, Fs;

		static Real solveBeta(const Real c, const Real N);
		Real computeDmgOverstress(Real dt);
		Real computeViscoplScalingFactor(Real sigmaTNorm, Real sigmaTYield,Real dt);

		virtual ~CpmPhys();
		YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CpmPhys,NormShearPhys,"Representation of a single interaction of the Cpm type: storage for relevant parameters.\n\n Evolution of the contact is governed by :yref:`Law2_Dem3DofGeom_CpmPhys_Cpm`, that includes damage effects and chages of parameters inside CpmPhys. See :ref:`cpm-model` for details.",
			((Real,E,NaN,"normal modulus (stiffness / crossSection) [Pa]"))
			((Real,G,NaN,"shear modulus [Pa]"))
			((Real,tanFrictionAngle,NaN,"tangens of internal friction angle [-]"))
			((Real,undamagedCohesion,NaN,"virgin material cohesion [Pa]"))
			((Real,crossSection,NaN,"equivalent cross-section associated with this contact [m²]"))
			((Real,epsCrackOnset,NaN,"strain at which the material starts to behave non-linearly"))
			((Real,epsFracture,NaN,"strain where the damage-evolution law tangent from the top (epsCrackOnset) touches the axis; since the softening law is exponential, this doesn't mean that the contact is fully damaged at this point, that happens only asymptotically"))
			((Real,dmgTau,-1,"characteristic time for damage (if non-positive, the law without rate-dependence is used)"))
			((Real,dmgRateExp,0,"exponent in the rate-dependent damage evolution"))
			((Real,dmgStrain,0,"damage strain (at previous or current step)"))
			((Real,dmgOverstress,0,"damage viscous overstress (at previous step or at current step)"))
			((Real,plTau,-1,"characteristic time for viscoplasticity (if non-positive, no rate-dependence for shear)"))
			((Real,plRateExp,0,"exponent in the rate-dependent viscoplasticity"))
			((Real,isoPrestress,0,"\"prestress\" of this link (used to simulate isotropic stress)"))
			((Real,kappaD,0,"Up to now maximum normal strain (semi-norm), non-decreasing in time."))
			((Real,epsNPl,0,"normal plastic strain (initially zero)"))
			((bool,neverDamage,false,"the damage evolution function will always return virgin state"))
			((Real,epsTrans,0,"Transversal strain (perpendicular to the contact axis)"))
			((Real,epsPlSum,0,"cummulative shear plastic strain measure (scalar) on this contact"))
			((bool,isCohesive,false,"if not cohesive, interaction is deleted when distance is greater than zero."))
			,
			createIndex(); epsT=Fs=Vector3r::Zero(); Fn=0; omega=0;
			,
			.def_readonly("omega",&CpmPhys::omega,"Damage internal variable")
			.def_readonly("epsT",&CpmPhys::epsT,"Transversal strain (not used)")
			.def_readonly("Fn",&CpmPhys::Fn,"Magnitude of normal force.")
			.def_readonly("Fs",&CpmPhys::Fs,"Magnitude of shear force")
			.def_readonly("epsN",&CpmPhys::epsN,"Current normal strain")
			.def_readonly("sigmaN",&CpmPhys::sigmaN,"Current normal stress")
			.def_readonly("sigmaT",&CpmPhys::sigmaT,"Current shear stress")
			.def_readonly("relResidualStrength",&CpmPhys::relResidualStrength,"Relative residual strength")
			.def_readonly("cummBetaIter",&CpmPhys::cummBetaIter,"Cummulative number of iterations inside CpmMat::solveBeta (for debugging).")
			.def_readonly("cummBetaCount",&CpmPhys::cummBetaCount,"Cummulative number of calls of CpmMat::solveBeta (for debugging).")
		);
	DECLARE_LOGGER;
	REGISTER_CLASS_INDEX(CpmPhys,NormShearPhys);
};
REGISTER_SERIALIZABLE(CpmPhys);


/*! @brief Convert macroscopic properties to CpmPhys with corresponding parameters.
 *
 * */
class Ip2_CpmMat_CpmMat_CpmPhys: public InteractionPhysicsFunctor{
	public:
		virtual void go(const shared_ptr<Material>& pp1, const shared_ptr<Material>& pp2, const shared_ptr<Interaction>& interaction);
		FUNCTOR2D(CpmMat,CpmMat);
		DECLARE_LOGGER;
		YADE_CLASS_BASE_DOC_ATTRS(Ip2_CpmMat_CpmMat_CpmPhys,InteractionPhysicsFunctor,"Convert 2 :yref:`CpmMat` instances to :yref:`CpmPhys` with corresponding parameters. Uses simple (arithmetic) averages if material are different. Simple copy of parameters is performed if the :yref:`material<CpmMat>` is shared between both particles. See :ref:`cpm-model` for detals.",
			((long,cohesiveThresholdIter,10,"Should new contacts be cohesive? They will before this iter#, they will not be afterwards. If 0, they will never be. If negative, they will always be created as cohesive (10 by default)."))
		);
};
REGISTER_SERIALIZABLE(Ip2_CpmMat_CpmMat_CpmPhys);



class Law2_Dem3DofGeom_CpmPhys_Cpm: public LawFunctor{
	public:
	/*! Damage evolution law */
	Real funcG(const Real& kappaD, const Real& epsCrackOnset, const Real& epsFracture, const bool& neverDamage) {
		if(kappaD<epsCrackOnset || neverDamage) return 0;
		return 1.-(epsCrackOnset/kappaD)*exp(-(kappaD-epsCrackOnset)/epsFracture);
	}
	//! return |sigmaT| at plastic surface for given sigmaN etc; not used by the law itself
	Real yieldSigmaTMagnitude(Real sigmaN, Real omega, Real undamagedCohesion, Real tanFrictionAngle);

	void go(shared_ptr<InteractionGeometry>& _geom, shared_ptr<InteractionPhysics>& _phys, Interaction* I, Scene* rootBody);

	FUNCTOR2D(Dem3DofGeom,CpmPhys);
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Law2_Dem3DofGeom_CpmPhys_Cpm,LawFunctor,"Constitutive law for the :ref:`cpm-model`.",
		((int,yieldSurfType,2,"yield function: 0: mohr-coulomb (original); 1: parabolic; 2: logarithmic, 3: log+lin_tension, 4: elliptic, 5: elliptic+log"))
		((Real,yieldLogSpeed,.1,"scaling in the logarithmic yield surface (should be <1 for realistic results; >=0 for meaningful results)"))
		((Real,yieldEllipseShift,NaN,"horizontal scaling of the ellipse (shifts on the +x axis as interactions with +y are given)"))
		((Real,omegaThreshold,((void)">=1. to deactivate, i.e. never delete any contacts",1.),"damage after which the contact disappears (<1), since omega reaches 1 only for strain →+∞"))
		((Real,epsSoft,((void)"approximates confinement -20MPa precisely, -100MPa a little over, -200 and -400 are OK (secant)",-3e-3),"Strain at which softening in compression starts (non-negative to deactivate)"))
		((Real,relKnSoft,.3,"Relative rigidity of the softening branch in compression (0=perfect elastic-plastic, <0 softening, >0 hardening)")),
		/*ctor*/,
		.def("funcG",&Law2_Dem3DofGeom_CpmPhys_Cpm::funcG,(py::arg("kappaD"),py::arg("epsCrackOnset"),py::arg("epsFracture"),py::arg("neverDamage")=false),"Damage evolution law, evaluating the $\\omega$ parameter. $\\kappa_D$ is historically maximum strain, *epsCrackOnset* ($\\varepsilon_0$) = :yref:`CpmMat.epsCrackOnset`, *epsFracture* = :yref:`CpmMat.epsFracture`; if *neverDamage* is ``True``, the value returned will always be 0 (no damage).")
		.def("yieldSigmaTMagnitude",&Law2_Dem3DofGeom_CpmPhys_Cpm::yieldSigmaTMagnitude,(py::arg("sigmaN"),py::arg("omega"),py::arg("undamagedCohesion"),py::arg("tanFrictionAngle")),"Return radius of yield surface for given material and state parameters; uses attributes of the current instance (*yieldSurfType* etc), change them before calling if you need that.")
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Law2_Dem3DofGeom_CpmPhys_Cpm);

#ifdef YADE_OPENGL
	#include<yade/pkg-common/GLDrawFunctors.hpp>
	class Gl1_CpmPhys: public GlInteractionPhysicsFunctor {
		public: virtual void go(const shared_ptr<InteractionPhysics>&,const shared_ptr<Interaction>&,const shared_ptr<Body>&,const shared_ptr<Body>&,bool wireFrame);
		virtual ~Gl1_CpmPhys() {};
		RENDERS(CpmPhys);
		DECLARE_LOGGER;
		YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_CpmPhys,GlInteractionPhysicsFunctor,"Render :yref:`CpmPhys` objects of interactions.",
			((bool,contactLine,true,"Show contact line"))
			((bool,dmgLabel,true,"Numerically show contact damage parameter"))
			((bool,dmgPlane,false,"[what is this?]"))
			((bool,epsT,false,"Show shear strain "))
			((bool,epsTAxes,false,"Show axes of shear plane "))
			((bool,normal,false,"Show contact normal"))
			((bool,colorStrain,false,"Render elements using transverse strain (relative to max elastic strain) [no longer used]"))
			((bool,epsNLabel,false,"Numerically show normal strain"))
		);
	};
	REGISTER_SERIALIZABLE(Gl1_CpmPhys);
#endif

class CpmStateUpdater: public PeriodicEngine {
	struct BodyStats{ int nCohLinks; int nLinks; Real dmgSum, epsPlSum; Vector3r sigma, tau; BodyStats(): nCohLinks(0), nLinks(0), dmgSum(0.), epsPlSum(0.), sigma(Vector3r::Zero()), tau(Vector3r::Zero()) {} };
	public:
		virtual void action(){ update(scene); }
		void update(Scene* rb=NULL);
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(CpmStateUpdater,PeriodicEngine,"Update :yref:`CpmState` of bodies based on state variables in :yref:`CpmPhys` of interactions with this bod. In particular, bodies' colors and :yref:`CpmState::normDmg` depending on average :yref:`damage<CpmPhys::omega>` of their interactions and number of interactions that were already fully broken and have disappeared is updated. This engine contains its own loop (2 loops, more precisely) over all bodies and should be run periodically to update colors during the simulation, if desired.",
		((Real,avgRelResidual,NaN,"Average residual strength at last run."))
		((Real,maxOmega,NaN,"Globally maximum damage parameter at last run.")),
		initRun=true;
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(CpmStateUpdater);

