
#include<woo/pkg/dem/Hertz.hpp>
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
	Real E=1./( (1-nu1*nu1)/E1 + (1-nu2*nu2)/E2 );
	Real R=1./(1./r1+1./r2);
	ph.kn0=(4/3.)*E*sqrt(R);

	// shear behavior
	Real G1=E1/(2*1+nu1); Real G2=E2/(2*1+nu2);
	Real G=.5*(G1+G2);
	Real nu=.5*(nu1+nu2); 
	ph.kt0=2*sqrt(4*R)*G/(2-nu);
	ph.tanPhi=min(mat1.tanPhi,mat2.tanPhi);

	// DMT adhesion ("sticking") force
	// See Dejaguin1975 ("Effect of Contact Deformation on the Adhesion of Particles"), eq (44)
	// Derjaguin has Fs=2πRφ(ε), which is derived for sticky sphere (with surface energy)
	// in contact with a rigid plane (without surface energy); therefore the value here is twice that of Derjaguin
	// See also Chiara's thesis, pg 39 eq (3.19).
	ph.Fa=4*M_PI*R*gamma; 

	// non-linear viscous damping parameters
	// only for meaningful values of en
	if(en>0 && en<1.){
		const Real& m1=C->leakPA()->shape->nodes[0]->getData<DemData>().mass;
		const Real& m2=C->leakPB()->shape->nodes[0]->getData<DemData>().mass;
		// equiv mass, but use only the other particle if one has no mass
		// if both have no mass, then mbar is irrelevant as their motion won't be influenced by force
		Real mbar=(m1<=0 && m2>0)?m2:((m1>0 && m2<=0)?m1:(m1*m2)/(m1+m2));
		// For eqs, see Antypov2012, (10) and (17)
		Real alpha=-sqrt(5)*log(en)/(sqrt(pow(log(en),2)+pow(M_PI,2)));
		ph.alpha_sqrtMK=max(0.,alpha*sqrt(mbar*ph.kn0)); // negative is nonsense, then no damping at all
	} else {
		// no damping at all
		ph.alpha_sqrtMK=0.0;
	}
	LOG_DEBUG("E="<<E<<", G="<<G<<", nu="<<nu<<", R="<<R<<", kn0="<<ph.kn0<<", kt0="<<ph.kt0<<", tanPhi="<<ph.tanPhi<<", Fa="<<ph.Fa<<", alpha_sqrtMK="<<ph.alpha_sqrtMK);
}


CREATE_LOGGER(Law2_L6Geom_HertzPhys_DMT);

void Law2_L6Geom_HertzPhys_DMT::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); HertzPhys& ph(cp->cast<HertzPhys>());
	// break contact
	if(g.uN>0){
		// track nonzero energy of broken contact with adhesion or any other residual force
		// TODO: take residual shear force in account?
		// TODO: account for Fa!=0
		if(unlikely(scene->trackEnergy)) scene->energy->add(normalElasticEnergy(ph.kn0,-g.uN),"dmtComeGo",dmtIx,EnergyTracker::IsIncrement|EnergyTracker::ZeroDontCreate);
		field->cast<DemField>().contacts->requestRemoval(C); return;
	}
	// new contacts with adhesion add energy to the system, which is then taken away again
	if(unlikely(scene->trackEnergy ) && C->isFresh(scene)){
		// TODO: scene->energy->add(???,"dmtComeGo",dmtIx,EnergyTracker::IsIncrement)
	}
	Real& Fn(ph.force[0]); Eigen::Map<Vector2r> Ft(&ph.force[1]);
	const Real& dt(scene->dt);
	const Real& velN(g.vel[0]);
	const Vector2r velT(g.vel[1],g.vel[2]);

	ph.torque=Vector3r::Zero();
	// viscous coefficient, both for normal and tangential force
	// Antypov2012 (10)
	Real cn=(ph.alpha_sqrtMK>0?ph.alpha_sqrtMK*pow_1_4(-g.uN):0.);

	// normal force
	ph.kn=(3/2.)*ph.kn0*sqrt(-g.uN);
	Fn=-ph.kn0*pow_i_2(-g.uN,3)+cn*velN+ph.Fa; // XXX: check the sign of Fa
	// normal viscous dissipation
	if(unlikely(scene->trackEnergy)) scene->energy->add(cn*velN*velN*dt,"viscN",viscNIx,EnergyTracker::IsIncrement|EnergyTracker::ZeroDontCreate);

	// shear force
	ph.kt=ph.kt0*sqrt(-g.uN);
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
		Ft+=cn*velT;
		if(unlikely(scene->trackEnergy)) scene->energy->add(cn*velT.dot(velT)*dt,"viscT",viscTIx,EnergyTracker::IsIncrement|EnergyTracker::ZeroDontCreate);
	}
	assert(!isnan(Fn)); assert(!isnan(Ft[0]) && !isnan(Ft[1]));
	// elastic potential energy
	if(unlikely(scene->trackEnergy)) scene->energy->add(normalElasticEnergy(ph.kn0,-g.uN)+0.5*Ft.squaredNorm()/ph.kt,"elast",elastPotIx,EnergyTracker::IsResettable);
	LOG_DEBUG("uN="<<g.uN<<", Fn="<<Fn<<"; duT/dt="<<velT[0]<<","<<velT[1]<<", Ft="<<Ft[0]<<","<<Ft[1]);
}


