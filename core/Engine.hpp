#pragma once

#include<woo/lib/object/Object.hpp>
#include<woo/core/Field.hpp>
#include<woo/core/Timing.hpp>
#include<woo/lib/base/Logging.hpp>

#include<stdexcept>
#include<boost/regex.hpp>

#include<woo/lib/pyutil/except.hpp>
#include<woo/lib/pyutil/gil.hpp>


#include<boost/foreach.hpp>
#ifndef FOREACH
#define FOREACH BOOST_FOREACH
#endif

struct Scene;
struct Field;

namespace py=boost::python;

class Engine: public Object {
	public:
		// pointer to the simulation and field, set by Scene::moveToNextTimeStep
		Scene* scene;
		//! high-level profiling information; not serializable
		TimingInfo timingInfo; 
		//! precise profiling information (timing of fragments of the engine)
		shared_ptr<TimingDeltas> timingDeltas;
		virtual bool isActivated() { return true; };
		//! notify engine that dead has been changed (does nothing by default)
		virtual void notifyDead(){};

		// cycles through fields and dispatches to run(), which then does the real work
		// by default runs the engine for accepted fields
		virtual void run();

		// virtual bool acceptsField(Field*){ cerr<<getClassName()<<"::acceptsField not overridden."<<endl; return true; }
		virtual bool needsField(){ return true; }
		virtual void setField();
		virtual bool acceptsField(Field* f){ return true; }
		shared_ptr<Field> field_get();
		void field_set(const shared_ptr<Field>&);

		#ifdef WOO_OPENGL
			virtual void render(const GLViewInfo&){};
		#endif

		void postLoad(Engine&,void*);
		void setDefaultScene();

		void runPy(const string&);

		virtual void getLabeledObjects(std::map<std::string,py::object>&){};
		template<class T> static void handlePossiblyLabeledObject(const shared_ptr<T>& o, std::map<std::string,py::object>& m){
			if(o->label.empty()) return;
			GilLock gilLock; // is this needed?
			if(m.count(o->label)>0) woo::NameError("Duplicate object label '"+o->label+"'.");
			boost::smatch match;
			if(boost::regex_match(o->label,match,boost::regex("([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\[([0-9]+)\\]"))){
				std::string lab0=match[1];	long index=lexical_cast<long>(match[2]);
				//cerr<<"Label "<<o->label<<" matches list regex: "<<lab0<<" @ "<<index<<endl;
				if(m.find(lab0)!=m.end()){
					if(!py::extract<py::list>(m[lab0]).check()) woo::NameError("Label "+o->label+" is subscripted, but and already existing label "+lab0+" is not a list!");
					//cerr<<"Using existing list with length "<<py::len(py::extract<py::list>(m[lab0]))<<endl;
				} else {
					//cerr<<"Creating empty list"<<endl;
					m[lab0]=py::list();
				}
				py::list lst=py::extract<py::list>(m[lab0]);
				//cerr<<"Adding empty objects to reach index "<<index;
				while(py::len(lst)<index+1) lst.append(py::object());
				if(!py::object(lst[index]).is_none()) woo::NameError("Labeled object "+lab0+"["+to_string(index)+"] already assigned!");
				lst[index]=py::object(o);
			} else if(boost::regex_match(o->label,match,boost::regex("[a-zA-Z_][a-zA-Z0-9_]*"))){
				m[o->label]=py::object(o);
			} else woo::NameError("Object label '"+o->label+"' is not a valid python identifier name.");
		}
	private:
		// py access funcs	
		TimingInfo::delta timingInfo_nsec_get(){return timingInfo.nsec;};
		void timingInfo_nsec_set(TimingInfo::delta d){ timingInfo.nsec=d;}
		long timingInfo_nExec_get(){return timingInfo.nExec;};
		void timingInfo_nExec_set(long d){ timingInfo.nExec=d;}
		void explicitRun(); 

