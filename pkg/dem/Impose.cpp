#include<woo/pkg/dem/Impose.hpp>
WOO_PLUGIN(dem,(HarmonicOscillation)(AlignedHarmonicOscillations)(RadialForce)(Local6Dofs));

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
