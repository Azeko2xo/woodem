#include<woo/pkg/dem/Pellet.hpp>
WOO_PLUGIN(dem,(Law2_L6Geom_FrictPhys_Pellet)(PelletCData));

// TODO: energy tracking

CREATE_LOGGER(Law2_L6Geom_FrictPhys_Pellet);

void Law2_L6Geom_FrictPhys_Pellet::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); FrictPhys& ph(cp->cast<FrictPhys>());
	if(C->isFresh(scene)) C->data=make_shared<PelletCData>();
	assert(C->data && dynamic_pointer_cast<PelletCData>(C->data));
	// break contact
	if(g.uN>0){
		field->cast<DemField>().contacts->requestRemoval(C); return;
	}
	Real d0=g.lens.sum();
	Real& Fn(ph.force[0]); Eigen::Map<Vector2r> Ft(&ph.force[1]);
	Real& uNPl(C->data->cast<PelletCData>().uNPl);
	if(alpha<=0) uNPl=0.;
	const Vector2r velT(g.vel[1],g.vel[2]);

	ph.torque=Vector3r::Zero();
	
	// normal force
	Fn=ph.kn*(g.uN-uNPl); // trial force
	if(alpha>0){ // normal plasticity activated
		if(Fn>0) Fn=0;
		else{
			Real Fy=yieldForce(g.uN,ph.kn,d0);
			// normal plastic slip
			if(Fn<Fy){
				Real uNPl0=uNPl; // neede when tracking energy
				uNPl=g.uN-Fy/ph.kn;
				if(unlikely(scene->trackEnergy)){
					// backwards trapezoid integration
					Real Fy0=Fy+yieldForceDerivative(g.uN,ph.kn,d0)*(uNPl0-uNPl);
					Real dissip=.5*abs(Fy0+Fy)*abs(uNPl-uNPl0);
					scene->energy->add(dissip,plastSplit?"normPlast":"plast",plastSplit?normPlastIx:plastIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
				}
				Fn=Fy;
			}
			// in the elastic regime, Fn is trial force already
		}
	}

	// shear force
	Ft+=scene->dt*ph.kt*velT;
	Real maxFt=abs(Fn)*ph.tanPhi; assert(maxFt>=0);
	// shear plastic slip
	if(Ft.squaredNorm()>pow(maxFt,2)){
		Real FtNorm=Ft.norm();
		Real ratio=maxFt/FtNorm;
		if(unlikely(scene->trackEnergy)){
			Real dissip=(.5*(FtNorm-maxFt)+maxFt)*(FtNorm-maxFt)/ph.kt;
			scene->energy->add(dissip,"plast",plastIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
		}
		Ft*=ratio;
	}
	assert(!isnan(Fn)); assert(!isnan(Ft[0]) && !isnan(Ft[1]));
	// elastic potential energy
	if(unlikely(scene->trackEnergy)) scene->energy->add(0.5*(pow(Fn,2)/ph.kn+Ft.squaredNorm()/ph.kt),"elast",elastPotIx,EnergyTracker::IsResettable);
}
