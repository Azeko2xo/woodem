// © 2013 Václav Šmilauer <eu@doxos.eu>

#include<woo/pkg/dem/GridStore.hpp>
#include<woo/pkg/dem/GridCollider.hpp> // for timing macro GC_CHECKPOINT2
#include<boost/function_output_iterator.hpp>
#include<boost/range/algorithm/set_algorithm.hpp>

WOO_PLUGIN(dem,(GridStore));

CREATE_LOGGER(GridStore);


GridStore::GridStore(const Vector3i& _gridSize, int _cellLen, bool _denseLock, int _exIniSize, int _exNumMaps): gridSize(_gridSize), cellLen(_cellLen), denseLock(_denseLock), exIniSize(_exIniSize), exNumMaps(_exNumMaps){
	postLoad(*this,NULL);
}

void GridStore::postLoad(GridStore&,void* I){
	if(I!=NULL) throw std::logic_error("GridStore::postLoad: called after a variable was set. Which one!?");
	if(grid) return; // everything is set up already, this is a spurious call
	if(gridSize.minCoeff()<=0) throw std::logic_error("GridStore.gridSize: all dimensions must be positive.");
	if(cellLen<=1) throw std::logic_error("GridStore.cellLen must be greater than one.");
	if(exNumMaps<=0) throw std::logic_error("GridStore.exNumMaps must be positive.");
	if(exIniSize<=0) throw std::logic_error("GridStore.exIniSize must be positive.");
	grid=std::unique_ptr<gridT>(new gridT(boost::extents[gridSize[0]][gridSize[1]][gridSize[2]][cellLen+1],boost::c_storage_order()));
	// check that those are default-initialized (zeros)
	assert((*grid)[0][0][0][0]==0);
	assert((*grid)[0][0][0][1]==0);
	gridExx.resize(exNumMaps);
	size_t nMutexes=exNumMaps;
	if(denseLock) nMutexes+=gridSize.prod();
	mutexes.reserve(nMutexes);
	// mutexes will be deleted automatically when the container is freed
	for(size_t i=0; i<nMutexes; i++) mutexes.push_back(new boost::mutex);
	LOG_TRACE(mutexes.size()<<" mutexes, "<<gridSize.prod()<<" cells, "<<exNumMaps<<" extension maps");
	assert(mutexes.size()==(size_t)(exNumMaps+(denseLock?gridSize.prod():0)));
}

AlignedBox3r GridStore::ijk2boxShrink(const Vector3i& ijk, const Real& shrink) const{
	AlignedBox3r box=ijk2box(ijk); Vector3r s=box.sizes();
	box.min()=box.min()+.5*shrink*s;
	box.max()=box.max()-.5*shrink*s;
	return box;
}

Vector3r GridStore::xyzNearXyz(const Vector3r& xyz, const Vector3i& ijk) const{
	AlignedBox3r box=ijk2box(ijk);
	Vector3r ret;
	for(int ax:{0,1,2}){
		ret[ax]=(xyz[ax]<box.min()[ax]?box.min()[ax]:(xyz[ax]>box.max()[ax]?box.max()[ax]:xyz[ax]));
	}
	return ret;
}

Vector3r GridStore::xyzNearIjk(const Vector3i& from, const Vector3i& ijk) const{
	AlignedBox3r box=ijk2box(ijk);
	Vector3r ret;
	for(int ax:{0,1,2}){
		ret[ax]=(from[ax]<=ijk[ax]?box.min()[ax]:box.max()[ax]);
	}
	return ret;
}



bool GridStore::isCompatible(shared_ptr<GridStore>& other){
	// if grid dimension matches, tht is all we need
	if(this->sizes()!=other->sizes()) return false;
	if(this->lo!=other->lo) return false;
	if(this->cellSize!=other->cellSize) return false;
	return true;
}

