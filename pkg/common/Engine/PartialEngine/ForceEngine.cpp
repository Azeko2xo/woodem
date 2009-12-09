// 2004 © Janek Kozicki <cosurgi@berlios.de> 
// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 


#include"ForceEngine.hpp"
#include<yade/pkg-common/ParticleParameters.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg-common/LinearInterpolate.hpp>
#include<yade/pkg-dem/Shop.hpp>

YADE_PLUGIN((ForceEngine)(InterpolatingDirectedForceEngine));
void ForceEngine::applyCondition(Scene* ncb){
	FOREACH(body_id_t id, subscribedBodies){
		assert(ncb->bodies->exists(id));
		ncb->bex.addForce(id,force);
	}
}

void InterpolatingDirectedForceEngine::applyCondition(Scene* rb){
	Real virtTime=wrap ? Shop::periodicWrap(rb->simulationTime,*times.begin(),*times.rbegin()) : rb->simulationTime;
	direction.Normalize(); 
	force=linearInterpolate<Real>(virtTime,times,magnitudes,_pos)*direction;
	ForceEngine::applyCondition(rb);
}


YADE_REQUIRE_FEATURE(PHYSPAR);

