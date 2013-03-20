// © 2013 Václav Šmilauer <eu@doxos.eu>

#include<woo/pkg/dem/GridStore.hpp>
#include<boost/function_output_iterator.hpp>
#include<boost/range/algorithm/set_algorithm.hpp>

WOO_PLUGIN(dem,(GridStore));


GridStore::GridStore(const Vector3i& _gridSize, int _cellSize, bool _locking, int _exIniSize, int _exNumMaps): gridSize(_gridSize), cellSize(_cellSize), locking(_locking), exIniSize(_exIniSize), exNumMaps(_exNumMaps){
	postLoad(*this,NULL);
}

void GridStore::postLoad(GridStore&,void* I){
	if(I!=NULL) throw std::logic_error("GridStore::postLoad: called after a variable was set. Which one!?");
	if(gridSize.minCoeff()<=0) throw std::logic_error("GridStore.gridSize: all dimensions must be positive.");
	if(cellSize<=1) throw std::logic_error("GridStore.cellSize must be greater than one.");
	if(exNumMaps<=0) throw std::logic_error("GridStore.exNumMaps must be positive.");
	if(exIniSize<=0) throw std::logic_error("GridStore.exIniSize must be positive.");
	grid=gridT(boost::extents[gridSize[0]][gridSize[1]][gridSize[2]][cellSize+1],boost::c_storage_order());
	// check that those are default-initialized (zeros)
	assert(grid[0][0][0][0]==0);
	assert(grid[0][0][0][1]==0);
	gridExx.resize(exNumMaps);
	if(locking){
		size_t n=gridSize.prod();
		mutexes.reserve(n);
		// mutexes will be deleted automatically
		for(size_t i=0; i<n+exNumMaps; i++) mutexes.push_back(new boost::mutex);
		assert(mutexes.size()==n+exNumMaps);
	}
}

Vector3i GridStore::lin2ijk(size_t n) const{
	const auto& shape=grid.shape();
	return Vector3i(n/(shape[1]*shape[2]),(n%(shape[1]*shape[2]))/shape[2],(n%(shape[1]*shape[2]))%shape[2]);
}

size_t GridStore::ijk2lin(const Vector3i& ijk) const{
	const auto& shape=grid.shape();
	return ijk[0]*shape[1]*shape[2]+ijk[1]*shape[2]+ijk[2];
}


void GridStore::makeCompatible(shared_ptr<GridStore>& g, int l, bool _locking, int _exIniSize, int _exNumMaps) const {
	auto shape=grid.shape();
	l=l>0?l:shape[3]; // set to the real desired value of l
	assert(l>0);
	auto shape2=g->grid.shape();
	// create new grid if none or must be resized
	// (multi_array preserves elements when resizing, which is a copy operation we don't need at all;
	//  just create a new array from scratch in that case)
	if(!g || shape[0]!=shape2[0] || shape[1]!=shape2[1] || shape[2]!=shape2[2] || shape2[3]!=l || g->mutexes.size()!=(locking?shape[0]*shape[1]*shape[2]+exNumMaps:0) || (_exIniSize>0 && _exIniSize!=exIniSize) || (_exNumMaps>0 && _exNumMaps!=exNumMaps)){
		g=make_shared<GridStore>(Vector3i(shape[0],shape[1],shape[2]),l>0?l:shape[3],/*locking*/_locking,/*exIniSize*/_exIniSize>0?_exIniSize:exIniSize,/*exNumMaps*/_exNumMaps>0?_exNumMaps:exNumMaps);
		return;
	}
	assert(shape[0]==shape2[0] && shape[1]==shape2[1] && shape[2]==shape2[2] && shape2[3]==l);
	assert((!locking && g->mutexes.size()==0) || (locking && g->mutexes.size()==shape[0]*shape[1]*shape[2]+exNumMaps));
	assert(gridExx.size()==exNumMaps);
	for(auto gridEx: g->gridExx) gridEx.clear();
}

