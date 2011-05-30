#include<yade/pkg/dem/IdealElPl.hpp>
YADE_PLUGIN(dem,(Law2_L6Geom_FrictPhys_IdealElPl));

void Law2_L6Geom_FrictPhys_IdealElPl::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); FrictPhys& ph(cp->cast<FrictPhys>());
	if(g.uN>0 && !noBreak){ field->cast<DemField>().contacts.requestRemoval(C); return; }
	ph.torque=Vector3r::Zero();
	ph.force[0]=ph.kn*g.uN;
	Eigen::Map<Vector2r> Ft(&ph.force[1]); 
	Ft+=scene->dt*ph.kt*Vector2r(g.vel[1],g.vel[2]);
	Real maxFt=ph.force[0]*ph.tanPhi;
	if(Ft.squaredNorm()>maxFt*maxFt && !noSlip){
		Real ratio=maxFt/Ft.norm();
		Ft*=ratio;
		if(scene->trackEnergy){ Real dissip=(1/ph.kt)*(1/ratio-1)*Ft.squaredNorm(); if(dissip>0) scene->energy->add(dissip,"plastDissip",plastDissipIx,/*reset*/false); }
	}
	if(unlikely(scene->trackEnergy)){ scene->energy->add(0.5*(pow(ph.force[0],2)/ph.kn+Ft.squaredNorm()/ph.kt),"elastPot",elastPotIx,/*non-cummulative*/true); }
	// cerr<<scene->step<<": ##"<<C->pA->id<<"+"<<C->pB->id<<": F="<<ph.force<<endl;
};


