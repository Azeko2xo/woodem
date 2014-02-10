#include<woo/core/EnergyTracker.hpp>

WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_core_EnergyTracker__CLASS_BASE_DOC_ATTRS_PY);

WOO_PLUGIN(core,(EnergyTracker));

Real EnergyTracker::total() const {
	Real ret=0; size_t sz=energies.size(); for(size_t id=0; id<sz; id++) ret+=energies.get(id); return ret;
}

Real EnergyTracker::relErr() const {
	Real sumAbs=0, sum=0; size_t sz=energies.size(); for(size_t id=0; id<sz; id++){  Real e=energies.get(id); sumAbs+=abs(e); sum+=e; } return (sumAbs>0?sum/sumAbs:0.);
}

void EnergyTracker::clear() { energies.clear(); names.clear(); flags.clear();}

py::list EnergyTracker::keys_py() const {
	py::list ret; for(const auto& p: names) ret.append(p.first); return ret;
}
py::list EnergyTracker::items_py() const {
	py::list ret; for(const auto& p: names) ret.append(py::make_tuple(p.first,energies.get(p.second))); return ret;
}

int EnergyTracker::len_py() const { return names.size(); }

void EnergyTracker::add_py(const Real& val, const std::string& name, bool reset){
	int id=-1; add(val,name,id,(reset?IsResettable:IsIncrement));
}

Real EnergyTracker::getItem_py(const std::string& name){
	int id=-1; findId(name,id,/*flags*/0,/*newIfNotFound*/false); 
	if (id<0) KeyError("Unknown energy name '"+name+"'.");
	return energies.get(id);
}

void EnergyTracker::setItem_py(const std::string& name, Real val){
	int id=-1; set(val,name,id);
}

bool EnergyTracker::contains_py(const std::string& name){
	int id=-1; findId(name,id,/*flags*/0,/*newIfNotFound*/false); return id>=0;
}


py::dict EnergyTracker::perThreadData() const {
	py::dict ret;
	std::vector<std::vector<Real> > dta=energies.getPerThreadData();
	FOREACH(pairStringInt p,names) ret[p.first]=dta[p.second];
	return ret;
}
py::dict EnergyTracker::names_py() const{
	py::dict ret;
	FOREACH(const pairStringInt& si, names){
		ret[si.first]=si.second;
	}
	return ret;
}


EnergyTracker::pyIterator EnergyTracker::pyIter(){ return EnergyTracker::pyIterator(static_pointer_cast<EnergyTracker>(shared_from_this())); }
EnergyTracker::pyIterator EnergyTracker::pyIterator::iter(){ return *this; }
/* python access */
EnergyTracker::pyIterator::pyIterator(const shared_ptr<EnergyTracker>& _et): et(_et), I(et->names.begin()){}
string EnergyTracker::pyIterator::next(){
	if(I==et->names.end()) woo::StopIteration();
	return (I++)->first;
}
