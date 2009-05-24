// 2008 © Václav Šmilauer <eudoxos@arcig.cz> 
#pragma once
#include<yade/extra/Shop.hpp>

#include<yade/core/InteractionSolver.hpp>
#include<yade/core/FileGenerator.hpp>
#include<yade/pkg-common/RigidBodyParameters.hpp>
#include<yade/pkg-dem/BodyMacroParameters.hpp>
#include<yade/pkg-common/InteractionPhysicsEngineUnit.hpp>
#include<yade/pkg-dem/SpheresContactGeometry.hpp>
#include<yade/pkg-common/GLDrawFunctors.hpp>
#include<yade/pkg-common/PeriodicEngines.hpp>
#include<yade/pkg-common/NormalShearInteractions.hpp>
#include<yade/pkg-common/ConstitutiveLaw.hpp>


/* Engine encompassing several computations looping over all bodies/interactions
 *
 * * Compute and store unbalanced force over the whole simulation.
 * * Compute and store volumetric strain for every body.
 *
 * May be extended in the future to compute global stiffness etc as well.
 */
class BrefcomGlobalCharacteristics: public PeriodicEngine{
	public:
		bool useMaxForce; // use maximum unbalanced force instead of mean unbalanced force
		Real unbalancedForce;
		void compute(MetaBody* rb, bool useMax=false);
		virtual void action(MetaBody* rb){compute(rb,useMaxForce);}
		BrefcomGlobalCharacteristics(){};
	REGISTER_ATTRIBUTES(PeriodicEngine,
		(unbalancedForce)
		(useMaxForce)
	);
	DECLARE_LOGGER;
	REGISTER_CLASS_AND_BASE(BrefcomGlobalCharacteristics,PeriodicEngine);
};
REGISTER_SERIALIZABLE(BrefcomGlobalCharacteristics);

/*! @brief representation of a single interaction of the brefcom type: storage for relevant parameters.
 *
 * Evolution of the contact is governed by BrefcomLaw:
 * that includes damage effects and chages of parameters inside BrefcomContact.
 *
 */
class BrefcomContact: public NormalShearInteraction {
	private:
	public:
		/*! Fundamental parameters (constants) */
		Real
			//! normal modulus (stiffness / crossSection)
			E,
			//! shear modulus
			G,
			//! tangens of internal friction angle
			tanFrictionAngle, 
			//! virgin material cohesion
			undamagedCohesion,
			//! equivalent cross-section associated with this contact
			crossSection,
			//! strain at which the material starts to behave non-linearly
			epsCrackOnset,
			//! strain where the damage-evolution law tangent from the top (epsCrackOnset) touches the axis;
			/// since the softening law is exponential, this doesn't mean that the contact is fully damaged at this point,
			/// that happens only asymptotically 
			epsFracture,
			//! damage after which the contact disappears (<1), since omega reaches 1 only for strain →+∞
			omegaThreshold,
			//! weight coefficient for shear strain when computing the strain semi-norm kappaD
			xiShear,
			//! characteristic time for damage (if non-positive, the law without rate-dependence is used)
			dmgTau,
			//! exponent in the rate-dependent damage evolution
			dmgRateExp,
			//! damage strain (at previous or current step)
			dmgStrain,
			//! damage viscous overstress (at previous step or at current step)
			dmgOverstress,
			//! characteristic time for viscoplasticity (if non-positive, no rate-dependence for shear)
			plTau,
			//! exponent in the rate-dependent viscoplasticity
			plRateExp,
			//! "prestress" of this link (used to simulate isotropic stress)
			isoPrestress;
		/*! Up to now maximum normal strain (semi-norm), non-decreasing in time. */
		Real kappaD;
		/*! Transversal strain (perpendicular to the contact axis) */
		Real epsTrans;
		/*! if not cohesive, interaction is deleted when distance is greater than zero. */
		bool isCohesive;
		/*! the damage evlution function will always return virgin state */
		bool neverDamage;
		/*! cummulative plastic strain measure (scalar) on this contact */
		Real epsPlSum;
		//! debugging, to see convergence rate
		static long cummBetaIter, cummBetaCount;

		/*! auxiliary variable for visualization, recalculated in BrefcomLaw at every iteration */
		// FIXME: Fn and Fs are stored as Vector3r normalForce, shearForce in NormalShearInteraction 
		Real omega, Fn, sigmaN, epsN, relResidualStrength; Vector3r epsT, sigmaT, Fs;


		static Real solveBeta(const Real c, const Real N);
		Real computeDmgOverstress(Real dt);
		Real computeViscoplScalingFactor(Real sigmaTNorm, Real sigmaTYield,Real dt);



