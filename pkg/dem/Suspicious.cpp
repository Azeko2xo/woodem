#include<woo/pkg/dem/Suspicious.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Contact.hpp>
#include<woo/pkg/dem/L6Geom.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/dem/Gl1_DemField.hpp>
#endif

CREATE_LOGGER(Suspicious);
WOO_PLUGIN(dem,(Suspicious));
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Suspicious__CLASS_BASE_DOC_ATTRS);

void Suspicious::run(){
	#ifdef WOO_OPENL
		boost::mutex::scoped_lock l(errMutex);
	#endif
	const auto& dem=field->cast<DemField>();
	long nNode=0,nCon=0;

	// compute average values
	Real vel=0.,force=0.,fn=0.,ft=0.,un=0.;
	for(const auto& node: dem.nodes){
		const auto& dyn=node->getData<DemData>();
		vel+=dyn.vel.norm(); force+=dyn.force.norm();
		nNode++;
	}
	vel/=nNode; force/=nNode;

	for(const auto& C: *dem.contacts){
		if(!C || !C->isReal() || !C->geom->isA<L6Geom>()) continue;
		un+=abs(C->geom->cast<L6Geom>().uN);
		fn+=abs(C->phys->force[0]);
		ft+=Vector2r(C->phys->force[1],C->phys->force[2]).norm();
		nCon++;
	}
	un/=nCon; fn/=nCon; ft/=nCon;

	// assign average values; TODO: apply smoothing here?!
	avgVel=vel; avgForce=force; avgUn=un; avgFn=fn; avgFt=ft;

	// loop again and check values
	bool allOk=true, parVelOk=true, parForceOk=true, conOk=true; 
	for(const auto& p: errPar) p->shape->setHighlighted(false);
	errPar.clear(); errCon.clear();

	for(const auto& node: dem.nodes){
		const auto& dyn=node->getData<DemData>();
		Particle::id_t id=-1;
		if(!dyn.parRef.empty()){ id=(*dyn.parRef.begin())->id; }
		if(dyn.vel.norm()>avgVel*relThreshold){ parVelOk=allOk=false; LOG_ERROR("S.dem.nodes["<<dyn.linIx<<"] / S.dem.par["<<id<<"]: velocity "<<dyn.vel.transpose()<<", norm="<<dyn.vel.norm()<<" exceeds limit value of "<<relThreshold<<"*"<<avgVel<<" [*"<<dyn.vel.norm()/avgVel<<"]"); if(!dyn.parRef.empty()) errPar.push_back(static_pointer_cast<Particle>((*dyn.parRef.begin())->shared_from_this())); }
		if(dyn.force.norm()>avgForce*relThreshold){ parForceOk=allOk=false; LOG_ERROR("S.dem.nodes["<<dyn.linIx<<"] / S.dem.par["<<id<<"]: force "<<dyn.force.transpose()<<", norm="<<dyn.force.norm()<<" exceeds limit value of "<<relThreshold<<"*"<<avgForce<<" [*"<<dyn.force.norm()/avgForce<<"]"); if(!dyn.parRef.empty()) errPar.push_back(static_pointer_cast<Particle>((*dyn.parRef.begin())->shared_from_this())); }
	}

	for(const auto& C: *dem.contacts){
		if(!C || !C->isReal() || !C->geom->isA<L6Geom>()) continue;
		const Real& un=abs(C->geom->cast<L6Geom>().uN);
		const Real& fn=abs(C->phys->force[0]);
		Real ft=Vector2r(C->phys->force[1],C->phys->force[2]).norm();
		if(un>avgUn*relThreshold){ conOk=false; LOG_ERROR("S.dem.con["<<C->pyId1()<<","<<C->pyId2()<<"]: overlap "<<un<<" exceeds the limit value of "<<relThreshold<<"*"<<avgVel<<" [*"<<un/avgUn<<"]"); errCon.push_back(C); }
		if(fn>avgFn*relThreshold){ conOk=false; LOG_ERROR("S.dem.con["<<C->pyId1()<<","<<C->pyId2()<<"]: overlap "<<un<<" exceeds the limit value of "<<relThreshold<<"*"<<avgVel<<" [*"<<un/avgFn<<"]"); errCon.push_back(C); }
		if(ft>avgFt*relThreshold){ conOk=false; LOG_ERROR("S.dem.con["<<C->pyId1()<<","<<C->pyId2()<<"]: overlap "<<un<<" exceeds the limit value of "<<relThreshold<<"*"<<avgVel<<" [*"<<un/avgFt<<"]"); errCon.push_back(C); }
	}
	#ifdef WOO_OPENGL
		if(!parForceOk) Gl1_DemField::glyph=Gl1_DemField::GLYPH_FORCE;
		if(!parVelOk) Gl1_DemField::glyph=Gl1_DemField::GLYPH_VEL;
	#endif

	if(!allOk) throw std::runtime_error("Suspicious conditions, summary above in the terminal.");
};


#ifdef WOO_OPENGL
#include<woo/lib/opengl/GLUtils.hpp>
void Suspicious::render(const GLViewInfo& viewInfo){
	boost::mutex::scoped_lock l(errMutex);
	Vector3r size=.05*viewInfo.sceneRadius*Vector3r::Ones();
	for(const auto& p: errPar){
		Vector3r pos=p->shape->nodes[0]->pos;
		if(scene->isPeriodic) pos=scene->cell->canonicalizePt(pos);
		p->shape->setHighlighted(true);
		GLUtils::AlignedBox(AlignedBox3r(pos-size,pos+size),Vector3r(.68,1.,.2));
	};
	for(const auto& C: errCon){
		if(!C->isReal()) continue;
		Vector3r pos(C->geom->node->pos);
		if(scene->isPeriodic) pos=scene->cell->canonicalizePt(pos);
		GLUtils::AlignedBox(AlignedBox3r(pos-size,pos+size),Vector3r(1.,.68,.6));
	}
}
#endif /* WOO_OPENGL */

