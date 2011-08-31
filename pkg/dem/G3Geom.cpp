/*
	This code was mostly written by original Yade authors, in particular Bruno Chareyre.
	It was ported purely for pure comparison purposes.
*/
#include<yade/pkg/dem/G3Geom.hpp>

YADE_PLUGIN(dem,(G3Geom)(Cg2_Sphere_Sphere_G3Geom)(Law2_G3Geom_FrictPhys_IdealElPl)(G3GeomCData));

void G3Geom::rotateVectorWithContact(Vector3r& v){
	assert(!isnan(orthonormalAxis.maxCoeff()) && !isnan(twistAxis.maxCoeff()));
	v-=v.cross(orthonormalAxis);
	v-=v.cross(twistAxis);
	// v-=normal.dot(v)*normal;
};

bool Cg2_Sphere_Sphere_G3Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	assert(s1->numNodesOk()); assert(s2->numNodesOk());
	assert(s1->nodes[0]->hasData<DemData>()); assert(s2->nodes[0]->hasData<DemData>());
	const DemData& dyn1(s1->nodes[0]->getData<DemData>()); const DemData& dyn2(s2->nodes[0]->getData<DemData>());
	const Vector3r& pos1(s1->nodes[0]->pos), pos2(s2->nodes[0]->pos);
	const Real& r1=s1->cast<Sphere>().radius; const Real& r2=s2->cast<Sphere>().radius;
	const Real& dt=scene->dt;

	Vector3r normal=(pos2+shift2)-pos1;
	if(!C->isReal() && !force && (pow(r1+r2,2)-normal.squaredNorm()<0)){
		// cerr<<"relPos="<<normal.transpose()<<", dist="<<normal.norm()<<", r1="<<r1<<", r2="<<r2<<endl;
		return false;
	}

	shared_ptr<G3Geom> g3g;
	bool isNew=!C->geom;
	if(!isNew) g3g=YADE_PTR_CAST<G3Geom>(C->geom);
	else { g3g=make_shared<G3Geom>(); C->geom=g3g; }
	Real dist=normal.norm(); normal/=dist; // normal is unit vector now
	g3g->uN=dist-(r1+r2);
	C->geom->node->pos=pos1+(r1+.5*g3g->uN)*normal;
	if(!isNew) {
		const Vector3r& oldNormal=g3g->normal;
		g3g->orthonormalAxis=oldNormal.cross(normal);
		Real angle=dt*0.5*oldNormal.dot(dyn1.angVel+dyn2.angVel);
		g3g->twistAxis=angle*oldNormal;
	}
	else{
		g3g->twistAxis=g3g->orthonormalAxis=Vector3r(NaN,NaN,NaN);
		// local and global orientation coincide
		C->geom->node->ori=Quaternionr::Identity();
	}
	g3g->normal=normal;
	
	/* compute relative velocity */
	Vector3r relVel;
	Vector3r shiftVel=scene->isPeriodic?scene->cell->intrShiftVel(C->cellDist):Vector3r::Zero();

	if(noRatch){
		/* B.C. Comment :
		Short explanation of what we want to avoid :
		Numerical ratcheting is best understood considering a small elastic cycle at a contact between two grains : assuming b1 is fixed, impose this displacement to b2 :
		1. translation "dx" in the normal direction
		2. rotation "a"
		3. translation "-dx" (back to initial position)
		4. rotation "-a" (back to initial orientation)
		If the branch vector used to define the relative shear in rotation×branch is not constant (typically if it is defined from the vector center→contactPoint), then the shear displacement at the end of this cycle is not zero: rotations *a* and *-a* are multiplied by branches of different lengths.
		It results in a finite contact force at the end of the cycle even though the positions and orientations are unchanged, in total contradiction with the elastic nature of the problem. It could also be seen as an *inconsistent energy creation or loss*. Given that DEM simulations tend to generate oscillations around equilibrium (damped mass-spring), it can have a significant impact on the evolution of the packings, resulting for instance in slow creep in iterations under constant load.
		The solution adopted here to avoid ratcheting is as proposed by McNamara and co-workers.
		They analyzed the ratcheting problem in detail - even though they comment on the basis of a cycle that differs from the one shown above. One will find interesting discussions in e.g. DOI 10.1103/PhysRevE.77.031304, even though solution it suggests is not fully applied here (equations of motion are not incorporating alpha, in contradiction with what is suggested by McNamara et al.).
		 */
		// For sphere-facet contact this will give an erroneous value of relative velocity...
		Real alpha=(useAlpha?(r1+r2)/(r1+r2+g3g->uN):1.);
		relVel=(dyn2.vel-dyn1.vel)*alpha+dyn2.angVel.cross(-r2*normal)-dyn1.angVel.cross(r1*normal);
		relVel+=alpha*shiftVel;
	} else {
		// It is correct for sphere-sphere and sphere-facet contact
		Vector3r c1x=C->geom->node->pos-pos1;
		Vector3r c2x=C->geom->node->pos-(pos2+shift2);
		relVel=(dyn2.vel+dyn2.angVel.cross(c2x))-(dyn1.vel+dyn1.angVel.cross(c1x));
		relVel+=shiftVel;
	}
	//keep the shear part only
	relVel-=normal.dot(relVel)*normal;

	// update shear displacement delta
	g3g->dShear=-relVel*dt;

	return true;
}