void GridStore::makeCompatible(shared_ptr<GridStore>& g, int l, bool _denseLock, int _exIniSize, int _exNumMaps) const {
	auto shape=grid->shape();
	l=l>0?l:shape[3]; // set to the real desired value of l
	assert(l>0);
	// create new grid if none or must be resized
	// (multi_array preserves elements when resizing, which is a copy operation we don't need at all;
	//  just create a new array from scratch in that case)
	if(!g || gridSize!=g->gridSize || (l>0 && g->cellLen!=l) || g->denseLock!=_denseLock || (_exIniSize>0 && _exIniSize!=exIniSize) || (_exNumMaps>0 && _exNumMaps!=exNumMaps)){
		// avoid message during --test
		if(gridSize.prod()>100) LOG_WARN("Allocating new grid.");
		g=make_shared<GridStore>(Vector3i(shape[0],shape[1],shape[2]),l>0?l:shape[3],/*denseLock*/_denseLock,/*exIniSize*/_exIniSize>0?_exIniSize:exIniSize,/*exNumMaps*/_exNumMaps>0?_exNumMaps:exNumMaps);
		g->lo=lo; g->cellSize=cellSize;
		return;
	}
	assert(g);
	assert(gridSize==g->gridSize && (l<0 || g->cellLen==l));
	assert(g->mutexes.size()==(size_t)g->exNumMaps+(g->denseLock?g->gridSize.prod():0));
	assert(_exNumMaps<0 || (int)g->gridExx.size()==_exNumMaps);
	g->lo=lo; g->cellSize=cellSize;
}

void GridStore::clear() {
	// TODO: get id_t as a typedef from inside gridT?
	std::memset((void*)grid->data(),0,grid->num_elements()*sizeof(id_t)); 
	// std::fill(grid->origin(),grid->origin()+grid->num_elements(),0);
	clear_ex();
};

void GridStore::clear_ex() {
	// don't forget the reference here!
	for(auto& gridEx: gridExx) gridEx.clear();
	#if 0
		cerr<<"Cleared grid, sizes are: "; for(const auto& c: pyCounts()) cerr<<c<<" "; cerr<<endl;
	#endif
}

const GridStore::gridExT& GridStore::getGridEx(const Vector3i& ijk) const {
	size_t ix=ijk2lin(ijk);
	assert(exNumMaps>0);
	assert(gridExx.size()==(size_t)exNumMaps);
	return gridExx[ix%exNumMaps];
}

GridStore::gridExT& GridStore::getGridEx(const Vector3i& ijk) {
	size_t ix=ijk2lin(ijk);
	assert(exNumMaps>0);
	assert(gridExx.size()==(size_t)exNumMaps);
	return gridExx[ix%exNumMaps];
}

void GridStore::protected_append(const Vector3i& ijk, const GridStore::id_t& id){
	checkIndices(ijk);
	assert(denseLock);
	boost::mutex::scoped_lock lock(*getMutex</*mutexEx*/false>(ijk));
	append(ijk,id);
}

