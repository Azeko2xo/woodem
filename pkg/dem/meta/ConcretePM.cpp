// 2007,2008 © Václav Šmilauer <eudoxos@arcig.cz> 
#include"ConcretePM.hpp"
#include<yade/core/Scene.hpp>
#include<yade/pkg-dem/DemXDofGeom.hpp>
#include<yade/pkg-dem/Shop.hpp>

YADE_PLUGIN((CpmState)(CpmMat)(Ip2_CpmMat_CpmMat_CpmPhys)(CpmPhys)(Law2_Dem3DofGeom_CpmPhys_Cpm)
	#ifdef YADE_OPENGL
		(GLDrawCpmPhys)
	#endif	
		(CpmStateUpdater));


/********************** Ip2_CpmMat_CpmMat_CpmPhys ****************************/

CREATE_LOGGER(Ip2_CpmMat_CpmMat_CpmPhys);

void Ip2_CpmMat_CpmMat_CpmPhys::go(const shared_ptr<Material>& pp1, const shared_ptr<Material>& pp2, const shared_ptr<Interaction>& interaction){
	// no updates of an already existing contact necessary
	if(interaction->interactionPhysics) return;
	shared_ptr<CpmPhys> cpmPhys(new CpmPhys());
	interaction->interactionPhysics=cpmPhys;
	CpmMat* mat1=YADE_CAST<CpmMat*>(pp1.get());
	CpmMat* mat2=YADE_CAST<CpmMat*>(pp2.get());

	// check unassigned values
	assert(!isnan(mat1->G_over_E));
	if(!mat1->neverDamage) {
		assert(!isnan(mat1->sigmaT));
		assert(!isnan(mat1->epsCrackOnset));
		assert(!isnan(mat1->relDuctility));
		assert(!isnan(mat1->G_over_E));
	}

	// bodies sharing the same material; no averages necessary
	if(mat1->id>=0 && mat1->id==mat2->id) {
		cpmPhys->E=mat1->young;
		cpmPhys->G=mat1->young*mat1->G_over_E;
		cpmPhys->tanFrictionAngle=tan(mat1->frictionAngle);
		cpmPhys->undamagedCohesion=mat1->sigmaT;
		cpmPhys->epsFracture=mat1->relDuctility*mat1->epsCrackOnset;
		cpmPhys->isCohesive=(cohesiveThresholdIter<0 || scene->currentIteration<cohesiveThresholdIter);
		#define _CPATTR(a) cpmPhys->a=mat1->a
			_CPATTR(epsCrackOnset);
			_CPATTR(neverDamage);
			_CPATTR(dmgTau);
			_CPATTR(dmgRateExp);
			_CPATTR(plTau);
			_CPATTR(plRateExp);
			_CPATTR(isoPrestress);
		#undef _CPATTR
	} else {
		// averaging over both materials
		#define _AVGATTR(a) cpmPhys->a=.5*(mat1->a+mat2->a)
			cpmPhys->E=.5*(mat1->young+mat2->young);
			cpmPhys->G=.5*(mat1->G_over_E+mat2->G_over_E)*.5*(mat1->young+mat2->young);
			cpmPhys->tanFrictionAngle=tan(.5*(mat1->frictionAngle+mat2->frictionAngle));
			cpmPhys->undamagedCohesion=.5*(mat1->sigmaT+mat2->sigmaT);
			cpmPhys->epsFracture=.5*(mat1->relDuctility+mat2->relDuctility)*.5*(mat1->epsCrackOnset+mat2->epsCrackOnset);
			cpmPhys->isCohesive=(cohesiveThresholdIter<0 || scene->currentIteration<cohesiveThresholdIter);
			_AVGATTR(epsCrackOnset);
			cpmPhys->neverDamage=(mat1->neverDamage || mat2->neverDamage);
			_AVGATTR(dmgTau);
			_AVGATTR(dmgRateExp);
			_AVGATTR(plTau);
			_AVGATTR(plRateExp);
			_AVGATTR(isoPrestress);
		#undef _AVGATTR
	}
	// NOTE: some params are not assigned until in Law2_Dem3DofGeom_CpmPhys_Cpm, since they need geometry as well; those are:
	// 	crossSection, kn, ks
}




