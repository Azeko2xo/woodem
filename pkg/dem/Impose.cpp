#include<woo/pkg/dem/Impose.hpp>
WOO_PLUGIN(dem,(HarmonicOscillation)(AlignedHarmonicOscillations)(CircularOrbit)(RadialForce)(Local6Dofs));


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