void GridStore::append(const Vector3i& ijk, const GridStore::id_t& id, bool noSizeInc){
	checkIndices(ijk);
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	// 0th element is the number of elements
	// it is fetched and incremented atomically
	int& cellSz=(*grid)[i][j][k][0];
	int oldCellSz=CompUtils::fetch_add(&cellSz,noSizeInc?0:1);
	assert(oldCellSz>=0);
	const int &denseSz=grid->shape()[3]-1; // decrement as size itself takes space
	assert(!noSizeInc || oldCellSz>=denseSz);
	if(oldCellSz<denseSz){
		// store in the dense part, shift by one because of size at the beginning
		(*grid)[i][j][k][oldCellSz+1]=id;
	} else {
		assert(exIniSize>0);
		auto& gridEx=getGridEx(ijk);
		boost::mutex::scoped_lock lock(*getMutex</*mutexEx*/true>(ijk));
		if(oldCellSz==denseSz){ 
			// thread oldCellSz==denseSz, but extension vector was created in the meantime by another thread - try again
			if(gridEx.find(ijk)!=gridEx.end()){
				#if 1
					LOG_ERROR("gridEx.find(ijk)!=gridEx.end() when creating new extension vector; gridEx(ijk) contents:");
					auto ex=gridEx.find(ijk)->second;
					for(size_t i=0; i<ex.size(); i++) LOG_ERROR("ex["<<ijk<<"]["<<i<<"]="<<ex[i]);
				#endif
				#ifdef WOO_OPENMP
					assert(omp_in_parallel());
				#endif
				lock.unlock();
				append(ijk,id,/*noSizeInc*/true); // try again
				return;
			}
			assert(gridEx.find(ijk)==gridEx.end());
			vector<id_t>& ex=gridEx[ijk];
			ex.resize(exIniSize);
			ex[0]=id;
		} else {
			// existing extension vector
			auto exI=gridEx.find(ijk);
			// another thread incremented cellSz before us, but did not add extension vector yet
			if(exI==gridEx.end()){
				#ifdef WOO_OPENMP
					assert(omp_in_parallel());
				#endif
				lock.unlock();
				append(ijk,id,/*noSizeInc*/true); // try again
				return;
			}
			assert(exI!=gridEx.end());
			vector<id_t>& ex(exI->second);
			size_t exIx=oldCellSz-denseSz; // first unused index
			assert(exIx>0);
			if(exIx==ex.size()) ex.resize(ex.size()+exIniSize);
			assert(exIx<=ex.size());
			ex[exIx]=id;
		}
	}
}

void GridStore::clear_dense(const Vector3i& ijk){
	checkIndices(ijk);
	(*grid)[ijk[0]][ijk[1]][ijk[2]][0]=0;
}

void GridStore::pyAppend(const Vector3i& ijk, GridStore::id_t id) {
	if(denseLock) protected_append(ijk, id);
	else append(ijk,id);
}

void GridStore::pyDelItem(const Vector3i& ijk){
	if(denseLock){ boost::mutex::scoped_lock(*getMutex</*mutexEx*/false>(ijk)); clear_dense(ijk); }
	else clear_dense(ijk);
	boost::mutex::scoped_lock(*getMutex</*mutexEx*/true>(ijk));
	auto& gridEx=getGridEx(ijk);
	gridEx.erase(ijk);
}

py::list GridStore::pyGetItem(const Vector3i& ijk) const {
	size_t sz=this->size(ijk);
	py::list ret;
	for(size_t l=0; l<sz; l++) ret.append(this->get(ijk,l));
	return ret;
}

void GridStore::pySetItem(const Vector3i& ijk, const vector<GridStore::id_t>& ids) {
	pyDelItem(ijk);
	for(id_t id: ids) pyAppend(ijk,id);
}

py::dict GridStore::pyCountEx() const {
	py::dict ret;
	for(const auto& gridEx: gridExx){
		for(const auto& ijkVec: gridEx){
			const Vector3i& ijk(ijkVec.first);
			ret[py::make_tuple(ijk[0],ijk[1],ijk[2])]=(int)size(ijkVec.first)-(int)cellLen;
			// FIXME: this fails?!
			//assert(ijkVec.second.size()>=size(ijkVec.first)-cellLen+1);
		}
	}
	return ret;
}

vector<int> GridStore::pyCounts() const{
	size_t N=linSize();
	vector<int> ret;
	for(size_t n=0; n<N; n++){
		size_t cnt=size(lin2ijk(n));
		if(ret.size()<=cnt) ret.resize(cnt+1,0);
		ret[cnt]+=1;
	}
	return ret;
}

py::tuple GridStore::pyRawData(const Vector3i& ijk){
	py::list dense;
	for(int l=0; l<(int)grid->shape()[3]; l++) dense.append((*grid)[ijk[0]][ijk[1]][ijk[2]][l]);
	auto& gridEx=getGridEx(ijk);
	auto ijkVec=gridEx.find(ijk);
	py::list extra;
	if(ijkVec!=gridEx.end()){
		for(const id_t& id: ijkVec->second) extra.append(id);
	}
	return py::make_tuple(dense,extra);
}