	DECLARE_LOGGER;

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Engine,Object,ClassTrait().doc("Basic execution unit of simulation, called from the simulation loop (O.engines)"),
		((bool,dead,false,AttrTrait<Attr::triggerPostLoad>(),"If true, this engine will not run at all; can be used for making an engine temporarily deactivated and only resurrect it at a later point."))
		((string,label,,AttrTrait<>().activeLabel(),"Textual label for this object; must be valid python identifier, you can refer to it directly from python."))
		// ((bool,parallelFields,false,,"Whether to run (if compiled with openMP) this engine on active fields in parallel"))
		((shared_ptr<Field>,field,,AttrTrait<>().noGui().noDump(),"User-requested `woo.core.Field` to run this engine on; if empty, fields will be searched for admissible ones; if more than one is found, exception will be raised."))
		((bool,userAssignedField,false,AttrTrait<Attr::readonly>().noGui(),"Whether the `woo.core.Engine.field` was user-assigned or automatically assigned, to know whether to update automatically."))
		((bool,isNewObject,true,AttrTrait<Attr::hidden>(),"Flag to recognize in postLoad whether this object has just been constructed, to set userAssignedField properly (ugly...)"))
		,/* ctor */ setDefaultScene(); ,
		/* py */
		.add_property("execTime",&Engine::timingInfo_nsec_get,&Engine::timingInfo_nsec_set,"Cummulative time this Engine took to run (only used if :ref:`O.timingEnabled<Master.timingEnabled>`\\ ==\\ ``True``).")
		.add_property("execCount",&Engine::timingInfo_nExec_get,&Engine::timingInfo_nExec_set,"Cummulative count this engine was run (only used if :ref:`O.timingEnabled<Master.timingEnabled>`\\ ==\\ ``True``).")
		.def_readonly("timingDeltas",&Engine::timingDeltas,"Detailed information about timing inside the Engine itself. Empty unless enabled in the source code and :ref:`O.timingEnabled<Master.timingEnabled>`\\ ==\\ ``True``.")
		.def("__call__",&Engine::explicitRun)
		.def("acceptsField",&Engine::acceptsField)
		.add_property("field",&Engine::field_get,&Engine::field_set,"Field to run this engine on; if unassigned, or set to *None*, automatic field selection is triggered.")
	);
};
REGISTER_SERIALIZABLE(Engine);

class PartialEngine: public Engine{
	WOO_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(PartialEngine,Engine,"Engine affecting only particular bodies in the simulation, defined by `ids<woo.core.PartialEngine.ids>`.",
		((std::vector<int>,ids,,,"`ids<woo.dem.Particle.id>` of particles affected by this PartialEngine.")),
		/*deprec*/, /*init*/, /* ctor */, /* py */
	);
};
REGISTER_SERIALIZABLE(PartialEngine);

class GlobalEngine: public Engine{
	public :
	WOO_CLASS_BASE_DOC(GlobalEngine,Engine,"Engine that will generally affect the whole simulation (contrary to :ref:`PartialEngine`).");
};
REGISTER_SERIALIZABLE(GlobalEngine);

class ParallelEngine: public Engine {
	public:
		typedef vector<vector<shared_ptr<Engine> > > slaveContainer;
		virtual void run();
		virtual bool isActivated(){return true;}
		virtual bool needsField(){ return false; }
		virtual void setField();
		virtual void getLabeledObjects(std::map<std::string,py::object>&);
		void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);

	// py access
		py::list pySlavesGet();
		void pySlavesSet(const py::list& slaves);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ParallelEngine,Engine,"Engine for running other Engine in parallel.\n\n.. admonition:: Special constructor\n\nPossibly nested list of engines, where each top-level item (engine or list) will be run in parallel; nested lists will be run sequentially.",
		((slaveContainer,slaves,,AttrTrait<Attr::hidden>(),"[will be overridden]")),
		/*ctor*/,
		/*py*/
		.add_property("slaves",&ParallelEngine::pySlavesGet,&ParallelEngine::pySlavesSet,"List of lists of Engines; each top-level group will be run in parallel with other groups, while Engines inside each group will be run sequentially, in given order.");
	);
};
REGISTER_SERIALIZABLE(ParallelEngine);


