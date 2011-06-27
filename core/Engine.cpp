#include<yade/core/Engine.hpp>
#include<yade/core/Scene.hpp>
#include<yade/core/Field.hpp>

#include<yade/lib/pyutil/gil.hpp>

CREATE_LOGGER(Engine);

void Engine::run(){ throw std::logic_error((getClassName()+" did not override Engine::run()").c_str()); } 

void Engine::explicitRun(){ scene=Omega::instance().getScene().get(); if(!field) setField(); run(); }

void Engine::setField(){
	if(userAssignedField) return; // do nothing in this case
	if(!needsField()) return; // no field required, do nothing
	vector<shared_ptr<Field> > accepted;
	FOREACH(const shared_ptr<Field> f, scene->fields){ if(acceptsField(f.get())) accepted.push_back(f); }
	if(accepted.size()>1) throw std::runtime_error(("Engine "+getClassName()+" accepted "+lexical_cast<string>(accepted.size())+" fields to run on. Only one field is allowed; this ambiguity can be resolved by setting the field attribute.").c_str()); 
	if(accepted.empty()) throw std::runtime_error(("Engine "+getClassName()+" accepted no field to run on; remove it from engines.").c_str()); 
	field=accepted[0];
}

void Engine::postLoad(Engine&){
	if(isNewObject && field){ userAssignedField=true; }
	isNewObject=false;
}

shared_ptr<Field> Engine::field_get(){ return field; }

void Engine::field_set(const shared_ptr<Field>& f){
	if(!f) { setField(); userAssignedField=false; }
	else{ field=f; userAssignedField=true; }
}


void ParallelEngine::setField(){
	FOREACH(vector<shared_ptr<Engine> >& grp, slaves){
		FOREACH(const shared_ptr<Engine>& e, grp){
			e->scene=scene;
			e->setField();
		}
	}
}


void ParallelEngine::run(){
	// openMP warns if the iteration variable is unsigned...
	const int size=(int)slaves.size();
	#ifdef YADE_OPENMP
		#pragma omp parallel for
	#endif
	for(int i=0; i<size; i++){
		// run every slave group sequentially
		FOREACH(const shared_ptr<Engine>& e, slaves[i]) {
			//cerr<<"["<<omp_get_thread_num()<<":"<<e->getClassName()<<"]";
			e->scene=scene;
			if(!e->field && e->needsField()) throw std::runtime_error((getClassName()+" has no field to run on, but requires one.").c_str());
			if(!e->dead && e->isActivated()){ e->run(); }
		}
	}
}

void ParallelEngine::pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){
	if(py::len(args)==0) return;
	if(py::len(args)>1) yade::TypeError("ParallelEngine takes 0 or 1 non-keyword arguments ("+lexical_cast<string>(py::len(args))+" given)");
	py::extract<py::list> listEx(args[0]);
	if(!listEx.check()) yade::TypeError("ParallelEngine: non-keyword argument must be a list");
	pySlavesSet(listEx());
}

void ParallelEngine::pySlavesSet(const py::list& slaves2){
	int len=py::len(slaves2);
	slaves.clear();
	for(int i=0; i<len; i++){
		py::extract<std::vector<shared_ptr<Engine> > > serialGroup(slaves2[i]);
		if (serialGroup.check()){ slaves.push_back(serialGroup()); continue; }
		py::extract<shared_ptr<Engine> > serialAlone(slaves2[i]);
		if (serialAlone.check()){ vector<shared_ptr<Engine> > aloneWrap; aloneWrap.push_back(serialAlone()); slaves.push_back(aloneWrap); continue; }
		yade::TypeError("List elements must be either (a) sequences of engines to be executed one after another (b) individual engines.");
	}
}

py::list ParallelEngine::pySlavesGet(){
	py::list ret;
	FOREACH(vector<shared_ptr<Engine > >& grp, slaves){
		if(grp.size()==1) ret.append(py::object(grp[0]));
		else ret.append(py::object(grp));
	}
	return ret;
}

void ParallelEngine::getLabeledObjects(std::map<std::string,py::object>& m){
	FOREACH(vector<shared_ptr<Engine> >& grp, slaves) FOREACH(const shared_ptr<Engine>& e, grp) Engine::handlePossiblyLabeledObject(e,m);
	Engine::getLabeledObjects(m);
};



bool PeriodicEngine::isActivated(){
	const Real& virtNow=scene->time;
	Real realNow=getClock();
	const long& stepNow=scene->step;
	if((nDo<0 || nDone<nDo) &&
		((virtPeriod>0 && virtNow-virtLast>=virtPeriod) ||
		 (realPeriod>0 && realNow-realLast>=realPeriod) ||
		 (stepPeriod>0 && stepNow-stepLast>=stepPeriod))){
		realLast=realNow; virtLast=virtNow; stepLast=stepNow; nDone++;
		return true;
	}
	if(nDone==0){
		realLast=realNow; virtLast=virtNow; stepLast=stepNow; nDone++;
		if(initRun) return true;
		return false;
	}
	return false;
}

void PyRunner::run(){ if(command.size()>0) pyRunString(command); }

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