/********************** CpmPhys ****************************/
CREATE_LOGGER(CpmPhys);

// !! at least one virtual function in the .cpp file
CpmPhys::~CpmPhys(){};

CREATE_LOGGER(Law2_Dem3DofGeom_CpmPhys_Cpm);

long CpmPhys::cummBetaIter=0, CpmPhys::cummBetaCount=0;

Real CpmPhys::solveBeta(const Real c, const Real N){
	#ifdef YADE_DEBUG
		cummBetaCount++;
	#endif
	const int maxIter=20;
	const Real maxError=1e-12;
	Real f, ret=0.;
	for(int i=0; i<maxIter; i++){
		#ifdef YADE_DEBUG
			cummBetaIter++;
		#endif
		Real aux=c*exp(N*ret)+exp(ret);
		f=log(aux);
		if(fabs(f)<maxError) return ret;
		Real df=(c*N*exp(N*ret)+exp(ret))/aux;
		ret-=f/df;
	}
	LOG_FATAL("No convergence after "<<maxIter<<" iters; c="<<c<<", N="<<N<<", ret="<<ret<<", f="<<f);
	throw runtime_error("Law2_Dem3DofGeom_CpmPhys_Cpm::solveBeta failed to converge.");
}

Real CpmPhys::computeDmgOverstress(Real dt){
	if(dmgStrain>=epsN*omega){ // unloading, no viscous stress
		dmgStrain=epsN*omega;
		LOG_TRACE("Elastic/unloading, no viscous overstress");
		return 0.;
	}
	Real c=epsCrackOnset*(1-omega)*pow(dmgTau/dt,dmgRateExp)*pow(epsN*omega-dmgStrain,dmgRateExp-1.);
	Real beta=solveBeta(c,dmgRateExp);
	Real deltaDmgStrain=(epsN*omega-dmgStrain)*exp(beta);
	dmgStrain+=deltaDmgStrain;
	LOG_TRACE("deltaDmgStrain="<<deltaDmgStrain<<", viscous overstress "<<(epsN*omega-dmgStrain)*E);
	/* σN=Kn(εN-εd); dmgOverstress=σN-(1-ω)*Kn*εN=…=Kn(ω*εN-εd) */
	return (epsN*omega-dmgStrain)*E;
}

Real CpmPhys::computeViscoplScalingFactor(Real sigmaTNorm, Real sigmaTYield,Real dt){
	if(sigmaTNorm<sigmaTYield) return 1.;
	Real c=undamagedCohesion*pow(plTau/(G*dt),plRateExp)*pow(sigmaTNorm-sigmaTYield,plRateExp-1.);
	Real beta=solveBeta(c,plRateExp);
	//LOG_DEBUG("scaling factor "<<1.-exp(beta)*(1-sigmaTYield/sigmaTNorm));
	return 1.-exp(beta)*(1-sigmaTYield/sigmaTNorm);
}


/********************** Law2_Dem3DofGeom_CpmPhys_Cpm ****************************/

/// yield surface parameters
int  Law2_Dem3DofGeom_CpmPhys_Cpm::yieldSurfType=2;
Real Law2_Dem3DofGeom_CpmPhys_Cpm::yieldLogSpeed=.1;
Real Law2_Dem3DofGeom_CpmPhys_Cpm::yieldEllipseShift=0.; // deprecated
/// compressive plasticity parameters
// approximates confinement -20MPa precisely, -100MPa a little over, -200 and -400 are OK (secant)
Real Law2_Dem3DofGeom_CpmPhys_Cpm::epsSoft=-3e-3; // deactivated if >=0
Real Law2_Dem3DofGeom_CpmPhys_Cpm::relKnSoft=.25;
// >=1. to deactivate (never delete damaged contacts)
Real Law2_Dem3DofGeom_CpmPhys_Cpm::omegaThreshold=1.;

