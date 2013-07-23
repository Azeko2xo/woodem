#include<woo/core/Scene.hpp>
#include<woo/core/Engine.hpp>
#include<woo/core/Field.hpp>
#include<woo/core/Timing.hpp>
#include<woo/lib/object/ObjectIO.hpp>
#include<woo/lib/pyutil/gil.hpp>

#include<woo/lib/base/Math.hpp>
#include<boost/foreach.hpp>
#include<boost/date_time/posix_time/posix_time.hpp>
#include<boost/algorithm/string.hpp>

#ifndef __MINGW64__
	#include<unistd.h> // getpid
#else
	// avoid namespace pollution - perhaps handled by boost::asio already?
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include<Windows.h> // GetCurrentProcessId, GetComputerName
#endif

namespace py=boost::python;

WOO_PLUGIN(core,(Scene));
CREATE_LOGGER(Scene);
// should be elsewhere, probably
bool TimingInfo::enabled=false;

void Scene::pyRun(long steps, bool wait){
	except.reset();
	if(running()) throw std::runtime_error("Scene.run: already running");
	{
		boost::mutex::scoped_lock l(runMutex);
		if(steps>0) stopAtStep=step+steps;
		/* run really */
		runningFlag=true;
		stopFlag=false;
		boost::function0<void> loop(boost::bind(&Scene::backgroundLoop,this));
		boost::thread th(loop);
		/* runs in separate thread now */
		bgThreadId=th.get_id();
	}
	if(wait) pyWait();
}

void Scene::pyStop(){
	if(!running()) return;
	{boost::mutex::scoped_lock l(runMutex); stopFlag=true; }
}

void Scene::pyOne(){
	except.reset();
	if(running()) throw std::runtime_error("Scene.step: already running.");
	doOneStep();
}

void Scene::pyWait(){
	if(!running()) return;
	Py_BEGIN_ALLOW_THREADS;
		while(running()) boost::this_thread::sleep(boost::posix_time::milliseconds(40));
	Py_END_ALLOW_THREADS;
	// handle possible exception: reset it and rethrow
	if(!except) return;
	std::exception e(*except);
	except.reset();
	throw e;
}

bool Scene::running(){ boost::mutex::scoped_lock l(runMutex); return runningFlag; }

// this function runs in background thread
// exception and threads don't work well, so any exception caught is
// stored and handled in the main thread
void Scene::backgroundLoop(){
	try{
		while(true){
			boost::this_thread::interruption_point();
			if(subStepping){ LOG_INFO("Scene.run: sub-stepping disabled."); subStepping=false; }
			doOneStep();
			if(stopAtStep>0 && stopAtStep==step){ boost::mutex::scoped_lock l(runMutex); stopFlag=true; }
			if(stopFlagSet()){ boost::mutex::scoped_lock l(runMutex); runningFlag=false; return; }
		}
	} catch(std::exception& e){
		LOG_ERROR("Exception: "<<endl<<e.what());
		except=make_shared<std::exception>(e);
		{ boost::mutex::scoped_lock l(runMutex); runningFlag=false; }
		return;
	}
}

void Scene::PausedContextManager::__enter__(){
	// since this function is called from python, other python threads must be allowed to run explicitly
	// otherwise, if the engine thread would be in python code (in PyRunner, for instance),
	// it would not be allowed to be run, therefore the step would not finish and the engine thread would
	// not release the lock, and we would get deadlocked.

	// this fails to detect when called from within engine with S.step() rather than S.run()
	if(boost::this_thread::get_id()==scene->bgThreadId) throw std::runtime_error("Scene.paused() may not be called from the engine thread!");
	#ifdef WOO_LOOP_MUTEX_HELP
		engineLoopMutexWaiting=true;
	#endif
	// boost::timed_mutex::scoped_lock lock(scene->engineLoopMutex,boost::defer_lock());
	Py_BEGIN_ALLOW_THREADS;
		while(!scene->engineLoopMutex.timed_lock(boost::posix_time::seconds(10))){
			LOG_WARN("Waiting for lock for 10 seconds; deadlocked? (Scene.paused() must not be called from within the engine loop, through PyRunner or otherwise.");
		}
	Py_END_ALLOW_THREADS;
	#ifdef WOO_LOOP_MUTEX_HELP
		engineLoopMutexWaiting=false;
	#endif
	LOG_DEBUG("Scene.paused(): locked");
}
// exception information are not used, but we need to accept those args
void Scene::PausedContextManager::__exit__(py::object exc_type, py::object exc_value, py::object traceback){
	scene->engineLoopMutex.unlock();
	LOG_DEBUG("Scene.paused(): unlocked");
}

