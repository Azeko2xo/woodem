#include<woo/pkg/dem/Pellet.hpp>
WOO_PLUGIN(dem,(PelletMat)(PelletPhys)(Cp2_PelletMat_PelletPhys)(Law2_L6Geom_PelletPhys_Pellet)(PelletCData));

void Cp2_PelletMat_PelletPhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){
	if(!C->phys) C->phys=make_shared<PelletPhys>();
	auto& mat1=m1->cast<PelletMat>(); auto& mat2=m2->cast<PelletMat>();
	auto& ph=C->phys->cast<PelletPhys>();
	Cp2_FrictMat_FrictPhys::updateFrictPhys(mat1,mat2,ph,C);
	ph.normPlastCoeff=.5*(max(0.,mat1.normPlastCoeff)+max(0.,mat2.normPlastCoeff));
	ph.ka=min(max(0.,mat1.kaDivKn),max(0.,mat2.kaDivKn))*ph.kn;
}



// FIXME: energy tracking does not consider adhesion and the integration is very bad (forward trapezoid)
// leading to 20% of energy erro in some cases

CREATE_LOGGER(Law2_L6Geom_PelletPhys_Pellet);

void Law2_L6Geom_PelletPhys_Pellet::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){
	const L6Geom& g(cg->cast<L6Geom>()); PelletPhys& ph(cp->cast<PelletPhys>());
	if(C->isFresh(scene)) C->data=make_shared<PelletCData>();
	assert(C->data && dynamic_pointer_cast<PelletCData>(C->data));
	// break contact
	if(g.uN>0){
		field->cast<DemField>().contacts->requestRemoval(C); return;
	}
	Real d0=g.lens.sum();
	Real& Fn(ph.force[0]); Eigen::Map<Vector2r> Ft(&ph.force[1]);
	Real& uNPl(C->data->cast<PelletCData>().uNPl);
	if(ph.normPlastCoeff<=0) uNPl=0.;
	const Vector2r velT(g.vel[1],g.vel[2]);

	ph.torque=Vector3r::Zero();
	
	// normal force
	Fn=ph.kn*(g.uN-uNPl); // trial force
	if(ph.normPlastCoeff>0){ // normal plasticity activated
		if(Fn>0){
			if(ph.ka<=0) Fn=0;
			else{ Fn=min(Fn,adhesionForce(g.uN,uNPl,ph.ka)); assert(Fn>0); }
		} else {
			Real Fy=yieldForce(g.uN,d0,ph.kn,ph.normPlastCoeff);
			// normal plastic slip
			if(Fn<Fy){
				Real uNPl0=uNPl; // neede when tracking energy
				uNPl=g.uN-Fy/ph.kn;
				if(unlikely(scene->trackEnergy)){
					// backwards trapezoid integration
					Real Fy0=Fy+yieldForceDerivative(g.uN,d0,ph.kn,ph.normPlastCoeff)*(uNPl0-uNPl);
					Real dissip=.5*abs(Fy0+Fy)*abs(uNPl-uNPl0);
					scene->energy->add(dissip,plastSplit?"normPlast":"plast",plastSplit?normPlastIx:plastIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
				}
				if(thinningFactor>0){
					Real dRad=thinningFactor*(uNPl0-uNPl);
					for(const auto& p:{C->pA,C->pB}){
						if(!dynamic_cast<Sphere*>(p->shape.get())) continue;
						auto& s=p->shape->cast<Sphere>();
						//Real r0=(C->geom->cast<L6Geom>().lens[p.get()==C->pA.get()?0:1]);
						Real r0=cbrt(3*s.nodes[0]->getData<DemData>().mass/(4*Mathr::PI*p->material->density));
						Real rMin=r0*rMinFrac;
						if(s.radius<=rMin) continue;
						boost::mutex::scoped_lock lock(s.nodes[0]->getData<DemData>().lock);
						// cerr<<"#"<<p->id<<": radius "<<s.radius<<" -> "<<s.radius-dRad<<endl;
						s.radius=max(rMin,s.radius-dRad);
						s.color=CompUtils::clamped(1-(s.radius-rMin)/(r0-rMin),0,1);
					}
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