class PeriodicEngine: public GlobalEngine{
	public:
		static Real getClock(){ timeval tp; gettimeofday(&tp,NULL); return tp.tv_sec+tp.tv_usec/1e6; }
		virtual bool isActivated();
		// set virtLast, realLast, stepLast as if we run now; don't modify nDo/nDone, though
		void fakeRun();
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(PeriodicEngine,GlobalEngine,
		"Run Engine::run with given fixed periodicity real time (=wall clock time, computation time), virtual time (simulation time), step number), by setting any of those criteria (virtPeriod, realPeriod, stepPeriod) to a positive value. They are all negative (inactive) by default.\n\nThe number of times this engine is activated can be limited by setting nDo>0. If the number of activations will have been already reached, no action will be called even if an active period has elapsed.\n\nIf initRun is set (true by default), the engine will run when called for the first time; otherwise it will only  start counting period (realLast etc interal variables) from that point, but without actually running, and will run only once a period has elapsed since the initial run.\n\nThis class should not be used directly; rather, derive your own engine which you want to be run periodically.\n\nDerived engines should override Engine::action(), which will be called periodically. If the derived Engine overrides also Engine::isActivated, it should also take in account return value from PeriodicEngine::isActivated, otherwise the periodicity will not be functional.\n\nExample with PyRnner, which derives from PeriodicEngine; likely to be encountered in python scripts)::\n\n\
		\
			PyRunner(realPeriod=5,stepPeriod=10000,command='print O.step')	\n\n\
		\
		will print step number every 10000 step or every 5 seconds of wall clock time, whiever comes first since it was last run.",
		((Real,virtPeriod,((void)"deactivated",0),,"Periodicity criterion using virtual (simulation) time (deactivated if <= 0)"))
		((Real,realPeriod,((void)"deactivated",0),,"Periodicity criterion using real (wall clock, computation, human) time (deactivated if <=0)"))
		((long,stepPeriod,((void)"deactivated",1),,"Periodicity criterion using step number (deactivated if <= 0)"))
		((long,nDo,((void)"deactivated",-1),,"Limit number of executions by this number (deactivated if negative)"))
		((long,nDone,0,,"Track number of executions (cumulative)."))
		((bool,initRun,true,,"Run the first time we are called as well."))
		((Real,virtLast,NaN,,"Tracks virtual time of last run."))
		((Real,realLast,NaN,,"Tracks real time of last run."))
		((long,stepLast,-1,,"Tracks step number of last run.")
		)
		((long,stepPrev,-1,AttrTrait<>().noGui(),"Number of step when we run previously (stepLast is the current step when the engine runs)"))
		((Real,virtPrev,-1,AttrTrait<>().noGui(),"Simulation time when run previously"))
		((Real,realPrev,-1,AttrTrait<>().noGui(),"Real time when run previously"))
		,/*ctor*/ realLast=getClock();
	);
};
REGISTER_SERIALIZABLE(PeriodicEngine);


struct PyRunner: public PeriodicEngine{
	virtual void run(){ Engine::runPy(command); }
	// to give command without saying 'command=...'
	virtual bool needsField(){ return false; }
	virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);
	WOO_CLASS_BASE_DOC_ATTRS(PyRunner,PeriodicEngine,
		"Execute a python command periodically, with defined (and adjustable) periodicity. See :ref:`PeriodicEngine` documentation for details.\n\n.. admonition:: Special constructor\n\n*command* can be given as first unnamed string argument (``PyRunner('foo()')``), stepPeriod as unnamed integer argument (``PyRunner('foo()',100)`` or ``PyRunner(100,'foo()')``).",
		((string,command,"",,"Command to be run by python interpreter. Not run if empty."))
	);
};
REGISTER_SERIALIZABLE(PyRunner);

