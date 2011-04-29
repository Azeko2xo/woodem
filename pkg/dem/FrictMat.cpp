#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/core/MatchMaker.hpp>
#include<yade/pkg/dem/L6Geom.hpp>

YADE_PLUGIN(dem,(ElastMat)(FrictMat)(FrictPhys)(Cp2_FrictMat_FrictPhys)(Law2_L6Geom_FrictPhys_IdealElPl));

void Cp2_FrictMat_FrictPhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){
	if(C->phys) return;
	C->phys=shared_ptr<CPhys>(new FrictPhys);
	FrictPhys& ph=C->phys->cast<FrictPhys>();
	FrictMat& mat1=m1->cast<FrictMat>(); FrictMat& mat2=m2->cast<FrictMat>();
	assert(dynamic_cast<L6Geom*>(C->geom.get()));
	Vector2r len=C->geom->cast<L6Geom>().lengthHint;
	ph.kn=1/(1/(mat1.young*2*len[0])+1/(mat2.young*2*len[1]));
	ph.kt=ktDivKn*ph.kn;
	ph.tanPhi=(!tanPhi)?std::min(mat1.tanPhi,mat2.tanPhi):(*tanPhi)(mat1.id,mat2.id,mat1.tanPhi,mat2.tanPhi);
};

void Law2_L6Geom_FrictPhys_IdealElPl::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); FrictPhys& ph(cp->cast<FrictPhys>());
	if(g.uN>0 && !noBreak){ scene->field->cast<DemField>().contacts.requestRemoval(C); return; }
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