		BrefcomContact(): NormalShearInteraction(),E(0), G(0), tanFrictionAngle(0), undamagedCohesion(0), crossSection(0), xiShear(0), dmgTau(-1), dmgRateExp(0), dmgStrain(0), plTau(-1), plRateExp(0), isoPrestress(0.), kappaD(0.), epsTrans(0.), epsPlSum(0.) { createIndex(); epsT=Vector3r::ZERO; isCohesive=false; neverDamage=false; omega=0; Fn=0; Fs=Vector3r::ZERO; epsPlSum=0; dmgOverstress=0; }
		virtual ~BrefcomContact();

		REGISTER_ATTRIBUTES(NormalShearInteraction,
			(E)
			(G)
			(tanFrictionAngle)
			(undamagedCohesion)
			(crossSection)
			(epsCrackOnset)
			(epsFracture)
			(omegaThreshold)
			(xiShear)
			(dmgTau)
			(dmgRateExp)
			(dmgStrain)
			(dmgOverstress)
			(plTau)
			(plRateExp)
			(isoPrestress)

			(cummBetaIter)
			(cummBetaCount)

			(kappaD)
			(neverDamage)
			(epsT)
			(epsTrans)
			(epsPlSum)

			(isCohesive)

			// auxiliary params to make them accessible from python
			(omega)
			(Fn)
			(Fs)
			(epsN)
			(sigmaN)
			(sigmaT)
			(relResidualStrength)
		);
	REGISTER_CLASS_AND_BASE(BrefcomContact,NormalShearInteraction);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(BrefcomContact);

/* This class holds information associated with each body */
class BrefcomPhysParams: public BodyMacroParameters {
	public:
		//! volumetric strain around this body
		Real epsVolumetric;
		//! number of (cohesive) contacts that damaged completely
		int numBrokenCohesive;
		//! number of contacts with this body
		int numContacts;
		//! average damage including already deleted contacts (it is really not damage, but 1-relResidualStrength now)
		Real normDmg;
		//! plastic strain on contacts already deleted
		Real epsPlBroken;
		//! sum of plastic strains normalized by number of contacts
		Real normEpsPl;
		BrefcomPhysParams(): epsVolumetric(0.), numBrokenCohesive(0), numContacts(0), normDmg(0.), epsPlBroken(0.), normEpsPl(0.) {createIndex();};
		REGISTER_ATTRIBUTES(BodyMacroParameters, (epsVolumetric) (numBrokenCohesive) (numContacts) (normDmg) (epsPlBroken) (normEpsPl));
		REGISTER_CLASS_AND_BASE(BrefcomPhysParams,BodyMacroParameters);
};
REGISTER_SERIALIZABLE(BrefcomPhysParams);

class ef2_Spheres_Brefcom_BrefcomLaw: public ConstitutiveLaw{
	public:
	/*! Damage evolution law */
	static Real funcG(const Real& kappaD, const Real& epsCrackOnset, const Real& epsFracture, const bool& neverDamage) {
		if(kappaD<epsCrackOnset || neverDamage) return 0;
		return 1.-(epsCrackOnset/kappaD)*exp(-(kappaD-epsCrackOnset)/epsFracture);
	}
		bool logStrain;
		//! yield function: 0: mohr-coulomb (original); 1: parabolic; 2: logarithmic, 3: log+lin_tension, 4: elliptic, 5: elliptic+log
		int yieldSurfType;
		//! scaling in the logarithmic yield surface (should be <1 for realistic results; >=0 for meaningful results)
		static Real yieldLogSpeed;
		static Real yieldEllipseShift;
		//! HACK: limit strain on some contacts by moving body #2 in the contact; only if refR1<0 (facet); deactivated if > 0
		static Real minStrain_moveBody2;
		ef2_Spheres_Brefcom_BrefcomLaw(): logStrain(false), yieldSurfType(0) { /*timingDeltas=shared_ptr<TimingDeltas>(new TimingDeltas);*/ }
		void go(shared_ptr<InteractionGeometry>& _geom, shared_ptr<InteractionPhysics>& _phys, Interaction* I, MetaBody* rootBody);
	FUNCTOR2D(Dem3DofGeom,BrefcomContact);
	REGISTER_CLASS_AND_BASE(ef2_Spheres_Brefcom_BrefcomLaw,ConstitutiveLaw);
	REGISTER_ATTRIBUTES(ConstitutiveLaw,(logStrain)(yieldSurfType)(yieldLogSpeed)(yieldEllipseShift)(minStrain_moveBody2));
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(ef2_Spheres_Brefcom_BrefcomLaw);

// obsolete
#if 0
class BrefcomLaw: public InteractionSolver{
	private:
		shared_ptr<ef2_Spheres_Brefcom_BrefcomLaw> functor;
		shared_ptr<BrefcomContact> BC;
		shared_ptr<SpheresContactGeometry> contGeom;
		MetaBody* rootBody;
		//! aplly calculated force on both particles (applied in the inverse sense on B)
		void applyForce(const Vector3r&, const body_id_t&, const body_id_t&);
		/*! Damage evolution law */
		Real funcG(const Real& kappaD, const Real& epsCrackOnset, const Real& epsFracture, const bool& neverDamage) const{ return ef2_Spheres_Brefcom_BrefcomLaw::funcG(kappaD,epsCrackOnset,epsFracture,neverDamage); }
		
