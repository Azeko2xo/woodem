#include<yade/core/Scene.hpp>
#include<yade/core/Engine.hpp>
#include<yade/core/Field.hpp>
#include<yade/core/Timing.hpp>

#include<yade/lib/base/Math.hpp>
#include<boost/foreach.hpp>
#include<boost/date_time/posix_time/posix_time.hpp>
#include<boost/algorithm/string.hpp>


// POSIX-only
#include<pwd.h>
#include<unistd.h>
#include<time.h>

namespace py=boost::python;

YADE_PLUGIN(core,(Scene));
CREATE_LOGGER(Scene);
// should be elsewhere, probably
bool TimingInfo::enabled=false;

std::string Scene::pyTagsProxy::getItem(const std::string& key){ return scene->tags[key]; }
void Scene::pyTagsProxy::setItem(const std::string& key,const std::string& val){ scene->tags[key]=val; }
void Scene::pyTagsProxy::delItem(const std::string& key){ size_t i=scene->tags.erase(key); if(i==0) yade::KeyError(key); }
py::list Scene::pyTagsProxy::keys(){ py::list ret; FOREACH(Scene::StrStrMap::value_type& i, scene->tags) ret.append(i.first); return ret; }
bool Scene::pyTagsProxy::has_key(const std::string& key){ return scene->tags.count(key)>0; }

void Scene::fillDefaultTags(){
	// fill default tags
	struct passwd* pw;
	char hostname[HOST_NAME_MAX];
	gethostname(hostname,HOST_NAME_MAX);
	pw=getpwuid(geteuid()); if(!pw) throw runtime_error("getpwuid(geteuid()) failed!");
	// a few default tags
	// real name: will have all non-ASCII characters replaced by ? since serialization doesn't handle that
	// the standard GECOS format is Real Name,,, - first comma and after will be discarded
	string gecos(pw->pw_gecos), gecos2; size_t p=gecos.find(","); if(p!=string::npos) boost::algorithm::erase_tail(gecos,gecos.size()-p); for(size_t i=0;i<gecos.size();i++){gecos2.push_back(((unsigned char)gecos[i])<128 ? gecos[i] : '?'); }
	tags["author"]=boost::algorithm::replace_all_copy(gecos2+" ("+string(pw->pw_name)+"@"+hostname+")"," ","~");
	tags["isoTime"]=boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());
	string id=boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())+"p"+lexical_cast<string>(getpid());
	tags["id"]=id;
	tags["d.id"]=id;
	tags["id.d"]=id;
	// tags.push_back("revision="+py::extract<string>(py::import("yade.config").attr("revision"))());;
}


vector<shared_ptr<Engine> > Scene::pyEnginesGet(void){ return _nextEngines.empty()?engines:_nextEngines; }

void Scene::pyEnginesSet(const vector<shared_ptr<Engine> >& e){
	if(subStep<0) engines=e;
	else _nextEngines=e;
	postLoad(*this);
}


shared_ptr<ScalarRange> Scene::getRange(const std::string& l) const{
	FOREACH(const shared_ptr<ScalarRange>& r, ranges) if(r->label==l) return r;
	throw std::runtime_error("No range labeled `"+l+"'.");
}



void Scene::postLoad(Scene&){
	// assign fields to engines
	FOREACH(const shared_ptr<Engine>& e, engines){
		//cerr<<e->getClassName()<<endl;
		e->scene=this;
		e->setField();
	}
	// manage labeled engines
	typedef std::map<std::string,py::object> StrObjMap; StrObjMap m;
	FOREACH(const shared_ptr<Engine>& e, engines){
		Engine::handlePossiblyLabeledObject(e,m);
		e->getLabeledObjects(m);
	}
	py::scope yadeScope(py::import("yade"));
	// py::scope foo(yadeScope);
	FOREACH(StrObjMap::value_type& v, m){
		// cout<<"Label: "<<v.first<<endl;
		yadeScope.attr(v.first.c_str())=v.second;
	}
	// delete labels which are no longer used
	// py::delattr(yadeScope,name);
}