#ifdef YADE_DEBUG
	#define _WATCH_MSG(msg) if(watched) cerr<<msg;
#else
	#define _WATCH_MSG(msg)
#endif


void Law2_G3Geom_FrictPhys_IdealElPl::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>&C){
	G3Geom& geom=cg->cast<G3Geom>(); FrictPhys& phys=cp->cast<FrictPhys>();
	#ifdef YADE_DEBUG
		bool watched=(max(C->pA->id,C->pB->id)==watch.maxCoeff() && (watch.minCoeff()<0 || min(C->pA->id,C->pB->id)==watch.minCoeff()));
	#endif
	_WATCH_MSG("Step "<<scene->step<<", ##"<<C->pA->id<<"+"<<C->pB->id<<": "<<endl);
	if(geom.uN>0){
		if(!noBreak){ _WATCH_MSG("Contact being broken."<<endl); field->cast<DemField>().contacts.requestRemoval(C); return; }
	}
	Vector3r normalForce=phys.kn*geom.uN*geom.normal;
	if(!C->data){ C->data=make_shared<G3GeomCData>(); }
	G3GeomCData& dta=C->data->cast<G3GeomCData>();
	// rotation axes are not computed if contact is new
	if(!C->isFresh(scene)) geom.rotateVectorWithContact(dta.shearForce);
	_WATCH_MSG("\trotated previous Fs="<<dta.shearForce.transpose());
	dta.shearForce-=phys.kt*geom.dShear;
	_WATCH_MSG("\t; incremented by "<<(phys.kt*geom.dShear).transpose()<<" to "<<dta.shearForce.transpose()<<endl);
	Real maxFs=max(0.,normalForce.squaredNorm()*std::pow(phys.tanPhi,2));
	_WATCH_MSG("\tFn="<<normalForce.norm()<<", trial Fs="<<dta.shearForce.transpose()<<", max Fs="<<sqrt(maxFs)<<endl);
	if(dta.shearForce.squaredNorm()>maxFs){
		Real ratio=sqrt(maxFs)/dta.shearForce.norm();
		Vector3r trialForce=dta.shearForce;//store prev force for definition of plastic slip
		//define the plastic work input and increment the total plastic energy dissipated
		dta.shearForce*=ratio;
		_WATCH_MSG("\tPlastic slip by "<<((dta.shearForce/ratio)*(1-ratio)).transpose()<<", ratio="<<ratio<<", new Ft="<<dta.shearForce.transpose()<<endl);
		if(scene->trackEnergy){
			Real dissip=((1/phys.kt)*(trialForce-dta.shearForce))/*plastic disp*/ .dot(dta.shearForce)/*active force*/;
			scene->energy->add(dissip,"plastDissip",plastDissipIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate);
		}
	}
	// elastic potential
	if(scene->trackEnergy) scene->energy->add(0.5*(normalForce.squaredNorm()/phys.kn+dta.shearForce.squaredNorm()/phys.kt),"elastPot",elastPotIx,/*reset at every timestep*/EnergyTracker::IsResettable);
	// this is the force in contact local coordinates, which will be applied
	// local coordinates are not rotated, though
	assert(C->geom->node->ori==Quaternionr::Identity());
	phys.force=normalForce+dta.shearForce;
}