#ifdef YADE_CPM_FULL_MODEL_AVAILABLE
	#include"../../../../brefcom-mm.hh"
#endif


void Law2_Dem3DofGeom_CpmPhys_Cpm::go(shared_ptr<InteractionGeometry>& _geom, shared_ptr<InteractionPhysics>& _phys, Interaction* I, Scene* scene){
	Dem3DofGeom* contGeom=static_cast<Dem3DofGeom*>(_geom.get());
	CpmPhys* BC=static_cast<CpmPhys*>(_phys.get());

	// just the first time
	if(I->isFresh(scene)){
		Real minRad=(contGeom->refR1<=0?contGeom->refR2:(contGeom->refR2<=0?contGeom->refR1:min(contGeom->refR1,contGeom->refR2)));
		BC->crossSection=Mathr::PI*pow(minRad,2);
		BC->kn=BC->crossSection*BC->E/contGeom->refLength;
		BC->ks=BC->crossSection*BC->G/contGeom->refLength;
	}

	// shorthands
		Real& epsN(BC->epsN);
		Vector3r& epsT(BC->epsT); Real& kappaD(BC->kappaD); Real& epsPlSum(BC->epsPlSum); const Real& E(BC->E); const Real& undamagedCohesion(BC->undamagedCohesion); const Real& tanFrictionAngle(BC->tanFrictionAngle); const Real& G(BC->G); const Real& crossSection(BC->crossSection); const Real& omegaThreshold(Law2_Dem3DofGeom_CpmPhys_Cpm::omegaThreshold); const Real& epsCrackOnset(BC->epsCrackOnset); Real& relResidualStrength(BC->relResidualStrength); const Real& epsFracture(BC->epsFracture); const bool& neverDamage(BC->neverDamage); Real& omega(BC->omega); Real& sigmaN(BC->sigmaN);  Vector3r& sigmaT(BC->sigmaT); Real& Fn(BC->Fn); Vector3r& Fs(BC->Fs); // for python access
	 const bool& isCohesive(BC->isCohesive);

	#ifdef CPM_MATERIAL_MODEL
		Real& epsNPl(BC->epsNPl); const Real& dt=Omega::instance().getTimeStep(); const Real& dmgTau(BC->dmgTau); const Real& plTau(BC->plTau);const Real& yieldLogSpeed(Law2_Dem3DofGeom_CpmPhys_Cpm::yieldLogSpeed); const int& yieldSurfType(Law2_Dem3DofGeom_CpmPhys_Cpm::yieldSurfType); const Real& yieldEllipseShift(Law2_Dem3DofGeom_CpmPhys_Cpm::yieldEllipseShift); const Real& epsSoft(Law2_Dem3DofGeom_CpmPhys_Cpm::epsSoft); const Real& relKnSoft(Law2_Dem3DofGeom_CpmPhys_Cpm::relKnSoft); 
	#endif


	epsN=contGeom->strainN(); epsT=contGeom->strainT();
	
	// debugging
		#define YADE_VERIFY(condition) if(!(condition)){LOG_FATAL("Verification `"<<#condition<<"' failed!"); LOG_FATAL("in interaction #"<<I->getId1()<<"+#"<<I->getId2()); Omega::instance().saveSimulation("/tmp/verificationFailed.xml"); throw;}

		#define NNAN(a) YADE_VERIFY(!isnan(a));
		#define NNANV(v) YADE_VERIFY(!isnan(v[0])); assert(!isnan(v[1])); assert(!isnan(v[2]));
		#ifdef YADE_DEBUG
			if(isnan(epsN)){
				LOG_FATAL("refLength="<<contGeom->refLength<<"; pos1="<<contGeom->se31.position<<"; pos2="<<contGeom->se32.position<<"; displacementN="<<contGeom->displacementN());
			throw runtime_error("!! epsN==NaN !!");
			}
			NNAN(epsN); NNANV(epsT);
		#endif
		NNAN(epsN); NNANV(epsT);

	// constitutive law 
	#ifdef CPM_MATERIAL_MODEL
		// complicated version
		if(epsSoft>=0)	epsN+=BC->isoPrestress/E;
		else{ // take softening into account for the prestress
			Real sigmaSoft=E*epsSoft;
			if(BC->isoPrestress>=sigmaSoft) epsN+=BC->isoPrestress/E; // on the non-softened branch yet
			// otherwise take the regular and softened branches separately (different moduli)
			else epsN+=sigmaSoft/E+(BC->isoPrestress-sigmaSoft)/(E*relKnSoft);
		}
		CPM_MATERIAL_MODEL
	#else
		// simplified public model
		epsN+=BC->isoPrestress/E;
		// very simplified version of the constitutive law
		kappaD=max(max(0.,epsN),kappaD); // internal variable, max positive strain (non-decreasing)
		omega=isCohesive?funcG(kappaD,epsCrackOnset,epsFracture,neverDamage):1.; // damage variable (non-decreasing, as funcG is also non-decreasing)
		sigmaN=(1-(epsN>0?omega:0))*E*epsN; // damage taken in account in tension only
		sigmaT=G*epsT; // trial stress
		Real yieldSigmaT=max((Real)0.,undamagedCohesion*(1-omega)-sigmaN*tanFrictionAngle); // Mohr-Coulomb law with damage
		if(sigmaT.SquaredLength()>yieldSigmaT*yieldSigmaT){
			sigmaT*=yieldSigmaT/sigmaT.Length(); // stress return
			epsPlSum+=yieldSigmaT*contGeom->slipToStrainTMax(yieldSigmaT/G); // adjust strain
		}
		relResidualStrength=isCohesive?(kappaD<epsCrackOnset?1.:(1-omega)*(kappaD)/epsCrackOnset):0;
	#endif
	sigmaN-=BC->isoPrestress;

	NNAN(kappaD); NNAN(epsCrackOnset); NNAN(epsFracture); NNAN(omega);
	NNAN(sigmaN); NNANV(sigmaT); NNAN(crossSection);

	// handle broken contacts
	if(epsN>0. && ((isCohesive && omega>omegaThreshold) || !isCohesive)){
		if(isCohesive){
			const shared_ptr<Body>& body1=Body::byId(I->getId1(),scene), body2=Body::byId(I->getId2(),scene); assert(body1); assert(body2);
			const shared_ptr<CpmState>& st1=YADE_PTR_CAST<CpmState>(body1->state), st2=YADE_PTR_CAST<CpmState>(body2->state);
			// nice article about openMP::critical vs. scoped locks: http://www.thinkingparallel.com/2006/08/21/scoped-locking-vs-critical-in-openmp-a-personal-shootout/
			{ boost::mutex::scoped_lock lock(st1->updateMutex); st1->numBrokenCohesive+=1; st1->epsPlBroken+=epsPlSum; }
			{ boost::mutex::scoped_lock lock(st2->updateMutex); st2->numBrokenCohesive+=1; st2->epsPlBroken+=epsPlSum; }
		}
		scene->interactions->requestErase(I->getId1(),I->getId2());
		return;
	}

	Fn=sigmaN*crossSection; BC->normalForce=Fn*contGeom->normal;
	Fs=sigmaT*crossSection; BC->shearForce=Fs;

	applyForceAtContactPoint(BC->normalForce+BC->shearForce, contGeom->contactPoint, I->getId1(), contGeom->se31.position, I->getId2(), contGeom->se32.position, scene);
}

