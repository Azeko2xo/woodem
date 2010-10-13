#pragma once
#include<yade/core/GlobalEngine.hpp>
#include<boost/python.hpp>

class ParallelEngine;
shared_ptr<ParallelEngine> ParallelEngine_ctor_list(const python::list& slaves);

class ParallelEngine: public Engine {
	public:
		typedef vector<vector<shared_ptr<Engine> > > slaveContainer;
		virtual void action();
		virtual bool isActivated(){return true;}
	// py access
		boost::python::list slaves_get();
		void slaves_set(const boost::python::list& slaves);
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(ParallelEngine,Engine,"Engine for running other Engine in parallel.",
		((slaveContainer,slaves,,,"[will be overridden]")),
		/*ctor*/,
		/*py*/
		.def("__init__",python::make_constructor(ParallelEngine_ctor_list),"Construct from (possibly nested) list of slaves.")
		.add_property("slaves",&ParallelEngine::slaves_get,&ParallelEngine::slaves_set,"List of lists of Engines; each top-level group will be run in parallel with other groups, while Engines inside each group will be run sequentially, in given order.");
	);
};
REGISTER_SERIALIZABLE(ParallelEngine);


