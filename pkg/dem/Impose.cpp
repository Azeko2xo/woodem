#include<woo/pkg/dem/Impose.hpp>
#include<woo/lib/smoothing/LinearInterpolate.hpp>

WOO_PLUGIN(dem,(HarmonicOscillation)(AlignedHarmonicOscillations)(CircularOrbit)(RadialForce)(Local6Dofs)(VariableAlignedRotation)(InterpolatedMotion));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_InterpolatedMotion__CLASS_BASE_DOC_ATTRS_CTOR);

void CircularOrbit::velocity(const Scene* scene, const shared_ptr<Node>& n){
	if(!node) throw std::runtime_error("CircularOrbit: node must not be None.");
	Vector3r& vv(n->getData<DemData>().vel);
	Vector2r p2((node->ori.conjugate()*(n->pos-node->pos)).head<2>()); // local 2d position
	Vector2r v2((node->ori.conjugate()*vv).head<2>()); // local 2d velocity
	// in polar coords
	Real theta=atan2(p2[1],p2[0]);
	Real rho=p2.norm();
	// tangential velocity at this point
	Real v2t=omega*rho; // XXX: might be actually smaller due to leapfrog?
	Real ttheta=theta+.5*v2t*scene->dt; // mid-step polar angle
	if(isFirstStepRun(scene)) angle+=omega*scene->dt;
	vv=node->ori*(Quaternionr(AngleAxisr(ttheta,Vector3r::UnitZ()))*Vector3r(0,v2t,0));
}

void RadialForce::force(const Scene* scene, const shared_ptr<Node>& n){
	if(!nodeA || !nodeB) throw std::runtime_error("RadialForce: nodeA and nodeB must not be None.");
	const Vector3r& A(nodeA->pos);
	const Vector3r& B(nodeB->pos);
	Vector3r axis=(B-A).normalized();
	Vector3r C=A+axis*((n->pos-A).dot(axis)); // closest point on the axis
	Vector3r towards=C-n->pos;
	if(towards.squaredNorm()==0) return; // on the axis, do nothing
	n->getData<DemData>().force-=towards.normalized()*F;
}

// function to impose both, distinguished by the last argument
void Local6Dofs::doImpose(const Scene* scene, const shared_ptr<Node>& n, bool velocity){
	DemData& dyn=n->getData<DemData>();
	for(int i=0;i<6;i++){
		if( velocity&&(whats[i]&Impose::VELOCITY)){
			if(i<3){
				//dyn.vel+=ori.conjugate()*(Vector3r::Unit(i)*(values[i]-(ori*dyn.vel).dot(Vector3r::Unit(i))));
				Vector3r locVel=ori*dyn.vel; locVel[i]=values[i]; dyn.vel=ori.conjugate()*locVel;
			} else {
				//dyn.angVel+=ori.conjugate()*(Vector3r::Unit(i%3)*(values[i]-(ori*dyn.angVel).dot(Vector3r::Unit(i%3))));
				Vector3r locAngVel=ori*dyn.angVel; locAngVel[i%3]=values[i]; dyn.angVel=ori.conjugate()*locAngVel;
			}
		}
		if(!velocity&&(whats[i]&Impose::FORCE)){
			// set absolute value, cancel anything set previously (such as gravity)
			if(i<3){ dyn.force =ori.conjugate()*(values[i]*Vector3r::Unit(i)); }
			else   { dyn.torque=ori.conjugate()*(values[i]*Vector3r::Unit(i%3)); }
		}
	}
}


void VariableAlignedRotation::postLoad(VariableAlignedRotation&,void*){
	if(timeAngVel.size()>1){
		for(size_t i=0; i<timeAngVel.size()-1; i++){
			if(!(timeAngVel[i+1].x()>=timeAngVel[i].x())) woo::ValueError("VariableAlignedRotation.timeAngVel: time values must be non-decreasing ("+to_string(timeAngVel[i].x())+".."+to_string(timeAngVel[i+1].x())+" at "+to_string(i)+".."+to_string(i+1));
		}
	}
}

void VariableAlignedRotation::velocity(const Scene* scene, const shared_ptr<Node>& n){
	auto& dyn=n->getData<DemData>();
	dyn.angVel[axis]=linearInterpolate(scene->time,timeAngVel,_interpPos);
}

void InterpolatedMotion::postLoad(InterpolatedMotion&,void* attr){
	if(attr!=NULL) return; // just some value being set
	if(poss.size()!=oris.size() || oris.size()!=times.size()) throw std::runtime_error("InterpolatedMotion: poss, oris, times must have the same length (not "+to_string(poss.size())+", "+to_string(oris.size())+", "+to_string(times.size()));
}

void InterpolatedMotion::velocity(const Scene* scene, const shared_ptr<Node>& n){
	Real nextT=scene->time+scene->dt;
	Vector3r nextPos=linearInterpolate(nextT,times,poss,_interpPos);
	Quaternionr nextOri, oriA, oriB; Real relAB;
	std::tie(oriA,oriB,relAB)=linearInterpolateRel(nextT,times,oris,_interpPos);
	nextOri=oriA.slerp(relAB,oriB);
	DemData& dyn=n->getData<DemData>();
	dyn.vel=(nextPos-n->pos)/scene->dt;
	AngleAxisr rot(n->ori.conjugate()*nextOri);
	dyn.angVel=rot.axis()*rot.angle()/scene->dt;
};