void GridStore::checkIndices(const Vector3i& ijk) const {
	assert(grid.shape()[0]==gridSize[0]);
	assert(grid.shape()[1]==gridSize[1]);
	assert(grid.shape()[2]==gridSize[2]);
	assert(ijk[0]>=0); assert(ijk[0]<grid.shape()[0]);
	assert(ijk[1]>=0); assert(ijk[1]<grid.shape()[1]);
	assert(ijk[2]>=0); assert(ijk[2]<grid.shape()[2]);
}

boost::mutex* GridStore::getMutex(const Vector3i& ijk, bool mutexEx){
	checkIndices(ijk);
	size_t ix=ijk2lin(ijk);
	if(!mutexEx){
		assert(ix<mutexes.size()-exNumMaps);
		assert(shape[0]*shape[1]*shape[2]+exNumMaps==mutexes.size());
		return &mutexes[ix];
	} else {
		assert(exNumMaps>0);
		return &mutexes[mutexes.size()-(ix%exNumMaps)];
	}
}

const GridStore::gridExT& GridStore::getGridEx(const Vector3i& ijk) const {
	size_t ix=ijk2lin(ijk);
	assert(exNumMaps>0);
	assert(gridExx.size()==muteExMod);
	return gridExx[ix%exNumMaps];
}

GridStore::gridExT& GridStore::getGridEx(const Vector3i& ijk) {
	size_t ix=ijk2lin(ijk);
	assert(exNumMaps>0);
	assert(gridExx.size()==muteExMod);
	return gridExx[ix%exNumMaps];
}

void GridStore::protected_append(const Vector3i& ijk, const GridStore::id_t& id){
	checkIndices(ijk);
	boost::mutex::scoped_lock lock(*getMutex(ijk));
	append(ijk,id,/*lockEx*/true);
}

void GridStore::append(const Vector3i& ijk, const GridStore::id_t& id, bool lockEx){
	checkIndices(ijk);
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	// 0th element is the number of elements
	int& cellSz=grid[i][j][k][0];
	assert(cellSz>=0);
	const int &denseSz=grid.shape()[3]-1; // decrement as size itself takes space
	if(cellSz<denseSz){
		// store in the dense part, shift by one because of size at the beginning
		grid[i][j][k][cellSz+1]=id;
	} else {
		assert(exIniSize>0);
		auto& gridEx=getGridEx(ijk);
		if(cellSz==denseSz){ 
			// new extension vector; gridEx[ijk] default-constructs vector<id_t>
			boost::mutex::scoped_lock lock(*getMutex(ijk,/*mutexEx*/true),boost::defer_lock);
			if(lockEx) lock.lock();
			assert(gridEx.find(ijk)==gridEx.end());
			vector<id_t>& ex=gridEx[ijk];
			if(lockEx) lock.unlock();
			ex.resize(exIniSize);
			ex[0]=id;
		} else {
			// existing extension vector
			auto exI=gridEx.find(ijk);
			assert(exI!=gridEx.end());
			vector<id_t>& ex(exI->second);
			size_t exIx=cellSz-denseSz;
			assert(exIx>0);
			assert(exIx<=ex.size());
			if(exIx==ex.size()) ex.resize(exIx+exIniSize);
			ex[exIx]=id;
		}
	}
	cellSz++;
}

void GridStore::clear_dense(const Vector3i& ijk){
	checkIndices(ijk);
	grid[ijk[0]][ijk[1]][ijk[2]][0]=0;
}

void GridStore::pyAppend(const Vector3i& ijk, GridStore::id_t id) {
	if(locking) protected_append(ijk, id);
	else append(ijk,id);
}

void GridStore::pyClear(const Vector3i& ijk){
	boost::mutex::scoped_lock lock(*getMutex(ijk),boost::defer_lock);
	if(locking) lock.lock();
	clear_dense(ijk);
	if(locking) lock.unlock();
	boost::mutex::scoped_lock lockEx(*getMutex(ijk,/*mutexEx*/true),boost::defer_lock);
	if(locking) lockEx.lock();
	auto gridEx=getGridEx(ijk);
	auto I=gridEx.find(ijk);
	if(I!=gridEx.end()) gridEx.erase(I);
	if(locking) lockEx.unlock();
}

