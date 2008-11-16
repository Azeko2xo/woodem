#include<yade/extra/Brefcom.hpp>
#include<boost/python.hpp>
using namespace boost;
using namespace std;
# if 0
Real elasticEnergyDensityInAABB(python::tuple AABB){
	Vector3r bbMin=tuple2vec(python::extract<python::tuple>(AABB[0])()), bbMax=tuple2vec(python::extract<python::tuple>(AABB[1])()); Vector3r box=bbMax-bbMin;
	shared_ptr<MetaBody> rb=Omega::instance().getRootBody();
	Real E=0;
	FOREACH(const shared_ptr<Interaction>&i, *rb->transientInteractions){
		if(!i->interactionPhysics) continue;
		shared_ptr<BrefcomContact> bc=dynamic_pointer_cast<BrefcomContact>(i->interactionPhysics); if(!bc) continue;
		const shared_ptr<Body>& b1=Body::byId(i->getId1(),rb), b2=Body::byId(i->getId2(),rb);
		bool isIn1=isInBB(b1->physicalParameters->se3.position,bbMin,bbMax), isIn2=isInBB(b2->physicalParameters->se3.position,bbMin,bbMax);
		if(!isIn1 && !isIn2) continue;
		Real weight=1.;
		if((!isIn1 && isIn2) || (isIn1 && !isIn2)){
			//shared_ptr<Body> bIn=isIn1?b1:b2, bOut=isIn2?b2:b1;
			Vector3r vIn=(isIn1?b1:b2)->physicalParameters->se3.position, vOut=(isIn2?b1:b2)->physicalParameters->se3.position;
			#define _WEIGHT_COMPONENT(axis) if(vOut[axis]<bbMin[axis]) weight=min(weight,abs((vOut[axis]-bbMin[axis])/(vOut[axis]-vIn[axis]))); else if(vOut[axis]>bbMax[axis]) weight=min(weight,abs((vOut[axis]-bbMax[axis])/(vOut[axis]-vIn[axis])));
			_WEIGHT_COMPONENT(0); _WEIGHT_COMPONENT(1); _WEIGHT_COMPONENT(2);
			assert(weight>=0 && weight<=1);
		}
		E+=.5*bc->E*bc->crossSection*pow(bc->epsN,2)+.5*bc->G*bc->crossSection*pow(bc->epsT.Length(),2);
	}
	return E/(box[0]*box[1]*box[2]);
}
#endif