#ifdef YADE_OPENGL
	/********************** GLDrawCpmPhys ****************************/
	#include<yade/lib-opengl/OpenGLWrapper.hpp>
	#include<yade/lib-opengl/GLUtils.hpp>

	CREATE_LOGGER(GLDrawCpmPhys);

	bool GLDrawCpmPhys::contactLine=true;
	bool GLDrawCpmPhys::dmgLabel=true;
	bool GLDrawCpmPhys::dmgPlane=false;
	bool GLDrawCpmPhys::epsNLabel=true;
	bool GLDrawCpmPhys::epsT=false;
	bool GLDrawCpmPhys::epsTAxes=false;
	bool GLDrawCpmPhys::normal=false;
	bool GLDrawCpmPhys::colorStrain=false;


	void GLDrawCpmPhys::go(const shared_ptr<InteractionPhysics>& ip, const shared_ptr<Interaction>& i, const shared_ptr<Body>& b1, const shared_ptr<Body>& b2, bool wireFrame){
		const shared_ptr<CpmPhys>& BC=static_pointer_cast<CpmPhys>(ip);
		const shared_ptr<Dem3DofGeom>& geom=YADE_PTR_CAST<Dem3DofGeom>(i->interactionGeometry);

		//Vector3r lineColor(BC->omega,1-BC->omega,0.0); /* damaged links red, undamaged green */
		Vector3r lineColor=Shop::scalarOnColorScale(1.-BC->relResidualStrength);

		if(colorStrain) lineColor=Vector3r(
			min((Real)1.,max((Real)0.,-BC->epsTrans/BC->epsCrackOnset)),
			min((Real)1.,max((Real)0.,BC->epsTrans/BC->epsCrackOnset)),
			min((Real)1.,max((Real)0.,abs(BC->epsTrans)/BC->epsCrackOnset-1)));

		// FIXME: should be computed by the renderer; for now, use the real values
		const Se3r& dispSe31=b1->state->se3;
		const Se3r& dispSe32=b2->state->se3;

		if(contactLine) GLUtils::GLDrawLine(dispSe31.position,dispSe32.position,lineColor);
		if(dmgLabel){ GLUtils::GLDrawNum(BC->omega,0.5*(dispSe32.position+dispSe32.position),lineColor); }
		else if(epsNLabel){ GLUtils::GLDrawNum(BC->epsN,0.5*(dispSe31.position+dispSe32.position),lineColor); }
		if(BC->omega>0 && dmgPlane){
			Real halfSize=sqrt(1-BC->relResidualStrength)*.5*.705*sqrt(BC->crossSection);
			Vector3r midPt=.5*Vector3r(dispSe31.position+dispSe32.position);
			glDisable(GL_CULL_FACE);
			glPushMatrix();
				glTranslatev(midPt);
				Quaternionr q; q.Align(Vector3r::UNIT_Z,geom->normal);
				Vector3r axis; Real angle; q.ToAxisAngle(axis,angle);
				glRotatef(angle*Mathr::RAD_TO_DEG,axis[0],axis[1],axis[2]);
				glBegin(GL_POLYGON);
					glColor3v(lineColor); 
					glVertex3d(halfSize,0.,0.);
					glVertex3d(.5*halfSize,.866*halfSize,0.);
					glVertex3d(-.5*halfSize,.866*halfSize,0.);
					glVertex3d(-halfSize,0.,0.);
					glVertex3d(-.5*halfSize,-.866*halfSize,0.);
					glVertex3d(.5*halfSize,-.866*halfSize,0.);
				glEnd();
			glPopMatrix();
		}

		const Vector3r& cp=static_pointer_cast<Dem3DofGeom>(i->interactionGeometry)->contactPoint;
		if(epsT){
			Real maxShear=(BC->undamagedCohesion-BC->sigmaN*BC->tanFrictionAngle)/BC->G;
			Real relShear=BC->epsT.Length()/maxShear;
			Real scale=.5*geom->refLength;
			Vector3r dirShear=BC->epsT; dirShear.Normalize();
			if(epsTAxes){
				GLUtils::GLDrawLine(cp-Vector3r(scale,0,0),cp+Vector3r(scale,0,0));
				GLUtils::GLDrawLine(cp-Vector3r(0,scale,0),cp+Vector3r(0,scale,0));
				GLUtils::GLDrawLine(cp-Vector3r(0,0,scale),cp+Vector3r(0,0,scale));
			}
			GLUtils::GLDrawArrow(cp,cp+dirShear*relShear*scale,Vector3r(1.,0.,0.));
			GLUtils::GLDrawLine(cp+dirShear*relShear*scale,cp+dirShear*scale,Vector3r(.3,.3,.3));

			/* normal strain */ GLUtils::GLDrawArrow(cp,cp+geom->normal*(BC->epsN/maxShear),Vector3r(0.,1.,0.));
		}
		//if(normal) GLUtils::GLDrawArrow(cp,cp+geom->normal*.5*BC->equilibriumDist,Vector3r(0.,1.,0.));
	}
