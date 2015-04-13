#include<woo/pkg/dem/Ice.hpp>
WOO_PLUGIN(dem,(IceMat)(IcePhys)(Cp2_IceMat_IcePhys)(Law2_L6Geom_IcePhys));
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_IceMat__CLASS_BASE_DOC_ATTRS_CTOR);
#if 0
	WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_IceCData__CLASS_BASE_DOC_ATTRS);
#endif
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cp2_IceMat_IcePhys__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Law2_L6Geom_IcePhys__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_IcePhys__CLASS_BASE_DOC_ATTRS_CTOR_PY);


void Cp2_IceMat_IcePhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){
	if(!C->phys) C->phys=make_shared<IcePhys>();
	auto& mat1=m1->cast<IceMat>(); auto& mat2=m2->cast<IceMat>();
	auto& ph=C->phys->cast<IcePhys>();
	auto& g=C->geom->cast<L6Geom>();
	Cp2_FrictMat_FrictPhys::updateFrictPhys(mat1,mat2,ph,C);
	// twisting/rolling stiffnesses
	Vector2r alpha=.5*(mat1.alpha+mat2.alpha);
	ph.kWR=g.contA*alpha.cwiseProduct(Vector2r(ph.kn,ph.kt));
	// breakage limits
	Real cn=.5*(mat1.breakN+mat2.breakN)*(g.lens.sum()/(g.lens[0]/mat1.young+g.lens[1]/mat2.young));
	Vector3r beta=.5*(mat1.beta+mat2.beta);
	const Real& A(g.contA);
	ph.brkNT=Vector2r(cn*A,beta[0]*cn*A);
	ph.brkWR=Vector2r(beta[1],beta[2]).cwiseProduct(ph.brkNT)*sqrt(A);
	// rolling friction
	ph.mu=min(mat1.mu,mat2.mu);
	// bonding
	ph.bonds=scene->step<step01?bonds0:bonds1;
}

Real Law2_L6Geom_IcePhys::elastE(const IcePhys& p){
	return .5*(
		+(p.kn!=0?(pow(p.force[0],2)/p.kn):0.)
		+(p.kt!=0?(pow(p.force[1],2)+pow(p.force[2],2))/p.kt:0.)
		+(p.kWR[0]!=0?pow(p.torque[0],2)/p.kWR[0]:0.)
		+(p.kWR[1]!=0?(pow(p.torque[1],2)+pow(p.torque[2],2))/p.kWR[1]:0.)
	);
}

bool Law2_L6Geom_IcePhys::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); IcePhys& ph(cp->cast<IcePhys>());
#if 0
	//if(C->isFresh(scene)) C->data=make_shared<IceCData>();
	assert(C->data && C->data->isA<IceCData>());
#endif
	const Real& dt(scene->dt);
	// forces and torques in all 4 senses
	Real& Fn(ph.force[0]);  Eigen::Map<Vector2r> Ft(&ph.force[1]);
	Real& Tw(ph.torque[0]); Eigen::Map<Vector2r> Tr(&ph.torque[1]);
	// relative velocities
	Eigen::Map<const Vector2r> velT(&g.vel[1]);
	const Real& angVelW(g.angVel[0]); Eigen::Map<const Vector2r> angVelR(&g.angVel[1]);
	Real Ee0; // elastic energies
	bool energy(unlikely(scene->trackEnergy));
	// compute current (with old forces) elastic potential for the case the contact disappears
	if(energy) Ee0=elastE(ph); 
	// fresh contact, set uN0 for the first time
	if(C->isFresh(scene) && iniEqlb) ph.uN0=g.uN;

	// normal force: total formulation
	Fn=ph.kn*(g.uN-ph.uN0);
	// incremental formulation for the rest
	Ft+=dt*ph.kt*velT;
	Tw+=dt*ph.kWR[0]*angVelW;
	Tr+=dt*ph.kWR[1]*angVelR;

	// breakage conditions: if any breakable bond breaks, all other bonds break, too
	if((ph.isBrkBondX(0) && Fn>ph.brkNT[0]) ||
		(ph.isBrkBondX(1) && Ft.squaredNorm()>pow(ph.brkNT[1],2)) ||
		(ph.isBrkBondX(2) && abs(Tw)>ph.brkWR[1]) ||
		(ph.isBrkBondX(3) && Tr.squaredNorm()>pow(ph.brkWR[1],2))
	) ph.setAllBroken();

	// delete the contact if unbonded and in tension
	if(!ph.isBondX(0) && Fn>0){
		if(energy) scene->energy->add(Ee0,"break",brokenIx,EnergyTracker::IsIncrement);
		return false;
	}
	// current elastic potential (this includes the energy dissipated plastically below)
	if(energy){ scene->energy->add(elastE(ph),"elast",elastIx,EnergyTracker::IsResettable); }

	// plasticity
	Real negFn=-min(0.,Fn);
	Real sqrtApi=sqrt(g.contA/M_PI);
	// compute yield values
	Real Fty=negFn*ph.tanPhi, Twy=sqrtApi*negFn*ph.tanPhi, Try=sqrtApi*negFn*ph.mu;
	// plastic slip for unbonded senses
	if(!ph.isBondX(1) && Ft.squaredNorm()>pow(Fty,2)) slipXd(Ft,Ft.norm(),Fty,ph.kt,scene,"plast",plastIx);
	if(!ph.isBondX(2) && Tw>Twy) slipXd(Tw,abs(Tw),Twy,ph.kWR[0],scene,"plast",plastIx);
	if(!ph.isBondX(3) && Tr.squaredNorm()>pow(Try,2)) slipXd(Tr,Tr.norm(),Try,ph.kWR[1],scene,"plast",plastIx);

	// contact still existing
	return true;
}