void Scene::moveToNextTimeStep(){
	if(runInternalConsistencyChecks){
		runInternalConsistencyChecks=false;
		// checkStateTypes();
	}
	// substepping or not, update engines from _nextEngines, if defined, at the beginning of step
	// subStep can be 0, which happens if simulations is saved in the middle of step (without substepping)
	// this assumes that prologue will not set _nextEngines, which is safe hopefully
	if(!_nextEngines.empty() && (subStep<0 || (subStep<=0 && !subStepping))){
		engines=_nextEngines;
		_nextEngines.clear();
		postLoad(*this); // setup labels, check fields etc
		// hopefully this will not break in some margin cases (subStepping with setting _nextEngines and such)
		subStep=-1;
	}
	if(likely(!subStepping && subStep<0)){
		/* set substep to 0 during the loop, so that engines/nextEngines handler know whether we are inside the loop currently */
		subStep=0;
		// ** 1. ** prologue
		if(isPeriodic) cell->integrateAndUpdate(dt);
		if(trackEnergy) energy->resetResettables();
		const bool TimingInfo_enabled=TimingInfo::enabled; // cache the value, so that when it is changed inside the step, the engine that was just running doesn't get bogus values
		TimingInfo::delta last=TimingInfo::getNow(); // actually does something only if TimingInfo::enabled, no need to put the condition here
		// ** 2. ** engines
		FOREACH(const shared_ptr<Engine>& e, engines){
			e->scene=this;
			if(!e->field && e->needsField()) throw std::runtime_error((getClassName()+" has no field to run on, but requires one.").c_str());
			if(e->dead || !e->isActivated()) continue;
			e->run();
			if(unlikely(TimingInfo_enabled)) {TimingInfo::delta now=TimingInfo::getNow(); e->timingInfo.nsec+=now-last; e->timingInfo.nExec+=1; last=now;}
		}
		// ** 3. ** epilogue
		step++;
		time+=dt;
		subStep=-1;
	} else {
		/* IMPORTANT: take care to copy EXACTLY the same sequence as is in the block above !! */
		if(TimingInfo::enabled){ TimingInfo::enabled=false; LOG_INFO("O.timingEnabled disabled, since O.subStepping is used."); }
		if(subStep<-1 || subStep>(int)engines.size()){ LOG_ERROR("Invalid value of Scene::subStep ("<<subStep<<"), setting to -1 (prologue will be run)."); subStep=-1; }
		// if subStepping is disabled, it means we have not yet finished last step completely; in that case, do that here by running all remaining substeps at once
		// if subStepping is enabled, just run the step we need (the loop is traversed only once, with subs==subStep)
		int maxSubStep=subStep;
		if(!subStepping){ maxSubStep=engines.size(); LOG_INFO("Running remaining sub-steps ("<<subStep<<"…"<<maxSubStep<<") before disabling sub-stepping."); }
		for(int subs=subStep; subs<=maxSubStep; subs++){
			assert(subs>=-1 && subs<=(int)engines.size());
			// ** 1. ** prologue
			if(subs==-1){
				if(isPeriodic) cell->integrateAndUpdate(dt);
				if(trackEnergy) energy->resetResettables();
			}
			// ** 2. ** engines
			else if(subs>=0 && subs<(int)engines.size()){
				const shared_ptr<Engine>& e(engines[subs]);
				e->scene=this;
				if(!e->field && e->needsField()) throw std::runtime_error((getClassName()+" has no field to run on, but requires one.").c_str());
				if(!e->dead && e->isActivated()) e->run();
			}
			// ** 3. ** epilogue
			else if(subs==(int)engines.size()){ step++; time+=dt; /* gives -1 along with the increment afterwards */ subStep=-2; }
			// (?!)
			else { /* never reached */ assert(false); }
		}
		subStep++; // if not substepping, this will make subStep=-2+1=-1, which is what we want
	}
}



#if 0
void Scene::checkStateTypes(){
	FOREACH(const shared_ptr<Body>& b, *bodies){
		if(!b || !b->material) continue;
		if(b->material && !b->state) throw std::runtime_error("Body #"+lexical_cast<string>(b->getId())+": has Body::material, but NULL Body::state.");
		if(!b->material->stateTypeOk(b->state.get())){
			throw std::runtime_error("Body #"+lexical_cast<string>(b->getId())+": Body::material type "+b->material->getClassName()+" doesn't correspond to Body::state type "+b->state->getClassName()+" (should be "+b->material->newAssocState()->getClassName()+" instead).");
		}
	}
}

void Scene::updateBound(){
	if(!bound) bound=shared_ptr<Bound>(new Bound);
	const Real& inf=std::numeric_limits<Real>::infinity();
	Vector3r mx(-inf,-inf,-inf);
	Vector3r mn(inf,inf,inf);
	FOREACH(const shared_ptr<Body>& b, *bodies){
		if(!b) continue;
		if(b->bound){
			for(int i=0; i<3; i++){
				if(!isinf(b->bound->max[i])) mx[i]=max(mx[i],b->bound->max[i]);
				if(!isinf(b->bound->min[i])) mn[i]=min(mn[i],b->bound->min[i]);
			}
		} else {
	 		mx=mx.cwise().max(b->state->pos);
 			mn=mn.cwise().min(b->state->pos);
		}
	}
	bound->min=mn; bound->max=mx;
}
#endif
