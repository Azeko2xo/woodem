#include<yade/core/EnergyTracker.hpp>

Real EnergyTracker::total() const { Real ret=0; size_t sz=energies.size(); for(size_t id=0; id<sz; id++) ret+=energies.get(id); return ret; }
Real EnergyTracker::relErr() const {
	Real sumAbs=0, sum=0; size_t sz=energies.size(); for(size_t id=0; id<sz; id++){  Real e=energies.get(id); sumAbs+=abs(e); sum+=e; } return (sumAbs>0?sum/sumAbs:0.);
}
py::list EnergyTracker::keys_py() const { py::list ret; FOREACH(pairStringInt p, names) ret.append(p.first); return ret; }
py::list EnergyTracker::items_py() const { py::list ret; FOREACH(pairStringInt p, names) ret.append(py::make_tuple(p.first,energies.get(p.second))); return ret; }
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
