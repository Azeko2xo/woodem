#include"RockPM.hpp"
#include<yade/core/MetaBody.hpp>
#include<yade/pkg-dem/BodyMacroParameters.hpp>
#include<yade/pkg-common/Sphere.hpp>
#include<yade/pkg-dem/DemXDofGeom.hpp>
#include<yade/pkg-dem/Shop.hpp>


YADE_PLUGIN((Law2_Dem3DofGeom_RockPMPhys_Rpm)(RpmMat)(Ip2_RpmMat_RpmMat_RpmPhys)(RpmPhys));


/********************** Law2_Dem3DofGeom_RockPMPhys_Rpm ****************************/
CREATE_LOGGER(Law2_Dem3DofGeom_RockPMPhys_Rpm);

void Law2_Dem3DofGeom_RockPMPhys_Rpm::go(shared_ptr<InteractionGeometry>& ig, shared_ptr<InteractionPhysics>& ip, Interaction* contact, MetaBody* rootBody){
	Dem3DofGeom* geom=static_cast<Dem3DofGeom*>(ig.get());
	RpmPhys* phys=static_cast<RpmPhys*>(ip.get());
	
	Real displN=geom->displacementN();
	const Real& crossSection=phys->crossSection;
	
	Real& epsN=phys->epsN;
	Vector3r& epsT=phys->epsT;
	
	Real& sigmaN=phys->sigmaN;
	Vector3r& sigmaT=phys->sigmaT;
	
	const Real& E = phys->E;
	const Real& G = phys->G;
	
	Real& Fn=phys->Fn;
	Vector3r& Fs=phys->Fs;
	

	if(displN<=0){
		/**Normal Interaction*/
		epsN=geom->strainN();
		epsT=geom->strainT();
		
		
		sigmaN=E*epsN;
		Fn=sigmaN*crossSection; phys->normalForce=Fn*geom->normal;
		
		sigmaT=G*epsT;

		/**Check, whether the shear stress more, than normal force multiplicated to tanFrictionAngle*/
		
		Real maxFsSq = phys->normalForce.SquaredLength()*phys->tanFrictionAngle*phys->tanFrictionAngle;
		if(sigmaT.SquaredLength()>maxFsSq) {
			sigmaT*=sqrt(maxFsSq/(sigmaT.SquaredLength()));
		}
		
		Fs=sigmaT*crossSection;
		phys->shearForce = Fs;

		/**Normal Interaction*/
		applyForceAtContactPoint(phys->normalForce + phys->shearForce, geom->contactPoint, contact->getId1(), geom->se31.position, contact->getId2(), geom->se32.position, rootBody);
		
		if ((phys->isCohesive)&&(displN<(-phys->lengthMaxCompression))) {
			phys->isCohesive = false;					///Destruction
		}
		return;
	} else {
		/**If spheres do not touch, check, whether they are cohesive*/
		if (phys->isCohesive) {
			/**If the distance 
			 * between spheres more than critical and they are cohesive,
			 * we delete the interaction
			 * Destruction.
			 **/
			if (displN>(phys->lengthMaxTension)) {
				rootBody->interactions->requestErase(contact->getId1(),contact->getId2());
				return; 
			} else {
			/**If the distance 
			 * between spheres less than critical and they are cohesive,
			 * we aply additional forces to keep particles together.
			 **/
				phys->normalForce=phys->kn*displN*geom->normal;
				applyForceAtContactPoint(phys->normalForce, geom->contactPoint, contact->getId1(), geom->se31.position, contact->getId2(), geom->se32.position, rootBody);
				return;
			}
		} else {
			/**
			 * Delete interactions
			 */ 
			rootBody->interactions->requestErase(contact->getId1(),contact->getId2());
			return;
		}
	}
}


CREATE_LOGGER(Ip2_RpmMat_RpmMat_RpmPhys);

void Ip2_RpmMat_RpmMat_RpmPhys::go(const shared_ptr<PhysicalParameters>& pp1, const shared_ptr<PhysicalParameters>& pp2, const shared_ptr<Interaction>& interaction){
	if(interaction->interactionPhysics) return; 

	Dem3DofGeom* contGeom=YADE_CAST<Dem3DofGeom*>(interaction->interactionGeometry.get());
	Omega& OO=Omega::instance();
	assert(contGeom);
	//LOG_WARN(Omega::instance().getCurrentIteration());
	const shared_ptr<RpmMat>& rpm1=YADE_PTR_CAST<RpmMat>(pp1);
	const shared_ptr<RpmMat>& rpm2=YADE_PTR_CAST<RpmMat>(pp2);
	
	const shared_ptr<BodyMacroParameters>& elast1=static_pointer_cast<BodyMacroParameters>(pp1);
	const shared_ptr<BodyMacroParameters>& elast2=static_pointer_cast<BodyMacroParameters>(pp2);
	long cohesiveThresholdIter=10;
	
	bool initCohesive = rpm1->initCohesive*rpm2->initCohesive;
	
	Real E12=2*elast1->young*elast2->young/(elast1->young+elast2->young);
	Real minRad=(contGeom->refR1<=0?contGeom->refR2:(contGeom->refR2<=0?contGeom->refR1:min(contGeom->refR1,contGeom->refR2)));
	Real S12=Mathr::PI*pow(minRad,2);
	Real G_over_E = (rpm1->G_over_E + rpm2->G_over_E)/2;
	shared_ptr<RpmPhys> contPhys(new RpmPhys());
	contPhys->E=E12;
	contPhys->G=E12*G_over_E;
	contPhys->tanFrictionAngle=tan(.5*(elast1->frictionAngle+elast2->frictionAngle));
	contPhys->crossSection=S12;
	contPhys->kn=contPhys->E*contPhys->crossSection;
	contPhys->ks=contPhys->G*contPhys->crossSection;
	
	contPhys->lengthMaxCompression=S12*(0.5*(rpm1->stressCompressMax+rpm2->stressCompressMax))/(contPhys->kn);
	contPhys->lengthMaxTension=S12*(0.5*(rpm1->stressCompressMax*rpm1->Brittleness+rpm2->stressCompressMax*rpm2->Brittleness))/(contPhys->kn);
	
	initDistance = contGeom->displacementN();

	if ((rpm1->exampleNumber==rpm2->exampleNumber)&&(initDistance<(contPhys->lengthMaxTension))&&(initCohesive)&&(OO.getCurrentIteration()<=cohesiveThresholdIter)) {
		contPhys->isCohesive=true;
	}

	
	interaction->interactionPhysics=contPhys;
}

RpmPhys::~RpmPhys(){};

