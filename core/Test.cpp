#include<woo/core/Test.hpp>

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_core_WooTestClass__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_core_WooTestPeriodicEngine__CLASS_BASE_DOC_ATTRS);

WOO_PLUGIN(core,(WooTestClass)(WooTestPeriodicEngine));


void WooTestClass::postLoad(WooTestClass&, void* addr){
	if(addr==NULL){ postLoadStage=POSTLOAD_CTOR; return; }
	if(addr==(void*)&foo_incBaz){ baz++; postLoadStage=POSTLOAD_FOO; return; }
	if(addr==(void*)&bar_zeroBaz){ baz=0; postLoadStage=POSTLOAD_BAR; return; }
	if(addr==(void*)&baz){ postLoadStage=POSTLOAD_BAZ; return; }
}


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
