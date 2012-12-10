#include<woo/core/Test.hpp>

py::list WooTestClass::aaccuGetRaw(){
	vector<vector<Real>> ddta=aaccu.getPerThreadData();
	py::list ret;
	for(auto dta: ddta) ret.append(py::list(dta));
	return ret;
}

void WooTestClass::aaccuSetRaw(const vector<Real>& r){
	aaccu.resize(r.size());
	for(size_t i=0; i<r.size(); i++) aaccu.set(i,r[i]);
}

void WooTestClass::aaccuWriteThreads(size_t ix, const vector<Real>& cycleData){
	if(ix>=aaccu.size()) aaccu.resize(ix+1);
	if(cycleData.size()==0) throw std::runtime_error("cycleData==[] (at least one value must be given).");
	aaccu.set(ix,0); // zero the whole line
	size_t i=0;
	#ifdef WOO_OPENMP
		#pragma omp parallel for
		for(i=0; i<(size_t)omp_get_num_threads(); i++)
	#endif
		{ aaccu.add(ix,cycleData[i%cycleData.size()]); }
}