	public:
		bool logStrain;
		BrefcomLaw(): logStrain(false) { };
		void action(MetaBody*);
	protected: 
	REGISTER_CLASS_AND_BASE(BrefcomLaw,InteractionSolver);
	REGISTER_ATTRIBUTES(InteractionSolver,(logStrain));
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(BrefcomLaw);
#endif 
/*! @brief Convert macroscopic properties to BrefcomContact with corresponding parameters.
 *
 * */
class BrefcomMakeContact: public InteractionPhysicsEngineUnit{
	private:
	public:
		/* nonelastic material parameters */
		/* alternatively (and more cleanly), we would have subclass of ElasticBodyParameters,
		 * which would define just those in addition to the elastic ones.
		 * This might be done later, for now hardcode that here. */
		/* uniaxial tension resistance, bending parameter of the damage evolution law, whear weighting constant for epsT in the strain seminorm (kappa) calculation. Default to NaN so that user gets loudly notified it was not set.
		
		*/
		Real sigmaT, epsCrackOnset, relDuctility, G_over_E, tau, expDmgRate, omegaThreshold, dmgTau, dmgRateExp, plTau, plRateExp, isoPrestress;
		//! Should new contacts be cohesive? They will before this iter#, they will not be afterwards. If 0, they will never be. If negative, they will always be created as cohesive.
		long cohesiveThresholdIter;
		//! Create contacts that don't receive any damage (BrefcomContact::neverDamage=true); defaults to false
		bool neverDamage;

		BrefcomMakeContact(){
			// init to signaling_NaN to force crash if not initialized (better than unknowingly using garbage values)
			sigmaT=epsCrackOnset=relDuctility=G_over_E=std::numeric_limits<Real>::signaling_NaN();
			neverDamage=false;
			cohesiveThresholdIter=-1;
			dmgTau=-1; dmgRateExp=0; plTau=-1; plRateExp=-1;
			omegaThreshold=0.999;
			isoPrestress=0;
		}

		virtual void go(const shared_ptr<PhysicalParameters>& pp1, const shared_ptr<PhysicalParameters>& pp2, const shared_ptr<Interaction>& interaction);
		REGISTER_ATTRIBUTES(InteractionPhysicsEngineUnit,
			(cohesiveThresholdIter)
			(G_over_E)
			(sigmaT)
			(neverDamage)
			(epsCrackOnset)
			(relDuctility)
			(dmgTau)
			(dmgRateExp)
			(plTau)
			(plRateExp)
			(omegaThreshold)
			(isoPrestress)
		);

		FUNCTOR2D(BrefcomPhysParams,BrefcomPhysParams);
		REGISTER_CLASS_AND_BASE(BrefcomMakeContact,InteractionPhysicsEngineUnit);
		DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(BrefcomMakeContact);

class GLDrawBrefcomContact: public GLDrawInteractionPhysicsFunctor {
	public: virtual void go(const shared_ptr<InteractionPhysics>&,const shared_ptr<Interaction>&,const shared_ptr<Body>&,const shared_ptr<Body>&,bool wireFrame);
	virtual ~GLDrawBrefcomContact() {};
	REGISTER_ATTRIBUTES(/*no base*/,(contactLine)(dmgLabel)(dmgPlane)(epsT)(epsTAxes)(normal)(colorStrain)(epsNLabel));
	RENDERS(BrefcomContact);
	REGISTER_CLASS_AND_BASE(GLDrawBrefcomContact,GLDrawInteractionPhysicsFunctor);
	DECLARE_LOGGER;
	static bool contactLine,dmgLabel,dmgPlane,epsT,epsTAxes,normal,colorStrain,epsNLabel;
};
REGISTER_SERIALIZABLE(GLDrawBrefcomContact);

class BrefcomDamageColorizer: public PeriodicEngine {
	public:
		//! maximum damage over all contacts
		Real maxOmega;
		BrefcomDamageColorizer(){maxOmega=0; /* run at the very beginning */ initRun=true;}
		virtual void action(MetaBody*);
	REGISTER_ATTRIBUTES(PeriodicEngine,(maxOmega));
	REGISTER_CLASS_AND_BASE(BrefcomDamageColorizer,PeriodicEngine);
};
REGISTER_SERIALIZABLE(BrefcomDamageColorizer);

