#include"MomentEngine.hpp"
#include<yade/core/Scene.hpp>


MomentEngine::MomentEngine(): moment(Vector3r::ZERO){}
MomentEngine::~MomentEngine(){}

void MomentEngine::applyCondition(Scene* ncb){
	FOREACH(const body_id_t id, subscribedBodies){
		// check that body really exists?
		scene->forces.addTorque(id,moment);
	}
}

YADE_PLUGIN((MomentEngine));