//py::object Scene::pyTagsProxy::unicodeFromStr(const string& s){
//	return py::object(py::handle<>(PyUnicode_FromString(s.c_str())));
//}

std::string Scene::pyTagsProxy::getItem(const std::string& key){ return scene->tags[key]; }
void Scene::pyTagsProxy::setItem(const std::string& key, const string& val){ scene->tags[key]=val;
}
void Scene::pyTagsProxy::delItem(const std::string& key){ size_t i=scene->tags.erase(key); if(i==0) woo::KeyError(key); }
py::list Scene::pyTagsProxy::keys(){ py::list ret; for(const auto& kv: scene->tags) ret.append(kv.first); return ret; }
py::list Scene::pyTagsProxy::values(){ py::list ret; for(const auto& kv: scene->tags) ret.append(kv.second); return ret; }
py::list Scene::pyTagsProxy::items(){ py::list ret; for(const auto& kv: scene->tags) ret.append(py::make_tuple(kv.first,kv.second)); return ret; }
bool Scene::pyTagsProxy::has_key(const std::string& key){ return scene->tags.count(key)>0; }

void Scene::pyTagsProxy::update(const pyTagsProxy& b){ for(const auto& i: b.scene->tags) scene->tags[i.first]=i.second; }

void Scene::fillDefaultTags(){
	char hostname[256];
	#ifndef __MINGW64__
		int ret=gethostname(hostname,255);
		tags["user"]=getenv("USER")+string("@")+string(ret==0?hostname:"[hostname lookup failed]");
	#else
		// http://msdn.microsoft.com/en-us/library/ms724295%28v=vs.85%29.aspx
		DWORD len=255;
		int ret=GetComputerName(hostname,&len);
		tags["user"]=getenv("USERNAME")+string("@")+string(ret!=0?hostname:"[hostname lookup failed]");
	#endif
		
	tags["isoTime"]=boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());
	string id=boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())+"p"+lexical_cast<string>(
	#ifndef __MINGW64__
		getpid()
	#else
		GetCurrentProcessId()
	#endif
	);
	tags["id"]=id;
	// no title, use empty
	tags["title"]="";
	tags["idt"]=tags["tid"]=id;
	//tags["d.id"]=tags["id.d"]=tags["d_id"]=tags["id_d"]=id;
	// tags.push_back("revision="+py::extract<string>(py::import("woo.config").attr("revision"))());;
	#ifdef WOO_LOOP_MUTEX_HELP
		// initialize that somewhere
		engineLoopMutexWaiting=false;	
	#endif
}


string Scene::expandTags(const string& s) const{
	// nothing to expand, just return
	if(s.find("{")==string::npos) return s;
	string s2(s);
	for(auto& keyVal: tags){
		boost::algorithm::replace_all(s2,"{"+keyVal.first+"}",keyVal.second);
	}
	return s2;
}

void Scene::ensureCl(){
	#ifdef WOO_OPENCL
		if(_clDev[0]<0) initCl(); // no device really initialized
		return;
	#else
		throw std::runtime_error("Yade was compiled without OpenCL support (add to features and recompile).");
	#endif
}

#ifdef WOO_OPENCL
void Scene::initCl(){
	Vector2i dev=(clDev[0]<0?Master::instance().defaultClDev:clDev);
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
	postLoad(*this,(void*)&engines);
}

