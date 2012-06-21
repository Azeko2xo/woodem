#include<woo/pkg/dem/ForceEngine.hpp>
WOO_PLUGIN(dem,(RadialForce));
void RadialForce::run(){
	FOREACH(const shared_ptr<Node>& node, nodes){
		assert(node->hasData<DemData>());
		DemData& dyn=node->getData<DemData>();
		Vector3r radial=(node->pos-(axisPt+axisDir * /* t */ ((node->pos-axisPt).dot(axisDir)))).normalized();
		if(radial.squaredNorm()==0) continue;
		dyn.addForceTorque(fNorm*radial,Vector3r::Zero());
	}
}
