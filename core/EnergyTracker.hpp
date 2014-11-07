#pragma once
#include<woo/lib/base/Types.hpp>
#include<woo/lib/base/openmp-accu.hpp>
#include<woo/lib/object/Object.hpp>
#include<woo/lib/pyutil/except.hpp>


namespace py=boost::python;

class EnergyTracker: public Object{
	WOO_DECL_LOGGER; 
	public:
	/* in flg, IsIncrement is a pseudo-value which doesn't do anything; is meant to increase readability of calls */
	enum{ IsIncrement=0, IsResettable=1, ZeroDontCreate=2 };
	void findId(const std::string& name, int& id, int flg, bool newIfNotFound=true){
		if(id>0) return; // the caller should have checked this already
		if(names.count(name)) id=names[name];
		else if(newIfNotFound) {
			#ifdef WOO_OPENMP
				#pragma omp critical
			#endif
				{ energies.resize(energies.size()+1); id=energies.size()-1; flags.resize(id+1); flags[id]=flg; names[name]=id; assert(id<(int)energies.size()); assert(id>=0); }
		}
	}
	// set value of the accumulator; note: must NOT be called from parallel sections!
	void set(const Real& val, const std::string& name, int &id){
		if(id<0) findId(name,id,/* do not reset value that is set directly, although it is not "incremental" */ IsIncrement);
		energies.set(id,val);
	}
	// add value to the accumulator; safely called from parallel sections
	void add(const Real& val, const std::string& name, int &id, int flg);
	// add value from python (without the possibility of caching index, do name lookup every time)
	void add_py(const Real& val, const std::string& name, bool reset=false);

	py::dict names_py() const;
	Real getItem_py(const std::string& name);
	void setItem_py(const std::string& name, Real val);
	bool contains_py(const std::string& name);
	int len_py() const;
	void clear();
	void resetResettables(){ size_t sz=energies.size(); for(size_t id=0; id<sz; id++){ if(flags[id] & IsResettable) energies.reset(id); } }

	Real total() const;
	Real relErr() const;
	py::list keys_py() const;
	py::list items_py() const;
	py::dict perThreadData() const;

	typedef std::map<std::string,int> mapStringInt;
	typedef std::pair<std::string,int> pairStringInt;

	class pyIterator{
		const shared_ptr<EnergyTracker> et; mapStringInt::iterator I;
	public:
		pyIterator(const shared_ptr<EnergyTracker>&);
		pyIterator iter();
		string next();
	};
	pyIterator pyIter();

	#define woo_core_EnergyTracker__CLASS_BASE_DOC_ATTRS_PY \
		EnergyTracker,Object,"Storage for tracing energies. Only to be used if O.traceEnergy is True.", \
		((OpenMPArrayAccumulator<Real>,energies,,,"Energy values, in linear array")) \
		((mapStringInt,names,,/*no python converter for this type*/AttrTrait<Attr::hidden>(),"Associate textual name to an index in the energies array [overridden bellow]."))  \
		((vector<int>,flags,,AttrTrait<Attr::readonly>(),"Flags for respective energies; most importantly, whether the value should be reset at every step.")) \
		, /*py*/ \
			.def("__len__",&EnergyTracker::len_py,"Number of items in the container.") \
			.def("__getitem__",&EnergyTracker::getItem_py,"Get energy value for given name.") \
			.def("__setitem__",&EnergyTracker::setItem_py,"Set energy value for given name (will create a non-resettable item, if it does not exist yet).") \
			.def("__contains__",&EnergyTracker::contains_py,"Query whether given key exists; used by ``'key' in EnergyTracker``") \
			.def("__iter__",&EnergyTracker::pyIter,"Return iterator over keys, to support python iteration protocol.") \
			.def("add",&EnergyTracker::add_py,(py::arg("dE"),py::arg("name"),py::arg("reset")=false),"Accumulate energy, used from python (likely inefficient)") \
			.def("clear",&EnergyTracker::clear,"Clear all stored values.") \
			.def("keys",&EnergyTracker::keys_py,"Return defined energies.") \
			.def("items",&EnergyTracker::items_py,"Return contents as list of (name,value) tuples.") \
			.def("total",&EnergyTracker::total,"Return sum of all energies.") \
			.def("relErr",&EnergyTracker::relErr,"Total energy divided by sum of absolute values.") \
			.add_property("names",&EnergyTracker::names_py) /* return name->id map as python dict */ \
			.add_property("_perThreadData",&EnergyTracker::perThreadData,"Contents as dictionary, where each value is tuple of individual threads' values (for debugging)"); \
			/* define nested class */ \
			/*py::scope foo(_classObj);*/ \
			py::class_<EnergyTracker::pyIterator>("EnergyTracker_iterator",py::init<pyIterator>()).def("__iter__",&pyIterator::iter).def(WOO_next_OR__next__,&pyIterator::next); 
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_core_EnergyTracker__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(EnergyTracker);