#endif

/********************** CpmGlobalCharacteristics ****************************/
/*** DEPRECATED ***/
#if 0
CREATE_LOGGER(CpmGlobalCharacteristics);
void CpmGlobalCharacteristics::compute(Scene* rb, bool useMaxForce){
	rb->forces.sync();

	// 1. reset volumetric strain (cummulative in the next loop)
	// 2. get maximum force on a body and sum of all forces (for averaging)
	Real sumF=0,maxF=0,currF;
	FOREACH(const shared_ptr<Body>& b, *rb->bodies){
	CpmMat* bpp(YADE_CAST<CpmMat*>(b->physicalParameters.get()));
		bpp->epsVolumetric=0;
		bpp->numContacts=0;
		currF=rb->forces.getForce(b->id).Length(); maxF=max(currF,maxF); sumF+=currF;
	}
	Real meanF=sumF/rb->bodies->size(); 

	// commulate normal strains from contacts
	// get max force on contacts
	Real maxContactF=0;
	FOREACH(const shared_ptr<Interaction>& I, *rb->interactions){
		if(!I->isReal()) continue;
		shared_ptr<CpmPhys> BC=YADE_PTR_CAST<CpmPhys>(I->interactionPhysics); assert(BC);
		maxContactF=max(maxContactF,max(BC->Fn,BC->Fs.Length()));
		CpmMat* bpp1(YADE_CAST<CpmMat*>(Body::byId(I->getId1())->physicalParameters.get()));
		CpmMat* bpp2(YADE_CAST<CpmMat*>(Body::byId(I->getId2())->physicalParameters.get()));
		bpp1->epsVolumetric+=BC->epsN; bpp1->numContacts+=1;
		bpp2->epsVolumetric+=BC->epsN; bpp2->numContacts+=1;
	}
	unbalancedForce=(useMaxForce?maxF:meanF)/maxContactF;

	FOREACH(const shared_ptr<Interaction>& I, *rb->interactions){
		if(!I->isReal()) continue;
		shared_ptr<CpmPhys> BC=YADE_PTR_CAST<CpmPhys>(I->interactionPhysics); assert(BC);
		CpmMat* bpp1(YADE_CAST<CpmMat*>(Body::byId(I->getId1())->physicalParameters.get()));
		CpmMat* bpp2(YADE_CAST<CpmMat*>(Body::byId(I->getId2())->physicalParameters.get()));
		Real epsVolAvg=.5*((3./bpp1->numContacts)*bpp1->epsVolumetric+(3./bpp2->numContacts)*bpp2->epsVolumetric);
		BC->epsTrans=(epsVolAvg-BC->epsN)/2.;
		//TRVAR5(I->getId1(),I->getId2(),BC->epsTrans,(3./bpp1->numContacts)*bpp1->epsVolumetric,(3./bpp2->numContacts)*bpp2->epsVolumetric);
		//TRVAR4(bpp1->numContacts,bpp1->epsVolumetric,bpp2->numContacts,bpp2->epsVolumetric);
	}
	#if 0
		FOREACH(const shared_ptr<Body>& b, *rb->bodies){
			CpmMat* bpp(YADE_PTR_CAST<CpmMat>(b->physicalParameters.get()));
			bpp->epsVolumeric*=3/bpp->numContacts;
		}
	#endif
}
#endif

