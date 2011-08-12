#include<yade/pkg/dem/IdealElPl.hpp>
YADE_PLUGIN(dem,(Law2_L6Geom_FrictPhys_IdealElPl)(Law2_L6Geom_FrictPhys_LinEl6));

#ifdef YADE_DEBUG
	#define _WATCH_MSG(msg) if(watched) cerr<<msg;
#else
	#define _WATCH_MSG(msg)
#endif

void Law2_L6Geom_FrictPhys_IdealElPl::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); FrictPhys& ph(cp->cast<FrictPhys>());
	#ifdef YADE_DEBUG
		bool watched=(max(C->pA->id,C->pB->id)==watch.maxCoeff() && min(C->pA->id,C->pB->id)==watch.minCoeff());
	#endif
	_WATCH_MSG("Step "<<scene->step<<", ##"<<C->pA->id<<"+"<<C->pB->id<<": "<<endl);
	if(g.uN>0 && !noBreak){ _WATCH_MSG("\tContact being broken."<<endl); field->cast<DemField>().contacts.requestRemoval(C); return; }
	ph.torque=Vector3r::Zero();
	ph.force[0]=ph.kn*g.uN;
	Eigen::Map<Vector2r> Ft(&ph.force[1]); 
	Ft+=scene->dt*ph.kt*Vector2r(g.vel[1],g.vel[2]);
	Real maxFt=abs(ph.force[0])*ph.tanPhi; assert(maxFt>=0);
	_WATCH_MSG("\tFn="<<ph.force[0]<<", trial Ft="<<Ft.transpose()<<" (incremented by "<<(scene->dt*ph.kt*Vector2r(g.vel[1],g.vel[2])).transpose()<<"), max Ft="<<maxFt<<endl);
	if(Ft.squaredNorm()>maxFt*maxFt && !noSlip){
		Real ratio=maxFt/Ft.norm();
		Ft*=ratio;
		_WATCH_MSG("\tPlastic slip by "<<((Ft/ratio)*(1-ratio)).transpose()<<", ratio="<<ratio<<", new Ft="<<Ft.transpose()<<endl);
		// not sure about the sin term; it would be the minimum dissipation path (?) if we consider the slip as associated (which it is not)
		if(scene->trackEnergy){ Real dissip=/*sin(atan(ph.tanPhi))* */ (1/ph.kt)*(1/ratio-1)*Ft.squaredNorm(); if(dissip>0) scene->energy->add(dissip,"plast",plastDissipIx,/*reset*/false); }
	}
	if(unlikely(scene->trackEnergy)){ scene->energy->add(0.5*(pow(ph.force[0],2)/ph.kn+Ft.squaredNorm()/ph.kt),"elast",elastPotIx,/*non-cummulative*/true); }
};

void Law2_L6Geom_FrictPhys_LinEl6::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	L6Geom& geom=cg->cast<L6Geom>(); FrictPhys& phys=cp->cast<FrictPhys>(); const Real& dt=scene->dt;
	if(charLen<0) throw std::invalid_argument("Law2_L6Geom_FrictPhys_LinEl6.charLen must be non-negative (is "+lexical_cast<string>(charLen)+")");
	// simple linear increments
	Vector3r kntt(phys.kn,phys.kt,phys.kt); // normal and tangent stiffnesses
	Vector3r ktbb(kntt/charLen); // twist and beding stiffnesses
	phys.force+=dt*(geom.vel.cwise()*kntt);
	phys.torque+=dt*(geom.angVel.cwise()*ktbb);
	// compute normal force non-incrementally
	phys.force[0]=phys.kn*geom.uN;
	if(scene->trackEnergy){
		/* both formulations give the same result with relative precision within 1e-14 (values 1e3, difference 1e-11) */
		// absolute, as .5*F^2/k (per-component)
		scene->energy->add(.5*((phys.force.array().pow(2)/kntt.array()).sum()+(phys.torque.array().pow(2)/ktbb.array()).sum()),"elast",elastPotIx,/*non-cummulative*/true);
		#if 0
			// incremental delta (needs mid-step force) as (F-½Δt v k)*Δt v
			scene->energy->add((phys.force-.5*dt*(geom.vel.cwise()*kntt)).dot(dt*geom.vel)+(phys.torque-.5*dt*(geom.angVel.cwise()*ktbb)).dot(dt*geom.angVel),"elast",elastPotIx,/*non-cummulative*/false);
		#endif
	}
};