#ifdef WOO_OPENGL
shared_ptr<ScalarRange> Scene::getRange(const std::string& l) const{
	for(const shared_ptr<ScalarRange>& r: ranges) if(r->label==l) return r;
	throw std::runtime_error("No range labeled `"+l+"'.");
}
#endif

void Scene::boostSave(const string& out){
	lastSave=out;
	Object::boostSave(out);
}

void Scene::saveTmp(const string& slot, bool quiet){
	lastSave=":memory:"+slot;
	Master::instance().saveTmp(static_pointer_cast<Scene>(shared_from_this()),slot,/*quiet*/true);
}

void Scene::postLoad(Scene&,void*){
	if(!clock0adjusted){
		clock0-=boost::posix_time::seconds(preSaveDuration);
		clock0adjusted=true;
	}
	#ifdef WOO_OPENCL
		// clDev is set and does not match really initialized device in _clDev
		if(clDev[0]!=_clDev[0] || clDev[1]!=_clDev[1]) initCl();
	#else
		if(clDev[0]>=0) ensureCl(); // only throws
	#endif
	if(!plot){
		LOG_WARN("Scene.plot==None is disallowed, assigning woo.core.Plot().");
		plot=make_shared<Plot>();
	}

	//
	// assign fields to engines
	int i=0;
	for(const shared_ptr<Engine>& e: engines){
		if(!e) throw std::runtime_error("Scene.engines["+to_string(i)+"]==None (not allowed).");
		//cerr<<e->getClassName()<<endl;
		e->scene=this;
		e->setField();
		i++;
	}
	// TODO: this can be removed, as labels should be saved just fine
	// manage labeled engines
	for(const shared_ptr<Engine>& e: engines){
		Engine::handlePossiblyLabeledObject(e,labels);
		e->getLabeledObjects(labels);
	}
}

void Scene::selfTest_maybe(bool force){
	if(!force && ((selfTestEvery<0) || (selfTestEvery>0 && (step%selfTestEvery!=0)) || (selfTestEvery==0 && step!=0))) return;
	LOG_INFO("Running self-tests at step "<<step<<" (selfTestEvery=="<<selfTestEvery<<", force="<<force<<")");
	try{
		for(const auto& f: fields){
			f->scene=this;
			if(!f) throw std::runtime_error("Scene.fields may not contain None.");
			f->selfTest();
		}
		for(const auto& e: engines){
			if(!e) throw std::runtime_error("Scene.engines may not contain None.");
			e->scene=this;
			if(!e->field && e->needsField()) throw std::runtime_error((getClassName()+" has no field to run on, but requires one.").c_str());
			e->selfTest();
		}
	} catch(std::exception&) {
		if(!force) LOG_ERROR("selfTest failed (step="<<step<<", selfTestEvery="<<selfTestEvery<<", force="<<force<<").");
		throw;
	};
};


void Scene::doOneStep(){
	#ifdef WOO_LOOP_MUTEX_HELP
		// add some daly to help the other thread locking the mutex; will be removed once 
		if(engineLoopMutexWaiting){
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		}
	#endif
	boost::timed_mutex::scoped_lock lock(engineLoopMutex);

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
		postLoad(*this,NULL); // setup labels, check fields etc
		// hopefully this will not break in some margin cases (subStepping with setting _nextEngines and such)
		subStep=-1;
	}
	for(const shared_ptr<Field>& f: fields) if(f->scene!=this) f->scene=this;
	if(likely(!subStepping && subStep<0)){
		/* set substep to 0 during the loop, so that engines/nextEngines handler know whether we are inside the loop currently */
		subStep=0;
		// ** 1. ** prologue
		selfTest_maybe();
		if(isPeriodic) cell->integrateAndUpdate(dt);
		if(trackEnergy) energy->resetResettables();
		const bool TimingInfo_enabled=TimingInfo::enabled; // cache the value, so that when it is changed inside the step, the engine that was just running doesn't get bogus values
		TimingInfo::delta last=TimingInfo::getNow(); // actually does something only if TimingInfo::enabled, no need to put the condition here
		// ** 2. ** engines
		for(const shared_ptr<Engine>& e: engines){
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
				selfTest_maybe();
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
