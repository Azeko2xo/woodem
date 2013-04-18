#include<woo/pkg/dem/IdealElPl.hpp>
WOO_PLUGIN(dem,(Law2_L6Geom_FrictPhys_IdealElPl)(IdealElPlData)(Law2_L6Geom_FrictPhys_LinEl6));

#ifdef WOO_DEBUG
	#define _WATCH_MSG(msg) if(watched) cerr<<msg;
#else
	#define _WATCH_MSG(msg)
#endif

CREATE_LOGGER(Law2_L6Geom_FrictPhys_IdealElPl);

void Law2_L6Geom_FrictPhys_IdealElPl::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); FrictPhys& ph(cp->cast<FrictPhys>());
	#ifdef WOO_DEBUG
		bool watched=(max(C->leakPA()->id,C->leakPB()->id)==watch.maxCoeff() && min(C->leakPA()->id,C->leakPB()->id)==watch.minCoeff());
	#endif
	_WATCH_MSG("Step "<<scene->step<<", ##"<<C->leakPA()->id<<"+"<<C->leakPB()->id<<": "<<endl);
	Real uN=g.uN;
	if(iniEqlb){
		if(C->isFresh(scene)){ C->data=make_shared<IdealElPlData>(); C->data->cast<IdealElPlData>().uN0=uN; }
		if(C->data) uN-=C->data->cast<IdealElPlData>().uN0;
	}
	if(uN>0 && !noBreak){ _WATCH_MSG("\tContact being broken."<<endl);
		// without slipping, it is possible that a contact breaks and shear force disappears
		if(noSlip && scene->trackEnergy){
			// when contact is broken, it is simply its elastic potential which gets lost
			// it should be the current potential, though, since the previous one already generated force response
			// ugly, we duplicate the code below here
			const Vector2r velT(g.vel[1],g.vel[2]); Eigen::Map<Vector2r> prevFt(&ph.force[1]);
			// const Real& prevFn=ph.force[0];
			Real Fn=ph.kn*uN; Vector2r Ft=prevFt+scene->dt*ph.kt*velT;
			// Real z=.5;
			//Real z=abs(prevFn)/(abs(Fn)+abs(prevFn));
			//Fn=.5*(z*Fn+(1-z)*ph.force[0]); Ft=.5*(z*prevFt+(1-z)*Ft);
			// Real Fn=ph.force[0]; Vector2r Ft(ph.force[1],ph.force[2]);
			scene->energy->add(.5*(pow(Fn,2)/ph.kn+Ft.squaredNorm()/ph.kt),"broken",brokenIx,EnergyTracker::IsIncrement);
		}
		field->cast<DemField>().contacts->requestRemoval(C); return;
	}
	ph.torque=Vector3r::Zero();

	// normal force
	ph.force[0]=ph.kn*uN;

	// tangential force
	Eigen::Map<Vector2r> Ft(&ph.force[1]); 
	if(noFrict){
		Ft=Vector2r::Zero();
	} else {
		// const Eigen::Map<Vector2r> velT(&g.vel[1]);
		const Vector2r velT(g.vel[1],g.vel[2]);
		Ft+=scene->dt*ph.kt*velT;
		Real maxFt=abs(ph.force[0])*ph.tanPhi; assert(maxFt>=0);
		_WATCH_MSG("\tFn="<<ph.force[0]<<", trial Ft="<<Ft.transpose()<<" (incremented by "<<(scene->dt*ph.kt*velT).transpose()<<"), max Ft="<<maxFt<<endl);
		if(Ft.squaredNorm()>maxFt*maxFt && !noSlip){
			Real FtNorm=Ft.norm();
			Real ratio=maxFt/FtNorm;
			// do this while Ft is still the trial value
			if(scene->trackEnergy){
				/* in the linear displacement-force graph, compute the are sliced away by the force drop; it has the
				1. top triangle part, .5*(Ft-Fm)*(Ft-Fm)/kt
				2. rectangle under this triangle, Fm*(Ft-Fm)/kt
				which gives together (.5*(Ft-Fm)+Fm)*(Ft-Fm)/kt (rectangle with top in the middle of the triangle height)
				where Fm=maxFt and Ft=FtNorm
				*/
				Real dissip=(.5*(FtNorm-maxFt)+maxFt)*(FtNorm-maxFt)/ph.kt;
				//Real dissip=(maxFt)*(FtNorm-maxFt)/ph.kt;
				scene->energy->add(dissip,"plast",plastDissipIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
			}
			Ft*=ratio;
			_WATCH_MSG("\tPlastic slip by "<<((Ft/ratio)*(1-ratio)).transpose()<<", ratio="<<ratio<<", new Ft="<<Ft.transpose()<<endl);
		}
		if(isnan(ph.force[0]) || isnan(ph.force[1]) || isnan(ph.force[2])){
			LOG_FATAL("##"<<C->leakPA()->id<<"+"<<C->leakPB()->id<<" ("<<C->leakPA()->shape->getClassName()<<"+"<<C->leakPB()->shape->getClassName()<<") has NaN force!");
			LOG_FATAL("    uN="<<uN<<", velT="<<velT.transpose()<<", F="<<ph.force.transpose()<<"; maxFt="<<maxFt<<"; kn="<<ph.kn<<", kt="<<ph.kt);
			throw std::runtime_error("NaN force in contact (message above)?!");
		}
	}
	if(unlikely(scene->trackEnergy)){ scene->energy->add(0.5*(pow(ph.force[0],2)/ph.kn+Ft.squaredNorm()/ph.kt),"elast",elastPotIx,EnergyTracker::IsResettable); }
};