/********************** CpmStateUpdater ****************************/
CREATE_LOGGER(CpmStateUpdater);
Real CpmStateUpdater::maxOmega=0.;
Real CpmStateUpdater::avgRelResidual=0.;

void CpmStateUpdater::update(Scene* _scene){
	Scene *scene=_scene?_scene:Omega::instance().getScene().get();
	vector<BodyStats> bodyStats; bodyStats.resize(scene->bodies->size());
	assert(bodyStats[0].nCohLinks==0); // should be initialized by dfault ctor
	avgRelResidual=0; Real nAvgRelResidual=0;
	FOREACH(const shared_ptr<Interaction>& I, *scene->interactions){
		if(!I->isReal()) continue;
		shared_ptr<CpmPhys> phys=dynamic_pointer_cast<CpmPhys>(I->interactionPhysics);
		if(!phys) continue;
		const body_id_t id1=I->getId1(), id2=I->getId2();
		Dem3DofGeom* geom=YADE_CAST<Dem3DofGeom*>(I->interactionGeometry.get());
		
		Vector3r normalStress=((1./phys->crossSection)*geom->normal.Dot(phys->normalForce))*geom->normal;
		bodyStats[id1].sigma+=normalStress; bodyStats[id2].sigma+=normalStress;
		Vector3r shearStress;
		for(int i=0; i<3; i++){
			int ix1=(i+1)%3,ix2=(i+2)%3;
			shearStress[i]=Mathr::Sign(geom->normal[ix1])*phys->shearForce[ix1]+Mathr::Sign(geom->normal[ix2])*phys->shearForce[ix2];
			shearStress[i]/=phys->crossSection;
		}
		bodyStats[id1].tau+=shearStress;
		bodyStats[id2].tau+=shearStress;
		bodyStats[id1].nLinks++; bodyStats[id2].nLinks++;
		
		if(!phys->isCohesive) continue;
		bodyStats[id1].nCohLinks++; bodyStats[id1].dmgSum+=(1-phys->relResidualStrength); bodyStats[id1].epsPlSum+=phys->epsPlSum;
		bodyStats[id2].nCohLinks++; bodyStats[id2].dmgSum+=(1-phys->relResidualStrength); bodyStats[id2].epsPlSum+=phys->epsPlSum;
		maxOmega=max(maxOmega,phys->omega);

		avgRelResidual+=phys->relResidualStrength;
		nAvgRelResidual+=1;
	}
	FOREACH(shared_ptr<Body> B, *scene->bodies){
		const body_id_t& id=B->getId();
		// add damaged contacts that have already been deleted
		CpmState* state=dynamic_cast<CpmState*>(B->state.get());
		if(!state) continue;
		state->sigma=bodyStats[id].sigma;
		state->tau=bodyStats[id].tau;
		int cohLinksWhenever=bodyStats[id].nCohLinks+state->numBrokenCohesive;
		if(cohLinksWhenever>0){
			state->normDmg=(bodyStats[id].dmgSum+state->numBrokenCohesive)/cohLinksWhenever;
			state->normEpsPl=(bodyStats[id].epsPlSum+state->epsPlBroken)/cohLinksWhenever;
			if(state->normDmg>1){
				LOG_WARN("#"<<id<<" normDmg="<<state->normDmg<<" nCohLinks="<<bodyStats[id].nCohLinks<<", numBrokenCohesive="<<state->numBrokenCohesive<<", dmgSum="<<bodyStats[id].dmgSum<<", numAllCohLinks"<<cohLinksWhenever);
			}
		}
		else { state->normDmg=0; state->normEpsPl=0;}
		B->shape->diffuseColor=Vector3r(state->normDmg,1-state->normDmg,B->isDynamic?0:1);
		nAvgRelResidual+=0.5*state->numBrokenCohesive; // add half or broken interactions, other body has the other half
	}
	avgRelResidual/=nAvgRelResidual;
}
