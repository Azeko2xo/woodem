
#include<woo/pkg/dem/Hertz.hpp>
#include <boost/math/tools/tuple.hpp>
#include <boost/math/tools/roots.hpp>

WOO_PLUGIN(dem,(HertzPhys)(Cp2_FrictMat_HertzPhys)(Law2_L6Geom_HertzPhys_DMT));

CREATE_LOGGER(Cp2_FrictMat_HertzPhys);
void Cp2_FrictMat_HertzPhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){
	if(!C->phys) C->phys=make_shared<HertzPhys>();
	auto& mat1=m1->cast<FrictMat>(); auto& mat2=m2->cast<FrictMat>();
	auto& ph=C->phys->cast<HertzPhys>();
	const auto& l6g=C->geom->cast<L6Geom>();
	const Real& r1=l6g.lens[0]; const Real& r2=l6g.lens[1];
	const Real& E1=mat1.young; const Real& E2=mat2.young;
	const Real& nu1=poisson; const Real& nu2=poisson;

	// normal behavior
	// Johnson1987, pg 427 (Appendix 3)
	ph.K=(4/3.)*1./( (1-nu1*nu1)/E1 + (1-nu2*nu2)/E2 );
	ph.R=1./(1./r1+1./r2);
	Real kn0=ph.K*sqrt(ph.R);
	ph.gamma=gamma;
	// COS alpha
	ph.alpha=alpha;

	#if 0
		// COS only
		Real K=(4/3.)*1./((1-nu1*nu1)/E1+(1-nu2*nu2)/E2); // COS Introduction
		// R already computed above
		Real lambda=-.924*log(1-1.02*alpha); // COS eq (10)
		Real hatLc=-7/4.+(1/4.)*((4.04*pow(lambda,1.4)-1)/(4.04*pow(lambda,1.4)+1));  // COS eq (12a)
		Real hatA0=(1.54+.279*((2.28*pow(lambda,1.3)-1)/(2.28*pow(lambda,1.3)+1))); // COS eq (12b) 
		ph.Lc=hatLc*M_PI*gamma*R; // COS eq (11a)
		ph.a0=hatA0/cbrt(K/(M_PI*gamma*R*R));
		ph.alpha=alpha;
		LOG_DEBUG("lambda="<<lambda<<", hatLc="<<hatLc<<", hatA0="<<hatA0<<", Lc="<<ph.Lc<<", a0="<<ph.a0<<", alpha="<<ph.alpha);
	#endif

	// shear behavior
	Real G1=E1/(2*1+nu1); Real G2=E2/(2*1+nu2);
	Real G=.5*(G1+G2);
	Real nu=.5*(nu1+nu2); 
	ph.kt0=2*sqrt(4*ph.R)*G/(2-nu);
	ph.tanPhi=min(mat1.tanPhi,mat2.tanPhi);

	// DMT adhesion ("sticking") force
	// See Dejaguin1975 ("Effect of Contact Deformation on the Adhesion of Particles"), eq (44)
	// Derjaguin has Fs=2πRφ(ε), which is derived for sticky sphere (with surface energy)
	// in contact with a rigid plane (without surface energy); therefore the value here is twice that of Derjaguin
	// See also Chiara's thesis, pg 39 eq (3.19).
	// ph.Fa=4*M_PI*ph.R*gamma; 

	// non-linear viscous damping parameters
	// only for meaningful values of en
	if(en>0 && en<1.){
		const Real& m1=C->leakPA()->shape->nodes[0]->getData<DemData>().mass;
		const Real& m2=C->leakPB()->shape->nodes[0]->getData<DemData>().mass;
		// equiv mass, but use only the other particle if one has no mass
		// if both have no mass, then mbar is irrelevant as their motion won't be influenced by force
		Real mbar=(m1<=0 && m2>0)?m2:((m1>0 && m2<=0)?m1:(m1*m2)/(m1+m2));
		// For eqs, see Antypov2012, (10) and (17)
		Real viscAlpha=-sqrt(5)*log(en)/(sqrt(pow(log(en),2)+pow(M_PI,2)));
		ph.alpha_sqrtMK=max(0.,viscAlpha*sqrt(mbar*kn0)); // negative is nonsense, then no damping at all
	} else {
		// no damping at all
		ph.alpha_sqrtMK=0.0;
	}
	LOG_DEBUG("K="<<ph.K<<", G="<<G<<", nu="<<nu<<", R="<<ph.R<<", kn0="<<kn0<<", kt0="<<ph.kt0<<", tanPhi="<<ph.tanPhi<<", alpha_sqrtMK="<<ph.alpha_sqrtMK);
}


CREATE_LOGGER(Law2_L6Geom_HertzPhys_DMT);

void Law2_L6Geom_HertzPhys_DMT::postLoad(Law2_L6Geom_HertzPhys_DMT&,void*){
}

