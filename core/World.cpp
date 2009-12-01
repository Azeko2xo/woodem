/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include"World.hpp"
#include<yade/core/Engine.hpp>
#include<yade/core/Timing.hpp>
#include<yade/core/TimeStepper.hpp>

#include<Wm3Math.h>
#include<yade/lib-base/yadeWm3.hpp>
#include<boost/foreach.hpp>
#include<boost/date_time/posix_time/posix_time.hpp>
#include<boost/algorithm/string.hpp>


/* this is meant to improve usability: World is ready by default (so is Omega by that token)
 * and different type of containers can still be used instead by explicit assignment */
#include<yade/core/BodyVector.hpp>
#include<yade/core/InteractionVecMap.hpp>

// POSIX-only
#include<pwd.h>
#include<unistd.h>
#include<time.h>

// should be elsewhere, probably
bool TimingInfo::enabled=false;

World::World() :
	Body(), bodies(new BodyVector), interactions(new InteractionVecMap){	
	engines.clear();
	initializers.clear();
	needsInitializers=true;
	currentIteration=0;
	simulationTime=0;
	stopAtIteration=0;
	stopAtRealTime=0; // not yet implemented
	stopAtVirtTime=0; // not yet implemented either
	isDynamic=false;
	dt=1e-8;
	selectedBody=-1;
	isPeriodic=false;
	// FIXME: move MetaInteractingGeometry to core and create it here right away
	// interactingGeometry=shared_ptr<InteractingGeometry>(new MetaInteractingGeometry);
	material=shared_ptr<Material>(new Material);

	// fill default tags
	struct passwd* pw;
	char hostname[HOST_NAME_MAX];
	gethostname(hostname,HOST_NAME_MAX);
	pw=getpwuid(geteuid()); if(!pw) throw runtime_error("getpwuid(geteuid()) failed!");
	// a few default tags
	// real name: will have all non-ASCII characters replaced by ? since serialization doesn't handle that
	// the standard GECOS format is Real Name,,, - first command and after will be discarded
	string gecos(pw->pw_gecos), gecos2; size_t p=gecos.find(","); if(p!=string::npos) boost::algorithm::erase_tail(gecos,gecos.size()-p); for(size_t i=0;i<gecos.size();i++){gecos2.push_back(((unsigned char)gecos[i])<128 ? gecos[i] : '?'); }
	tags.push_back(boost::algorithm::replace_all_copy(string("author=")+gecos2+" ("+string(pw->pw_name)+"@"+hostname+")"," ","~"));
	tags.push_back(string("isoTime="+boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())));
	tags.push_back(string("id="+boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())+"p"+lexical_cast<string>(getpid())));
	tags.push_back(string("description="));
}



void World::postProcessAttributes(bool deserializing){
	/* since yade::serialization doesn't properly handle shared pointers, iterate over all bodies and make materials shared again, if id>=0 */
	FOREACH(const shared_ptr<Body>& b, *bodies){
		if(!b->material || b->material->id<0) continue; // not a shared material
		assert(b->material->id < (int)materials.size());
		b->material=materials[b->material->id];
	}
}



void World::moveToNextTimeStep(){
	if(needsInitializers){
		checkStateTypes();
		FOREACH(shared_ptr<Engine> e, initializers){ e->world=this; if(!e->isActivated(this)) continue;e->action(this); } 
		bex.resize(bodies->size());
		needsInitializers=false;
	}
	//bex.reset(); // uncomment if PhysicalActionContainerReseter is removed
	bool TimingInfo_enabled=TimingInfo::enabled; // cache the value, so that when it is changed inside the step, the engine that was just running doesn't get bogus values
	TimingInfo::delta last=TimingInfo::getNow(); // actually does something only if TimingInfo::enabled, no need to put the condition here
	FOREACH(const shared_ptr<Engine>& e, engines){
		e->world=this;
		if(!e->isActivated(this)) continue;
		e->action(this);
		if(TimingInfo_enabled) {TimingInfo::delta now=TimingInfo::getNow(); e->timingInfo.nsec+=now-last; e->timingInfo.nExec+=1; last=now;}
	}
	currentIteration++;
	simulationTime+=dt;
}

shared_ptr<Engine> World::engineByName(string s){
	FOREACH(shared_ptr<Engine> e, engines){
		if(e->getClassName()==s) return e;
	}
	return shared_ptr<Engine>();
}

shared_ptr<Engine> World::engineByLabel(string s){
	FOREACH(shared_ptr<Engine> e, engines){
		if(e->label==s) return e;
	}
	return shared_ptr<Engine>();
}

void World::setTimeSteppersActive(bool a)
{
	FOREACH(shared_ptr<Engine> e, engines){
		if (Omega::instance().isInheritingFrom(e->getClassName(),"TimeStepper"))
			(dynamic_pointer_cast<TimeStepper>(e))->setActive(a);
	}
}

void World::checkStateTypes(){
	FOREACH(const shared_ptr<Body>& b, *bodies){
		if(!b || !b->material) continue;
		if(b->material && !b->state) throw std::runtime_error("Body #"+lexical_cast<string>(b->getId())+": has Body::material, but NULL Body::state.");
		if(!b->material->stateTypeOk(b->state.get())){
			throw std::runtime_error("Body #"+lexical_cast<string>(b->getId())+": Body::material type "+b->material->getClassName()+" doesn't correspond to Body::state type "+b->state->getClassName()+" (should be "+b->material->newAssocState()->getClassName()+" instead).");
		}
	}
}