py::list GridStore::pyGetItem(const Vector3i& ijk) const {
	size_t sz=this->size(ijk);
	py::list ret;
	for(int l=0; l<sz; l++) ret.append(this->get(ijk,l));
	return ret;
}

void GridStore::pySetItem(const Vector3i& ijk, const vector<GridStore::id_t>& ids) {
	pyClear(ijk);
	for(id_t id: ids) pyAppend(ijk,id);
}

py::dict GridStore::pyExCounts() const {
	py::dict ret;
	for(const auto& gridEx: gridExx){
		for(const auto& ijkVec: gridEx){
			ret[ijkVec.first]=size(ijkVec.first)-cellSize;
			assert(ijkVec.second.size()>size(ijkVec.first)-cellSize);
		}
	}
	return ret;
}

GridStore::id_t GridStore::get(const Vector3i& ijk, int l) const {
	checkIndices(ijk);
	assert(l>=0);
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	const int denseSz=grid.shape()[3]-1;
	__attribute__((unused)) const int cellSz=grid[i][j][k][0];
	assert(l<cellSz);
	if(l<denseSz) return grid[i][j][k][l+1];
	auto& gridEx=getGridEx(ijk);
	assert(gridEx.find(ijk)!=gridEx.end());
	auto ijkI=gridEx.find(ijk);
	assert(ijkI!=gridEx.end());
	const vector<id_t>& ex=ijkI->second;
	size_t exIx=l-denseSz;
	assert(exIx<ex.size() && exIx>=0);
	return ex[exIx];
}

size_t GridStore::size(const Vector3i& ijk) const {
	checkIndices(ijk);
	const int cellSz=grid[ijk[0]][ijk[1]][ijk[2]][0];
	return cellSz;
}

py::tuple GridStore::pyComputeRelativeComplements(GridStore& B) const{
	shared_ptr<GridStore> A_B, B_A;
	computeRelativeComplements(B,A_B,B_A);
	return py::make_tuple(A_B,B_A);
}


void GridStore::computeRelativeComplements(GridStore& B, shared_ptr<GridStore>& A_B, shared_ptr<GridStore>& B_A) const {
	//throw std::runtime_error("GridStore::computeRelativeComplements: not yet implemented.");
	const GridStore& A(*this);
	auto shape=grid.shape();
	assert(A.grid.shape()[0]==B.grid.shape()[0] && A.grid.shape()[1]==B.grid.shape()[1] && A.grid.shape()[2]==B.grid.shape()[2]);
	// use the same value of L for now
	makeCompatible(A_B,/*locking*/false); makeCompatible(B_A,/*locking*/false);
	// traverse all cells in parallel
	// http://stackoverflow.com/questions/5572464/how-to-traverse-a-boostmulti-array - ??
	// do it the old way :|
	size_t N=shape[0]*shape[1]*shape[2];
	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)	
	#endif
	for(size_t n=0; n<N; n++){
		// static_assert(std::is_same<gridT::storage_order_type,boost::c_storage_order>::value,"Storage order of boost::multi_array is not row-major?! Code should be adjusted, as memory access pattern might be iniefficient!");
		// TODO: assert c storage order here
		// checked against http://stackoverflow.com/a/12113479/761090
		Vector3i ijk=lin2ijk(n);
		checkIndices(ijk);
		A_B->clear_dense(ijk); B_A->clear_dense(ijk);
		// uset std::set and set_difference
		// for very small sets, just two nested loops might be faster
		#if 1
			std::set<id_t> setA; size_t maxA=A.size(ijk); for(size_t a=0; a<maxA; a++) setA.insert(A.get(ijk,a));
			std::set<id_t> setB; size_t maxB=B.size(ijk); for(size_t b=0; b<maxB; b++) setB.insert(B.get(ijk,b));
			// push to the grid directly from within the algo,
			// rather than storing the difference in a teporary and copying
			// http://stackoverflow.com/a/8018943/761090
			boost::range::set_difference(setA,setB,boost::make_function_output_iterator([&](id_t id){ A_B->append(ijk,id); }));
			boost::range::set_difference(setB,setA,boost::make_function_output_iterator([&](id_t id){ B_A->append(ijk,id); }));
		#endif
	}
}

