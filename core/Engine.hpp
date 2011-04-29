#pragma once

#include<yade/lib/serialization/Serializable.hpp>
#include<yade/core/Omega.hpp>
#include<yade/core/Timing.hpp>
#include<yade/lib/base/Logging.hpp>
#include<stdexcept>


#include<boost/foreach.hpp>
#ifndef FOREACH
#define FOREACH BOOST_FOREACH
#endif

class Scene;

class Engine: public Serializable{
	public:
		// pointer to the simulation, set at every step by Scene::moveToNextTimeStep
		Scene* scene;
		//! high-level profiling information; not serializable
		TimingInfo timingInfo; 
		//! precise profiling information (timing of fragments of the engine)
		shared_ptr<TimingDeltas> timingDeltas;

		virtual bool isActivated() { return true; };
		virtual void action();
	private:
		// py access funcs	
		TimingInfo::delta timingInfo_nsec_get(){return timingInfo.nsec;};
		void timingInfo_nsec_set(TimingInfo::delta d){ timingInfo.nsec=d;}
		long timingInfo_nExec_get(){return timingInfo.nExec;};
		void timingInfo_nExec_set(long d){ timingInfo.nExec=d;}
		void explicitAction(); 

	DECLARE_LOGGER;

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Engine,Serializable,"Basic execution unit of simulation, called from the simulation loop (O.engines)",
		((bool,dead,false,,"If true, this engine will not run at all; can be used for making an engine temporarily deactivated and only resurrect it at a later point."))
		((string,label,,,"Textual label for this object; must be valid python identifier, you can refer to it directly from python.")),
		/* ctor */ scene=Omega::instance().getScene().get() ,
		/* py */
		.add_property("execTime",&Engine::timingInfo_nsec_get,&Engine::timingInfo_nsec_set,"Cummulative time this Engine took to run (only used if :yref:`O.timingEnabled<Omega.timingEnabled>`\\ ==\\ ``True``).")
		.add_property("execCount",&Engine::timingInfo_nExec_get,&Engine::timingInfo_nExec_set,"Cummulative count this engine was run (only used if :yref:`O.timingEnabled<Omega.timingEnabled>`\\ ==\\ ``True``).")
		.def_readonly("timingDeltas",&Engine::timingDeltas,"Detailed information about timing inside the Engine itself. Empty unless enabled in the source code and :yref:`O.timingEnabled<Omega.timingEnabled>`\\ ==\\ ``True``.")
		.def("__call__",&Engine::explicitAction)
	);
};
REGISTER_SERIALIZABLE(Engine);

class PartialEngine: public Engine{
	YADE_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(PartialEngine,Engine,"Engine affecting only particular bodies in the simulation, defined by :yref:`ids<PartialEngine.ids>`.",
		((std::vector<int>,ids,,,":yref:`Ids<Body::id>` of bodies affected by this PartialEngine.")),
		/*deprec*/, /*init*/, /* ctor */, /* py */
	);
};
REGISTER_SERIALIZABLE(PartialEngine);

class GlobalEngine: public Engine{
	public :
	YADE_CLASS_BASE_DOC(GlobalEngine,Engine,"Engine that will generally affect the whole simulation (contrary to :yref:`PartialEngine`).");
};
REGISTER_SERIALIZABLE(GlobalEngine);


class PeriodicEngine: public GlobalEngine {
	public:
		static Real getClock(){ timeval tp; gettimeofday(&tp,NULL); return tp.tv_sec+tp.tv_usec/1e6; }
		virtual bool isActivated();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(PeriodicEngine,GlobalEngine,
		"Run Engine::action with given fixed periodicity real time (=wall clock time, computation time), \
		virtual time (simulation time), step number), by setting any of those criteria \
		(virtPeriod, realPeriod, stepPeriod) to a positive value. They are all negative (inactive)\
		by default.\n\n\
		\
		The number of times this engine is activated can be limited by setting nDo>0. If the number of activations \
		will have been already reached, no action will be called even if an active period has elapsed. \n\n\
		\
		If initRun is set (true by default), the engine will run when called for the first time; otherwise it will only \
		start counting period (realLast etc interal variables) from that point, but without actually running, and will run \
		only once a period has elapsed since the initial run. \n\n\
		\
		This class should not be used directly; rather, derive your own engine which you want to be run periodically. \n\n\
		\
		Derived engines should override Engine::action(), which will be called periodically. If the derived Engine \
		overrides also Engine::isActivated, it should also take in account return value from PeriodicEngine::isActivated, \
		since otherwise the periodicity will not be functional. \n\n\
		\
		Example with PyRunner, which derives from PeriodicEngine; likely to be encountered in python scripts):: \n\n\
		\
			PyRunner(realPeriod=5,stepPeriod=10000,command='print O.step')	\n\n\
		\
		will print step number every 10000 step or every 5 seconds of wall clock time, whiever comes first since it was \
		last run.",
		((Real,virtPeriod,((void)"deactivated",0),,"Periodicity criterion using virtual (simulation) time (deactivated if <= 0)"))
		((Real,realPeriod,((void)"deactivated",0),,"Periodicity criterion using real (wall clock, computation, human) time (deactivated if <=0)"))
		((long,stepPeriod,((void)"deactivated",1),,"Periodicity criterion using step number (deactivated if <= 0)"))
		((long,nDo,((void)"deactivated",-1),,"Limit number of executions by this number (deactivated if negative)"))
		((bool,initRun,true,,"Run the first time we are called as well."))
		((Real,virtLast,0,,"Tracks virtual time of last run |yupdate|."))
		((Real,realLast,0,,"Tracks real time of last run |yupdate|."))
		((long,stepLast,0,,"Tracks step number of last run |yupdate|."))
		((long,nDone,0,,"Track number of executions (cummulative) |yupdate|.")),
		/* this will be put inside the ctor */ realLast=getClock();
	);
};
REGISTER_SERIALIZABLE(PeriodicEngine);


struct PyRunner: public PeriodicEngine{
	virtual void action();
	// to give command without saying 'command=...'
	virtual void pyHandleCustomCtorArgs(python::tuple& t, python::dict& d);
	YADE_CLASS_BASE_DOC_ATTRS(PyRunner,PeriodicEngine,
		"Execute a python command periodically, with defined (and adjustable) periodicity. See :yref:`PeriodicEngine` documentation for details.\n\n.. admonition:: Special constructor\n\n*command* can be given as first unnamed string argument (``PyRunner('foo()')``), stepPeriod as unnamed integer argument (``PyRunner('foo()',100)`` or ``PyRunner(100,'foo()')``).",
		((string,command,"",,"Command to be run by python interpreter. Not run if empty."))
	);
};
REGISTER_SERIALIZABLE(PyRunner);