py::tuple GridStore::pyComplements(const shared_ptr<GridStore>& B, const int& setMinSize) const{
	if(!B) throw std::runtime_error("GridStore.complements: *B* must not be None");
	shared_ptr<GridStore> A_B,B_A;
	complements(B,A_B,B_A,setMinSize);
	return py::make_tuple(A_B,B_A);
}


void GridStore::complements(const shared_ptr<GridStore>& _B, shared_ptr<GridStore>& A_B, shared_ptr<GridStore>& B_A, const int& setMinSize, const shared_ptr<TimingDeltas>& timingDeltas) const {
	GC_CHECKPOINT2("rel-compl-begin");
	const GridStore& A(*this);
	const GridStore& B(*_B);
	if(B.gridSize!=gridSize){
		std::ostringstream oss; oss<<"GridStore::complements: gridSize mismatch: this "<<gridSize<<", other "<<B.gridSize;
		throw std::runtime_error(oss.str());
	}
	assert(A.gridSize==B.gridSize);
	// use the same value of L for now
	makeCompatible(A_B,/*denseLock*/false); makeCompatible(B_A,/*denseLock*/false);
	// clear extra storage; dense storage is cleared in the parallel section for every cell separately
	A_B->clear_ex(); B_A->clear_ex();
	// traverse all cells in parallel
	// http://stackoverflow.com/questions/5572464/how-to-traverse-a-boostmulti-array - ??
	// do it the old way :|
	size_t N=linSize();
	GC_CHECKPOINT2("rel-compl-prepare");

	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)	
	#endif
	for(size_t n=0; n<N; n++){
		// static_assert(std::is_same<gridT::storage_order_type,boost::c_storage_order>::value,"Storage order of boost::multi_array is not row-major?! Code should be adjusted, as memory access pattern might be iniefficient!");
		// TODO: assert c storage order here
		// checked against http://stackoverflow.com/a/12113479/761090
		GC_CHECKPOINT2(": loop-start");
		Vector3i ijk=lin2ijk(n);
		checkIndices(ijk);
		A_B->clear_dense(ijk); B_A->clear_dense(ijk);
		GC_CHECKPOINT2(": cleared");
		size_t sizeA=A.size(ijk), sizeB=B.size(ijk);
		// if one of the sets is empty, just copy things quickly
		if(sizeA==0 || sizeB==0){
			if(sizeA!=0) for(size_t a=0; a<sizeA; a++) A_B->append(ijk,A.get(ijk,a));
			if(sizeB!=0) for(size_t b=0; b<sizeB; b++) B_A->append(ijk,B.get(ijk,b));
			GC_CHECKPOINT2(": one empty");
			continue;
		}
		// for very small sets, just two nested loops might be faster (must be tried)
		if(setMinSize>0 && (int)sizeA<setMinSize && (int)sizeB<setMinSize){
			for(size_t a=0; a<sizeA; a++){ const auto& id=A.get(ijk,a); if(!B.contains(ijk,id)) A_B->append(ijk,id); }
			for(size_t b=0; b<sizeB; b++){ const auto& id=B.get(ijk,b); if(!A.contains(ijk,id)) B_A->append(ijk,id); }
			GC_CHECKPOINT2(": nested loop");
			continue;
		}
		// use std::set and set_difference
		std::set<id_t> setA; for(size_t a=0; a<sizeA; a++) setA.insert(A.get(ijk,a));
		std::set<id_t> setB; for(size_t b=0; b<sizeB; b++) setB.insert(B.get(ijk,b));
		// push to the grid directly from within the algo,
		// rather than storing the difference in a teporary and copying
		// http://stackoverflow.com/a/8018943/761090
		boost::range::set_difference(setA,setB,boost::make_function_output_iterator([&](id_t id){ A_B->append(ijk,id); }));
		boost::range::set_difference(setB,setA,boost::make_function_output_iterator([&](id_t id){ B_A->append(ijk,id); }));
		GC_CHECKPOINT2(": set-diff");
	}
	GC_CHECKPOINT2("rel-compl-end");
}

