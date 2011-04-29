#include<yade/core/Engine.hpp>
#include<yade/core/Scene.hpp>
#include<yade/lib/pyutil/gil.hpp>

CREATE_LOGGER(Engine);

void Engine::action(){
	LOG_FATAL("Engine "<<getClassName()<<" calling virtual method Engine::action(). Please submit bug report at http://bugs.launchpad.net/yade.");
	throw std::logic_error("Engine::action() called.");
}

void Engine::explicitAction(){
	scene=Omega::instance().getScene().get(); action();
}

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

void PyRunner::action(){ if(command.size()>0) pyRunString(command); }

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
	t=python::tuple();
};
