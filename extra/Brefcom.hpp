// 2008 © Václav Šmilauer <eudoxos@arcig.cz> 
#pragma once
#include<yade/extra/Shop.hpp>

#include<yade/core/InteractionSolver.hpp>
#include<yade/core/FileGenerator.hpp>
#include<yade/pkg-common/RigidBodyParameters.hpp>
#include<yade/pkg-dem/BodyMacroParameters.hpp>
#include<yade/pkg-common/Force.hpp>
#include<yade/pkg-common/Momentum.hpp>
#include<yade/pkg-common/InteractionPhysicsEngineUnit.hpp>
#include<yade/pkg-dem/SpheresContactGeometry.hpp>
#include<yade/pkg-common/GLDrawFunctors.hpp>
#include<yade/pkg-common/PeriodicEngines.hpp>
#include<yade/pkg-common/NormalShearInteractions.hpp>
#include<yade/pkg-dem/GlobalStiffness.hpp>

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
			//! characteristic time (if non-positive, the law without rate-dependence is used)
			tau,
			//! exponent in the rate-dependent damage evolution
			expDmgRate,
			//! coefficient that takes transversal strain into accound when calculating kappaDReduced
			transStrainCoeff;
		/*! Up to now maximum normal strain (semi-norm), non-decreasing in time. */
		Real kappaD;
		/*! Transversal strain (perpendicular to the contact axis) */
		Real epsTrans;
		/*! if not cohesive, interaction is deleted when distance is greater than zero. */
		bool isCohesive;
		/*! the damage evlution function will always return virgin state */
		bool neverDamage;

		/*! auxiliary variable for visualization, recalculated in BrefcomLaw at every iteration */
		// FIXME: Fn and Fs are stored as Vector3r normalForce, shearForce in NormalShearInteraction 
		Real omega, Fn, sigmaN, epsN, relResidualStrength; Vector3r epsT, sigmaT, Fs;

		BrefcomContact(): NormalShearInteraction(),E(0), G(0), tanFrictionAngle(0), undamagedCohesion(0), crossSection(0), xiShear(0), tau(0), expDmgRate(0) { createIndex(); epsT=Vector3r::ZERO; kappaD=0; isCohesive=false; neverDamage=false; omega=0; Fn=0; Fs=Vector3r::ZERO; }
		//	BrefcomContact(Real _E, Real _G, Real _tanFrictionAngle, Real _undamagedCohesion, Real _equilibriumDist, Real _crossSection, Real _epsCrackOnset, Real _epsFracture, Real _expBending, Real _xiShear, Real _tau=0, Real _expDmgRate=1): InteractionPhysics(), E(_E), G(_G), tanFrictionAngle(_tanFrictionAngle), undamagedCohesion(_undamagedCohesion), equilibriumDist(_equilibriumDist), crossSection(_crossSection), epsCrackOnset(_epsCrackOnset), epsFracture(_epsFracture), expBending(_expBending), xiShear(_xiShear), tau(_tau), expDmgRate(_expDmgRate) { epsT=Vector3r::ZERO; kappaD=0; isCohesive=false; neverDamage=false; omega=0; Fn=0; Fs=Vector3r::ZERO; /*TRVAR5(epsCrackOnset,epsFracture,Kn,crossSection,equilibriumDist); */ }
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
			(tau)
			(expDmgRate)
			(transStrainCoeff)

			(kappaD)
			(neverDamage)
			(epsT)
			(epsTrans)

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
		//! average damage including already deleted contacts
		Real normDmg;
		BrefcomPhysParams(): epsVolumetric(0.), numBrokenCohesive(0), numContacts(0), normDmg(0.) {createIndex();};
		REGISTER_ATTRIBUTES(BodyMacroParameters, (epsVolumetric) (numBrokenCohesive) (numContacts) (normDmg) );
		REGISTER_CLASS_AND_BASE(BrefcomPhysParams,BodyMacroParameters);
};
REGISTER_SERIALIZABLE(BrefcomPhysParams);

class BrefcomLaw: public InteractionSolver{
	private:
		//! aplly calculated force on both particles (applied in the inverse sense on B)
		shared_ptr<BrefcomContact> BC;
		shared_ptr<SpheresContactGeometry> contGeom;
		MetaBody* rootBody;
		void applyForce(const Vector3r&, const body_id_t&, const body_id_t&);
		/*! Cohesion evolution law (it is 1-funcH here) */
		inline Real funcH(const Real& kappaD) const{ return 1-funcG(kappaD); }
		/*! Damage evolution law */
		inline Real funcG(const Real& kappaD) const{
			const Real& epsCrackOnset=BC->epsCrackOnset, epsFracture=BC->epsFracture; const bool& neverDamage=BC->neverDamage; // shorthands
			if(kappaD<epsCrackOnset || neverDamage) return 0;
			return 1.-(epsCrackOnset/kappaD)*exp(-(kappaD-epsCrackOnset)/epsFracture);
		}
		
	public:
		BrefcomLaw() { Shop::Bex::initCache(); };
		void action(MetaBody*);
	protected: 
	NEEDS_BEX("Force","Momentum");
	REGISTER_CLASS_AND_BASE(BrefcomLaw,InteractionSolver);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(BrefcomLaw);

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
		
		expBending is positive if the damage evolution function is concave after fracture onset;
		reasonable value seems like 4.
		*/
		Real sigmaT, expBending, xiShear, epsCrackOnset, relDuctility, G_over_E, tau, expDmgRate, omegaThreshold, transStrainCoeff;
		//! Should new contacts be cohesive? They will before this iter#, they will not be afterwards. If 0, they will never be. If negative, they will always be created as cohesive.
		long cohesiveThresholdIter;
		//! Create contacts that don't receive any damage (BrefcomContact::neverDamage=true); defaults to false
		bool neverDamage;

		BrefcomMakeContact(){
			// init to signaling_NaN to force crash if not initialized (better than unknowingly using garbage values)
			sigmaT=xiShear=epsCrackOnset=relDuctility=G_over_E=transStrainCoeff=std::numeric_limits<Real>::signaling_NaN();
			neverDamage=false;
			cohesiveThresholdIter=-1;
			tau=-1; expDmgRate=0;
			omegaThreshold=0.98;
		}

		virtual void go(const shared_ptr<PhysicalParameters>& pp1, const shared_ptr<PhysicalParameters>& pp2, const shared_ptr<Interaction>& interaction);
		REGISTER_ATTRIBUTES(InteractionPhysicsEngineUnit,
			(cohesiveThresholdIter)
			(G_over_E)
			(expBending)
			(xiShear)
			(sigmaT)
			(neverDamage)
			(epsCrackOnset)
			(relDuctility)
			(tau)
			(expDmgRate)
			(omegaThreshold)
			(transStrainCoeff)
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
		BrefcomDamageColorizer(){maxOmega=0;}
		virtual void action(MetaBody*);
	REGISTER_ATTRIBUTES(PeriodicEngine,(maxOmega));
	REGISTER_CLASS_AND_BASE(BrefcomDamageColorizer,PeriodicEngine);
};
REGISTER_SERIALIZABLE(BrefcomDamageColorizer);

