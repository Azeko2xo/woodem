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

py::object WooTestClass::arr3d_py_get(){
	py::list l0;
	for(size_t i=0; i<arr3d.shape()[0]; i++){
		py::list l1;
		for(size_t j=0; j<arr3d.shape()[1]; j++){
			py::list l2;
			for(size_t k=0; k<arr3d.shape()[2]; k++){
				l2.append(arr3d[i][j][k]);
			}
			l1.append(l2);
		}
		l0.append(l1);
	}
	return l0;
}

void WooTestClass::arr3d_set(const Vector3i& shape, const VectorXr& data){
	if(data.rows()!=shape.prod()) throw std::runtime_error("Data must have exactly "+to_string(shape[0])+"x"+to_string(shape[1])+"x"+to_string(shape[2])+"="+to_string(shape.prod())+" items (not "+to_string(data.rows())+").");
	arr3d.resize(boost::extents[shape[0]][shape[1]][shape[2]]);
	for(size_t i=0; i<shape[0]; i++) for(size_t j=0; j<shape[1]; j++) for(size_t k=0; k<shape[2]; k++) arr3d[i][j][k]=data[i*shape[0]*shape[1]+j*shape[1]+k];
}