void Law2_L6Geom_FrictPhys_LinEl6::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	L6Geom& geom=cg->cast<L6Geom>(); FrictPhys& phys=cp->cast<FrictPhys>(); const Real& dt=scene->dt;
	if(charLen<0) throw std::invalid_argument("Law2_L6Geom_FrictPhys_LinEl6.charLen must be non-negative (is "+lexical_cast<string>(charLen)+")");
	// simple linear increments
	Vector3r kntt(phys.kn,phys.kt,phys.kt); // normal and tangent stiffnesses
	Vector3r ktbb(kntt/charLen); // twist and beding stiffnesses
	phys.force+=dt*(geom.vel.array()*kntt.array()).matrix();
	phys.torque+=dt*(geom.angVel.array()*ktbb.array()).matrix();
	// compute normal force non-incrementally
	phys.force[0]=phys.kn*geom.uN;
	if(scene->trackEnergy){
		// this handles zero stiffnesses correctly
		Real E=.5*pow(phys.force[0],2)/phys.kn; // normal stiffness always non-zero
		if(kntt[1]!=0.) E+=.5*(pow(phys.force[1],2)+pow(phys.force[2],2))/kntt[1];
		if(ktbb[0]!=0.) E+=.5*pow(phys.torque[0],2)/ktbb[0];
		if(ktbb[1]!=0.) E+=.5*(pow(phys.torque[1],2)+pow(phys.torque[2],2))/ktbb[1];
		scene->energy->add(E,"elast",elastPotIx,EnergyTracker::IsResettable);
		/* both formulations give the same result with relative precision within 1e-14 (values 1e3, difference 1e-11) */
		// absolute, as .5*F^2/k (per-component)
		//scene->energy->add(.5*((phys.force.array().pow(2)/kntt.array()).sum()+(phys.torque.array().pow(2)/ktbb.array()).sum()),"elast",elastPotIx,EnergyTracker::IsResettable);
		#if 0
			// incremental delta (needs mid-step force) as (F-½Δt v k)*Δt v
			scene->energy->add((phys.force-.5*dt*(geom.vel.cwise()*kntt)).dot(dt*geom.vel)+(phys.torque-.5*dt*(geom.angVel.cwise()*ktbb)).dot(dt*geom.angVel),"elast",elastPotIx,EnergyTracker::IsIncrement);
		#endif
	}
};
