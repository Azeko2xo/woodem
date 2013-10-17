#include<woo/core/Engine.hpp>
#include<woo/core/Scene.hpp>
#include<woo/core/Field.hpp>

#include<woo/lib/pyutil/gil.hpp>

CREATE_LOGGER(Engine);

void Engine::run(){ throw std::logic_error((getClassName()+" did not override Engine::run()").c_str()); } 

void Engine::explicitRun(){ scene=Master::instance().getScene().get(); if(!field) setField(); run(); }

void Engine::setDefaultScene(){ scene=Master::instance().getScene().get(); }

void Engine::setField(){
	if(userAssignedField) return; // do nothing in this case
	if(!needsField()) return; // no field required, do nothing
	vector<shared_ptr<Field> > accepted;
	FOREACH(const shared_ptr<Field> f, scene->fields){ if(acceptsField(f.get())) accepted.push_back(f); }
	if(accepted.size()>1){
		string err="Engine "+getClassName()+" accepted "+boost::lexical_cast<string>(accepted.size())+" fields to run on:";
		FOREACH(const shared_ptr<Field>& f,accepted) err+=" "+f->getClassName();
		err+=". Only one field is allowed; this ambiguity can be resolved by setting the field attribute.";
		throw std::runtime_error(err); 
	}
	if(accepted.empty()) throw std::runtime_error(("Engine "+getClassName()+" accepted no field to run on; remove it from engines.").c_str()); 
	field=accepted[0];
}

void Engine::postLoad(Engine&, void* addr){
	if(addr==NULL){
		if(isNewObject && field){ userAssignedField=true; }
		isNewObject=false;
	}
	if(addr==(void*)&dead){
		notifyDead();
	}
}

shared_ptr<Field> Engine::field_get(){ return field; }

void Engine::field_set(const shared_ptr<Field>& f){
	if(!f) { setField(); userAssignedField=false; }
	else{ field=f; userAssignedField=true; }
}

void Engine::runPy(const string& command){
	if(command.empty()) return;
	#if 0
		pyRunString(command);
	#else
		GilLock lock;
		try{
			// scripts are run in this namespace (wooMain)
			py::object global(py::import("wooMain").attr("__dict__"));
			py::dict local;
			local["scene"]=py::object(py::ptr(scene));
			local["S"]=py::object(py::ptr(scene));
			local["engine"]=py::object(py::ptr(this));
			local["field"]=py::object(field);
			local["woo"]=py::import("woo");
			// local["wooExtra"]=py::import("wooExtra"); // FIXME: not always importable
			py::exec(command.c_str(),global,local);
		} catch (py::error_already_set& e){
			throw std::runtime_error("PyRunner exception in '"+command+"':\n"+parsePythonException_gilLocked());
		};
	#endif
};


void ParallelEngine::setField(){
	for(vector<shared_ptr<Engine>>& grp: slaves){
		for(const shared_ptr<Engine>& e: grp){
			e->scene=scene;
			e->setField();
		}
	}
}



void ParallelEngine::run(){
	// openMP warns if the iteration variable is unsigned...
	const int size=(int)slaves.size();
	#ifdef WOO_OPENMP
		#pragma omp parallel for
	#endif
	for(int i=0; i<size; i++){
		// run every slave group sequentially
		for(const shared_ptr<Engine>& e: slaves[i]){
			//cerr<<"["<<omp_get_thread_num()<<":"<<e->getClassName()<<"]";
			e->scene=scene;
			if(!e->field && e->needsField()) throw std::runtime_error((getClassName()+" has no field to run on, but requires one.").c_str());
			if(!e->dead && e->isActivated()){ e->run(); }
		}
	}
}

void ParallelEngine::pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){
	if(py::len(args)==0) return;
	if(py::len(args)>1) woo::TypeError("ParallelEngine takes 0 or 1 non-keyword arguments ("+boost::lexical_cast<string>(py::len(args))+" given)");
	py::extract<py::list> listEx(args[0]);
	if(!listEx.check()) woo::TypeError("ParallelEngine: non-keyword argument must be a list");
	pySlavesSet(listEx());
	args=py::tuple();
}

void ParallelEngine::pySlavesSet(const py::list& slaves2){
	int len=py::len(slaves2);
	slaves.clear();
	for(int i=0; i<len; i++){
		py::extract<std::vector<shared_ptr<Engine> > > serialGroup(slaves2[i]);
		if (serialGroup.check()){ slaves.push_back(serialGroup()); continue; }
		py::extract<shared_ptr<Engine> > serialAlone(slaves2[i]);
		if (serialAlone.check()){ vector<shared_ptr<Engine> > aloneWrap; aloneWrap.push_back(serialAlone()); slaves.push_back(aloneWrap); continue; }
		woo::TypeError("List elements must be either (a) sequences of engines to be executed one after another (b) individual engines.");
	}
}

py::list ParallelEngine::pySlavesGet(){
	py::list ret;
	for(vector<shared_ptr<Engine>>& grp: slaves){
		if(grp.size()==1) ret.append(py::object(grp[0]));
		else ret.append(py::object(grp));
	}
	return ret;
}

void ParallelEngine::getLabeledObjects(const shared_ptr<LabelMapper>& labelMapper){
	for(vector<shared_ptr<Engine> >& grp: slaves) for(const shared_ptr<Engine>& e: grp) Engine::handlePossiblyLabeledObject(e,labelMapper);
	Engine::getLabeledObjects(labelMapper);
};

void PeriodicEngine::fakeRun(){
	const Real& virtNow=scene->time;
	Real realNow=getClock();
	const long& stepNow=scene->step;
	realPrev=realLast; realLast=realNow;
	virtPrev=virtLast; virtLast=virtNow;
	stepPrev=stepLast; stepLast=stepNow;
}

bool PeriodicEngine::isActivated(){
	const Real& virtNow=scene->time;
	Real realNow=getClock();
	const long& stepNow=scene->step;
	// we run for the very first time here, initialize counters
	bool initNow=(stepLast<0);
	if(initNow){ 
		realLast=realNow; virtLast=virtNow; stepLast=stepNow;
	}
	if(
		(nDo<0 || nDone<nDo) &&
		(	(virtPeriod>0 && virtNow-virtLast>=virtPeriod) ||
		 	(realPeriod>0 && realNow-realLast>=realPeriod) ||
		 	(stepPeriod>0 && ((stepNow-stepLast>=stepPeriod) || (stepModulo && stepNow%stepPeriod==0)))
		)
	){
		// we would run now, but if not desired, don't
		realPrev=realLast; realLast=realNow;
		virtPrev=virtLast; virtLast=virtNow;
		stepPrev=stepLast; stepLast=stepNow;
		// skip the initial run, but have all values of when last run set properly
		if(initNow && !initRun) { return false; }
		nDone++;
		return true;
	}
	return false;
}

void PyRunner::pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){
	bool cmdDone(false),stepDone(false);
	for(int i=0; i<py::len(t); i++){
		py::extract<string> exStr(t[i]);
		py::extract<int> exInt(t[i]);
		if(exStr.check()){
			if(!cmdDone) command=exStr();
			else throw std::invalid_argument(("PyRunner.command was already specified (extra unnamed string argument, at position "+lexical_cast<string>(i)+")").c_str());
			cmdDone=true;
			continue;
		} 
		if(exInt.check()){
			if(!stepDone) stepPeriod=exInt();
			else throw std::invalid_argument(("PyRunner.stepPeriod was already specified (extra unnamed int argument, at position "+lexical_cast<string>(i)+")").c_str());
			stepDone=true;
			continue;
		}
	}
	t=py::tuple();
};
