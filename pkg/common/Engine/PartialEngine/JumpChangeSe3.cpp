// 2008 © Václav Šmilauer <eudoxos@arcig.cz> 
#include"JumpChangeSe3.hpp"
#include<yade/core/State.hpp>
CREATE_LOGGER(JumpChangeSe3);
YADE_PLUGIN((JumpChangeSe3));

void JumpChangeSe3::action(){
	FOREACH(body_id_t id, subscribedBodies){
		const shared_ptr<Body>& b=Body::byId(id,scene);
		if(setVelocities){
			Real dt=Omega::instance().getTimeStep();
			b->state->vel=deltaSe3.position/dt;
			AngleAxisr aa(angleAxisFromQuat(deltaSe3.orientation)); aa.axis().normalize();
			b->state->angVel=aa.axis()*aa.angle()/dt;
			LOG_DEBUG("Angular velocity set to "<<aa.axis()*aa.angle()/dt<<". Axis="<<aa.axis()<<", angle="<<aa.angle());
		}
		if(!setVelocities || (setVelocities && !b->isDynamic)){
			b->state->pos+=deltaSe3.position;
			b->state->ori=deltaSe3.orientation*b->state->ori;
		}
	}
}

