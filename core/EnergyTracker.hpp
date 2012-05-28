#pragma once
#include<boost/python.hpp>
#include<boost/foreach.hpp>
#include<string>
#include<yade/lib/base/openmp-accu.hpp>
#include<yade/lib/serialization/Serializable.hpp>
#include<yade/lib/pyutil/except.hpp>

#ifndef FOREACH
	#define FOREACH BOOST_FOREACH
#endif

namespace py=boost::python;

class EnergyTracker: public Serializable{
	public:
	~EnergyTracker();
	/* in flg, IsIncrement is a pseudo-value which doesn't do anything; is meant to increase readability of calls */
	enum{ IsIncrement=0, IsResettable=1, ZeroDontCreate=2 };
	void findId(const std::string& name, int& id, int flg, bool newIfNotFound=true){
		if(id>0) return; // the caller should have checked this already
		if(names.count(name)) id=names[name];
		else if(newIfNotFound) {
			#ifdef YADE_OPENMP
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
	void add(const Real& val, const std::string& name, int &id, int flg){
		// if ZeroDontCreate is set, the value is zero and no such energy exists, it will not be created and id will be still negative
		if(id<0) findId(name,id,flg,/*newIfNotFound*/!(val==0. && (flg&ZeroDontCreate)));
		if(id>=0) energies.add(id,val);
	}
	// add value from python (without the possibility of caching index, do name lookup every time)
	void add_py(const Real& val, const std::string& name, bool reset=false){ int id=-1; add(val,name,id,(reset?IsResettable:IsIncrement)); }
	py::dict names_py() const;

	Real getItem_py(const std::string& name){
		int id=-1; findId(name,id,/*flags*/0,/*newIfNotFound*/false); 
		if (id<0) KeyError("Unknown energy name '"+name+"'.");
		return energies.get(id);
	}
	void setItem_py(const std::string& name, Real val){
		int id=-1; set(val,name,id);
	}
	void clear(){ energies.clear(); names.clear(); flags.clear();}
	void resetResettables(){ size_t sz=energies.size(); for(size_t id=0; id<sz; id++){ if(flags[id] & IsResettable) energies.reset(id); } }

	Real total() const;
	Real relErr() const;
	py::list keys_py() const;
	py::list items_py() const;
	py::dict perThreadData() const;

	typedef std::map<std::string,int> mapStringInt;
	typedef std::pair<std::string,int> pairStringInt;

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(EnergyTracker,Serializable,"Storage for tracing energies. Only to be used if O.traceEnergy is True.",
		((OpenMPArrayAccumulator<Real>,energies,,,"Energy values, in linear array"))
		((mapStringInt,names,,/*no python converter for this type*/AttrTrait<Attr::hidden>(),"Associate textual name to an index in the energies array [overridden bellow].")) 
		((vector<int>,flags,,AttrTrait<Attr::readonly>(),"Flags for respective energies; most importantly, whether the value should be reset at every step."))
		,/*ctor*/
		,/*py*/
			.def("__getitem__",&EnergyTracker::getItem_py,"Get energy value for given name.")
			.def("__setitem__",&EnergyTracker::setItem_py,"Set energy value for given name (will create a non-resettable item, if it does not exist yet).")
			.def("add",&EnergyTracker::add_py,(py::arg("dE"),py::arg("name"),py::arg("reset")=false),"Accumulate energy, used from python (likely inefficient)")
			.def("clear",&EnergyTracker::clear,"Clear all stored values.")
			.def("keys",&EnergyTracker::keys_py,"Return defined energies.")
			.def("items",&EnergyTracker::items_py,"Return contents as list of (name,value) tuples.")
			.def("total",&EnergyTracker::total,"Return sum of all energies.")
			.def("relErr",&EnergyTracker::relErr,"Total energy divided by sum of absolute values.")
			.add_property("names",&EnergyTracker::names_py) // return name->id map as python dict
			.add_property("_perThreadData",&EnergyTracker::perThreadData,"Contents as dictionary, where each value is tuple of individual threads' values (for debugging)")
	)
};
REGISTER_SERIALIZABLE(EnergyTracker);
