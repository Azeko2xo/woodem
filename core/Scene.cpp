#include<yade/core/Scene.hpp>
#include<yade/core/Engine.hpp>
#include<yade/core/Field.hpp>
#include<yade/core/Timing.hpp>
#include<yade/lib/object/ObjectIO.hpp>

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
py::list Scene::pyTagsProxy::keys(){ py::list ret; FOREACH(Scene::StrStrMap::value_type i, scene->tags) ret.append(i.first); return ret; }
bool Scene::pyTagsProxy::has_key(const std::string& key){ return scene->tags.count(key)>0; }

void Scene::fillDefaultTags(){
	// fill default tags
	struct passwd* pw;
	char hostname[HOST_NAME_MAX];
	gethostname(hostname,HOST_NAME_MAX);
	pw=getpwuid(geteuid()); if(!pw) throw runtime_error("getpwuid(geteuid()) failed!");
	// a few default tags
	// the standard GECOS format is Real Name,,, - first comma and after will be discarded
	string gecos(pw->pw_gecos); size_t p=gecos.find(","); if(p!=string::npos) boost::algorithm::erase_tail(gecos,gecos.size()-p);
	// no need to remove non-ascii anymore
	// for(size_t i=0;i<gecos.size();i++){gecos2.push_back(((unsigned char)gecos[i])<128 ? gecos[i] : '?'); }
	tags["user"]=gecos+" ("+string(pw->pw_name)+"@"+hostname+")";
	tags["isoTime"]=boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());
	string id=boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())+"p"+lexical_cast<string>(getpid());
	tags["id"]=id;
	tags["d.id"]=id;
	tags["id.d"]=id;
	// tags.push_back("revision="+py::extract<string>(py::import("yade.config").attr("revision"))());;
}

void Scene::ensureCl(){
	#ifdef YADE_OPENCL
		if(_clDev[0]<0) initCl(); // no device really initialized
		return;
	#else
		throw std::runtime_error("Yade was compiled without OpenCL support (add to features and recompile).");
	#endif
}

#ifdef YADE_OPENCL
void Scene::initCl(){
	Vector2i dev=(clDev[0]<0?Omega::instance().defaultClDev:clDev);
	clDev=Vector2i(-1,-1); // invalidate old settings before attempting new
	int pNum=dev[0], dNum=dev[1];
	std::vector<cl::Platform> platforms;
	std::vector<cl::Device> devices;
	cl::Platform::get(&platforms);
	if(pNum<0){
		LOG_WARN("OpenCL device will be chosen automatically; set --cl-dev or Scene.clDev to override (and get rid of this warning)");
		LOG_WARN("==== OpenCL devices: ====");
		for(size_t i=0; i<platforms.size(); i++){
			LOG_WARN("   "<<i<<". platform: "<<platforms[i].getInfo<CL_PLATFORM_NAME>());
			platforms[i].getDevices(CL_DEVICE_TYPE_ALL,&devices);
			for(size_t j=0; j<devices.size(); j++){
				LOG_WARN("      "<<j<<". device: "<<devices[j].getInfo<CL_DEVICE_NAME>());
			}
		}
		LOG_WARN("==== --------------- ====");
	}
	cl::Platform::get(&platforms);
	if(platforms.empty()){ throw std::runtime_error("No OpenCL platforms available."); }
	if(pNum>=(int)platforms.size()){ LOG_WARN("Only "<<platforms.size()<<" platforms available, taking 0th platform."); pNum=0; }
	if(pNum<0) pNum=0;
	platform=make_shared<cl::Platform>(platforms[pNum]);
	platform->getDevices(CL_DEVICE_TYPE_ALL,&devices);
	if(devices.empty()){ throw std::runtime_error("No OpenCL devices available on the platform "+platform->getInfo<CL_PLATFORM_NAME>()+"."); }
	if(dNum>=(int)devices.size()){ LOG_WARN("Only "<<devices.size()<<" devices available, taking 0th device."); dNum=0; }
	if(dNum<0) dNum=0;
	device=make_shared<cl::Device>(devices[dNum]);
	// create context only for one device
	context=make_shared<cl::Context>(vector<cl::Device>({*device}));
	LOG_WARN("OpenCL ready: platform \""<<platform->getInfo<CL_PLATFORM_NAME>()<<"\", device \""<<device->getInfo<CL_DEVICE_NAME>()<<"\".");
	queue=make_shared<cl::CommandQueue>(*context,*device);
	clDev=Vector2i(pNum,dNum);
	_clDev=clDev;
}
#endif


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

void Scene::boostSave(const string& out){
	lastSave=out;
	Object::boostSave(out);
}

void Scene::saveTmp(const string& slot, bool quiet){
	lastSave=":memory:"+slot;
	shared_ptr<Scene> thisPtr(this,null_deleter());
	Omega::instance().saveTmp(thisPtr,slot,/*quiet*/true);
}

void Scene::postLoad(Scene&){
	#ifdef YADE_OPENCL
		// clDev is set and does not match really initialized device in _clDev
		if(clDev[0]!=_clDev[0] || clDev[1]!=_clDev[1]) initCl();
	#else
		if(clDev[0]>=0) ensureCl(); // only throws
	#endif
	//
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
	if(isnan(dt)||dt<0) throw std::runtime_error("Scene::dt is NaN or negative");
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
	FOREACH(const shared_ptr<Field>& f, fields) if(f->scene!=this) f->scene=this;
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
		if(isPeriodic) cell->setNextGradV();
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
			else if(subs==(int)engines.size()){
				if(isPeriodic) cell->setNextGradV();
				step++; time+=dt; /* gives -1 along with the increment afterwards */ subStep=-2;
			}
			// (?!)
			else { /* never reached */ assert(false); }
		}
		subStep++; // if not substepping, this will make subStep=-2+1=-1, which is what we want
	}
}
