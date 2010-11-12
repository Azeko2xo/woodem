#pragma once
#include<boost/python.hpp>
#include<boost/foreach.hpp>
#include<string>
#include<yade/lib/base/openmp-accu.hpp>
#include<yade/lib/serialization/Serializable.hpp>

#ifndef FOREACH
	#define FOREACH BOOST_FOREACH
#endif

namespace py=boost::python;

class EnergyTracker: public Serializable{
	public:
	~EnergyTracker();
	void findId(const std::string& name, int& id, bool reset=false, bool newIfNotFound=true){
		if(id>0) return; // the caller should have checked this already
		if(names.count(name)) id=names[name];
		else if(newIfNotFound) {
			#ifdef YADE_OPENMP
				#pragma omp critical
			#endif
				{ energies.resize(energies.size()+1); id=energies.size()-1; resetStep.resize(id+1); resetStep[id]=reset; names[name]=id; assert(id<(int)energies.size()); assert(id>=0); }
		}
	}
	// set value of the accumulator; note: must NOT be called from parallel sections!
	void set(const Real& val, const std::string& name, int &id){
		if(id<0) findId(name,id,/* do not reset value that is set directly */ false);
		energies.set(id,val);
	}
	// add value to the accumulator; safely called from parallel sections
	void add(const Real& val, const std::string& name, int &id, bool reset=false){
		if(id<0) findId(name,id,reset);
		energies.add(id,val);
	}
	Real getItem_py(const std::string& name){
		int id=-1; findId(name,id,false,false); 
		if (id<0) {PyErr_SetString(PyExc_KeyError,("Unknown energy name '"+name+"'.").c_str());  python::throw_error_already_set(); }
		return energies.get(id);
	}
	void setItem_py(const std::string& name, Real val){
		int id=-1; set(val,name,id);
	}
	void clear(){ energies.clear(); names.clear(); resetStep.clear();}
	Real total(){ Real ret=0; size_t sz=energies.size(); for(size_t id=0; id<sz; id++) ret+=energies.get(id); return ret; }

	py::list keys_py(){ py::list ret; FOREACH(pairStringInt p, names) ret.append(p.first); return ret; }
	void resetResettables(){ size_t sz=energies.size(); for(size_t id=0; id<sz; id++){ if(resetStep[id]) energies.reset(id); } }

	typedef std::map<std::string,int> mapStringInt;
	typedef std::pair<std::string,int> pairStringInt;

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(EnergyTracker,Serializable,"Storage for tracing energies. Only to be used if O.traceEnergy is True.",
		((OpenMPArrayAccumulator<Real>,energies,,,"Energy values, in linear array"))
		((mapStringInt,names,,Attr::hidden,"Associate textual name to an index in the energies array."))
		((vector<bool>,resetStep,,Attr::hidden,"Whether the respective energy value should be reset at every step."))
		,/*ctor*/
		,/*py*/
			.def("__getitem__",&EnergyTracker::getItem_py,"Get energy value for given name.")
			.def("__setitem__",&EnergyTracker::setItem_py,"Set energy value for given name (will create a non-resettable item, if it does not exist yet).")
			.def("clear",&EnergyTracker::clear,"Clear all stored values.")
			.def("keys",&EnergyTracker::keys_py,"Return defined energies.")
			.def("total",&EnergyTracker::total,"Return sum of all energies.")
	)
};
REGISTER_SERIALIZABLE(EnergyTracker);
