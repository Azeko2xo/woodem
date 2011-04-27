/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#include<yade/core/BgThread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>


void ThreadWorker::setTerminate(bool b){ boost::mutex::scoped_lock lock(m_mutex); m_should_terminate=b; };
bool ThreadWorker::shouldTerminate(){ boost::mutex::scoped_lock lock(m_mutex); return m_should_terminate;};
void ThreadWorker::setProgress(float i){ boost::mutex::scoped_lock lock(m_mutex); m_progress=i; };
void ThreadWorker::setStatus(std::string s){ boost::mutex::scoped_lock lock(m_mutex); m_status=s; };
float ThreadWorker::progress(){ boost::mutex::scoped_lock lock(m_mutex); return m_progress; };
std::string ThreadWorker::getStatus(){ boost::mutex::scoped_lock lock(m_mutex); return m_status;};
void ThreadWorker::setReturnValue(boost::any a){ boost::mutex::scoped_lock lock(m_mutex); m_val = a; };
boost::any ThreadWorker::getReturnValue(){ boost::mutex::scoped_lock lock(m_mutex); return m_val; };
bool ThreadWorker::done(){ boost::mutex::scoped_lock lock(m_mutex); return m_done; };
void ThreadWorker::callSingleAction(){
	{ boost::mutex::scoped_lock lock(m_mutex); m_done = false; }
	this->singleAction();
	{ boost::mutex::scoped_lock lock(m_mutex); m_done = true; }
};


CREATE_LOGGER(ThreadRunner);
CREATE_LOGGER(SimulationFlow);

void ThreadRunner::run(){
	// this is the body of execution of separate thread
	boost::mutex::scoped_lock lock(m_runmutex);
	try{
		workerThrew=false;
		while(looping()) {
			call();
			if(m_thread_worker->shouldTerminate()){ stop(); return; }
		}
	} catch (std::exception& e){
		LOG_FATAL("Exception occured: "<<endl<<e.what());
		workerException=std::exception(e); workerThrew=true;
		stop(); return;
	}
}

void ThreadRunner::call(){
	// this is the body of execution of separate thread
	boost::mutex::scoped_lock lock(m_callmutex);
	m_thread_worker->setTerminate(false);
	m_thread_worker->callSingleAction();
}

void ThreadRunner::pleaseTerminate(){ stop(); m_thread_worker->setTerminate(true); }
void ThreadRunner::spawnSingleAction(){
	boost::mutex::scoped_lock boollock(m_boolmutex);
	boost::mutex::scoped_lock calllock(m_callmutex);
	if(m_looping) return;
	boost::function0<void> call( boost::bind( &ThreadRunner::call , this ) );
	boost::thread th(call);
}

void ThreadRunner::start(){
	boost::mutex::scoped_lock lock(m_boolmutex);
	if(m_looping) return;
	m_looping=true;
	boost::function0<void> run( boost::bind( &ThreadRunner::run , this ) );
	boost::thread th(run);
}

void ThreadRunner::stop(){
	//std::cerr<<__FILE__<<":"<<__LINE__<<":"<<__FUNCTION__<<std::endl;
	if(!m_looping) return;
	//std::cerr<<__FILE__<<":"<<__LINE__<<":"<<__FUNCTION__<<std::endl;
	boost::mutex::scoped_lock lock(m_boolmutex);
	//std::cerr<<__FILE__<<":"<<__LINE__<<":"<<__FUNCTION__<<std::endl;
	m_looping=false;
}

bool ThreadRunner::looping(){
	boost::mutex::scoped_lock lock(m_boolmutex);
	return m_looping;
}

ThreadRunner::~ThreadRunner(){
	pleaseTerminate();
	boost::mutex::scoped_lock runlock(m_runmutex);
	boost::mutex::scoped_lock calllock(m_callmutex);
}




void SimulationFlow::singleAction()
{
	Scene* scene=Omega::instance().getScene().get();
	if (!scene) throw logic_error("SimulationFlow::singleAction: no Scene object?!");
	if(scene->subStepping) { LOG_INFO("Sub-stepping disabled when running simulation continuously."); scene->subStepping=false; }
	scene->moveToNextTimeStep();
	if(scene->stopAtStep>0 && scene->step==scene->stopAtStep) setTerminate(true);
};

