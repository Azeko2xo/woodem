#include<woo/pkg/dem/Ellipsoid.hpp>
#include<woo/pkg/dem/Sphere.hpp>

WOO_PLUGIN(dem,(Ellipsoid));

void woo::Ellipsoid::selfTest(const shared_ptr<Particle>& p){
	if(!(semiAxes.minCoeff()>0)) throw std::runtime_error("Ellipsoid #"+to_string(p->id)+": all semi-princial semiAxes must be positive (current minimum is "+to_string(semiAxes.minCoeff())+")");
	if(!numNodesOk()) throw std::runtime_error("Ellipsoid #"+to_string(p->id)+": numNodesOk() failed: must be 1, not "+to_string(nodes.size())+".");
}

void woo::Ellipsoid::updateDyn(const Real& density) const {
	assert(numNodesOk());
	auto& dyn=nodes[0]->getData<DemData>();
	dyn.mass=(4/3.)*M_PI*semiAxes.prod()*density;
	dyn.inertia=(1/5.)*dyn.mass*Vector3r(pow(semiAxes[1],2)+pow(semiAxes[2],2),pow(semiAxes[2],2)+pow(semiAxes[0],2),pow(semiAxes[0],2)+pow(semiAxes[1],2));
};


#ifdef WOO_OPENGL
WOO_PLUGIN(gl,(Gl1_Ellipsoid));
#endif