void Law2_L6Geom_HertzPhys_DMT::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); HertzPhys& ph(cp->cast<HertzPhys>());
	Real& Fn(ph.force[0]); Eigen::Map<Vector2r> Ft(&ph.force[1]);
	ph.torque=Vector3r::Zero();
	const Real& dt(scene->dt);
	const Real& velN(g.vel[0]);
	const Vector2r velT(g.vel[1],g.vel[2]);


	if(g.uN>0){
		// TODO: track nonzero energy of broken contact with adhesion
		// TODO: take residual shear force in account?
		// if(unlikely(scene->trackEnergy)) scene->energy->add(normalElasticEnergy(ph.kn0,0),"dmtComeGo",dmtIx,EnergyTracker::IsIncrement|EnergyTracker::ZeroDontCreate);
		field->cast<DemField>().contacts->requestRemoval(C); return;
	}
	// new contacts with adhesion add energy to the system, which is then taken away again
	if(unlikely(scene->trackEnergy ) && C->isFresh(scene)){
		// TODO: scene->energy->add(???,"dmtComeGo",dmtIx,EnergyTracker::IsIncrement)
	}
	// current normal stiffness
	Real kn0=ph.K*sqrt(ph.R);
	// TODO: this not valid for Schwarz
	ph.kn=(3/2.)*kn0*sqrt(-g.uN);
	// normal elastic and adhesion forces
	// those are only split in the DMT model, Fna is zero for Schwarz or Hertz
	Real Fne, Fna=0.;
	if(ph.alpha==0.){
		// pure DMT
		Fne=-kn0*pow_i_2(-g.uN,3); // elastic force
		// DMT adhesion ("sticking") force
		// See Dejaguin1975 ("Effect of Contact Deformation on the Adhesion of Particles"), eq (44)
		// Derjaguin has Fs=2πRφ(ε), which is derived for sticky sphere (with surface energy)
		// in contact with a rigid plane (without surface energy); therefore the value here is twice that of Derjaguin
		// See also Chiara's thesis, pg 39 eq (3.19).
		Fna=4*M_PI*ph.R*ph.gamma;
	} else {
		const Real& gamma(ph.gamma); const Real& R(ph.R); const Real& alpha(ph.alpha); const Real& K(ph.K);
		Real delta=-g.uN; // inverse convention
		// Schwarz
		Real Pc=-6*M_PI*R*gamma/(pow(alpha,2)+3);
		Real xi=sqrt(((2*M_PI*gamma)/(3*K))*(1-3/(pow(alpha,2)+3)));
		Real deltaMin=-3*cbrt(R*pow(xi,4)); // -3R(-1/3)*ξ^(-4/3)
		// solution brackets
		// XXX: a is for sure also greater than delta(a) for Hertz model, with delta>0
		// this should be combined with aMin which gives the function apex
		Real aMin=pow_i_3(R*xi,2); // (Rξ)^(2/3)
		Real aMax=aMin+sqrt(R*(delta-deltaMin));
		auto delta_diff_ddiff=[&](const Real& a){
			Real aInvSqrt=1/sqrt(a);
			return boost::math::make_tuple(
				pow(a,2)/R-4*xi*sqrt(a)-delta, // subtract delta as we need f(x)=0
				2*a/R-2*xi*aInvSqrt,
				2/R+xi*pow(aInvSqrt,3)
			);
		};
		short digits=std::numeric_limits<Real>::digits/2;
		// use 2aMin as intial guess for new contacts, and previous value for old contacts
		Real aInit=(C->isFresh(scene)?2*aMin:ph.contRad); 
		unsigned long iter=100;
		Real a=boost::math::tools::halley_iterate(delta_diff_ddiff,aInit,aMin,aMax,digits,iter);
		// increment call and iteration count
		#ifdef WOO_SCHWARZ_COUNTERS
			nCallsIters.add(0,1);
			nCallsIters.add(1,iter);
		#endif

		ph.contRad=a;
		// cerr<<"a0="<<aInit<<", a="<<a<<", delta="<<delta<<", delta="<<std::get<0>(delta_diff_diff(a))<<endl;
		Real Pne=pow(sqrt(pow(a,3)*(K/R))-alpha*sqrt(-Pc),2)+Pc;
		Fne=-Pne; // inverse convention
	}

	// viscous coefficient, both for normal and tangential force
	// Antypov2012 (10)
	Real eta=(ph.alpha_sqrtMK>0?ph.alpha_sqrtMK*pow_1_4(-g.uN):0.);
	Real Fnv=eta*velN; // viscous force
	// DMT ONLY (for now at least):
	if(ph.alpha==0. && noAttraction && Fne+Fnv>0) Fnv=-Fne; // avoid viscosity which would induce attraction with DMT
	// total normal force
	Fn=Fne+Fna+Fnv; 
	// normal viscous dissipation
	if(unlikely(scene->trackEnergy)) scene->energy->add(Fnv*velN*dt,"viscN",viscNIx,EnergyTracker::IsIncrement|EnergyTracker::ZeroDontCreate);

	// shear sense; zero shear stiffness in tension (XXX: should be different with adhesion)
	ph.kt=ph.kt0*sqrt(g.uN<0?-g.uN:0);
	Ft=dt*ph.kt*velT;
	// sliding: take adhesion in account
	Real maxFt=max(0.,Fn)*ph.tanPhi;
	if(Ft.squaredNorm()>pow(maxFt,2)){
		// sliding
		Real FtNorm=Ft.norm();
		Real ratio=maxFt/FtNorm;
		// sliding dissipation
		if(unlikely(scene->trackEnergy)) scene->energy->add((.5*(FtNorm-maxFt)+maxFt)*(FtNorm-maxFt)/ph.kt,"plast",plastIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
		Ft*=ratio;
	} else {
		// viscous tangent force (only applied in the absence of sliding)
		Ft+=eta*velT;
		if(unlikely(scene->trackEnergy)) scene->energy->add(eta*velT.dot(velT)*dt,"viscT",viscTIx,EnergyTracker::IsIncrement|EnergyTracker::ZeroDontCreate);
	}
	assert(!isnan(Fn)); assert(!isnan(Ft[0]) && !isnan(Ft[1]));
	// elastic potential energy
	if(unlikely(scene->trackEnergy)) scene->energy->add(normalElasticEnergy(kn0,-g.uN)+0.5*Ft.squaredNorm()/ph.kt,"elast",elastPotIx,EnergyTracker::IsResettable);
	LOG_DEBUG("uN="<<g.uN<<", Fn="<<Fn<<"; duT/dt="<<velT[0]<<","<<velT[1]<<", Ft="<<Ft[0]<<","<<Ft[1]);
}


