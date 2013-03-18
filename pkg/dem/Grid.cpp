// © 2013 Václav Šmilauer <eu@doxos.eu>

#include<woo/pkg/dem/Grid.hpp>

GridStore::GridStore(const Vector3i& ijk, int l, bool locking){
	assert(ijk[0]>0); assert(ijk[1]>0); assert(ijk[2]>0); assert(l>0);
	grid=gridT(boost::extents[ijk[0]][ijk[1]][ijk[2]][l+1]);
	// check that those are default-initialized (zeros)
	assert(grid[0][0][0][0]==0);
	assert(grid[0][0][0][1]==0);
	if(locking){
		size_t n=ijk[0]*ijk[1]*ijk[2];
		mutexes.reserve(n);
		// mutexes will be deleted automatically
		for(size_t i=0; i<n; i++) mutexes.push_back(new boost::mutex);
		assert(mutexes.size()==n);
	}
}

void GridStore::checkIndices(const Vector3i& ijk) const {
	assert(ijk[0]>=0); assert(ijk[0]<grid.shape()[0]);
	assert(ijk[1]>=0); assert(ijk[1]<grid.shape()[1]);
	assert(ijk[2]>=0); assert(ijk[2]<grid.shape()[2]);
}

boost::mutex* GridStore::getMutex(const Vector3i& ijk){
	checkIndices(ijk);
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	auto shape=grid.shape();
	assert(i*shape[1]*shape[2]+j*shape[2]+k<mutexes.size());
	assert(shape[0]*shape[1]*shape[2]==mutexes.size());
	return &mutexes[i*shape[1]*shape[2]+j*shape[2]+k];
}

void GridStore::append(const Vector3i& ijk, const GridStore::id_t& id){
	checkIndices(ijk);
	boost::mutex::scoped_lock lock(*getMutex(ijk));
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	// 0th element is the number of elements
	int& cellSz=grid[i][j][k][0];
	assert(cellSz>=0);
	const int &denseSz=grid.shape()[3]-1; // decrement as size itself takes space
	if(cellSz<denseSz){
		// store in the dense part, shift by one because of size at the beginning
		grid[i][j][k][cellSz+1]=id;
	} else {
		assert(mapInitSz>0);
		assert((cellSz==denseSz && gridEx.find(ijk)==gridEx.end()) || (cellSz>denseSz && gridEx.find(ijk)!=gridEx.end()));
		if(cellSz==denseSz){ 
			// new extension vector; gridEx[ijk] default-constructs vector<id_t>
			vector<id_t>& ex=gridEx[ijk]; ex.resize(mapInitSz);
			ex[0]=id;
		} else {
			// existing extension vector
			vector<id_t>& ex=gridEx[ijk];
			size_t exIx=cellSz-denseSz;
			assert(exIx>0);
			assert(exIx<=ex.size());
			if(exIx==ex.size()) ex.resize(exIx+mapInitSz);
			ex[exIx]=id;
		}
	}
	cellSz++;
}

GridStore::id_t GridStore::get(const Vector3i& ijk, int l) {
	checkIndices(ijk);
	assert(l>=0);
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	const int denseSz=grid.shape()[3]-1;
	__attribute__((unused)) const int cellSz=grid[i][j][k][0];
	assert(l<cellSz);
	if(l<denseSz) return grid[i][j][k][l+1];
	assert(gridEx.find(ijk)!=gridEx.end());
	const vector<id_t>& ex=gridEx[ijk];
	size_t exIx=l-denseSz;
	assert(exIx<ex.size() && exIx>=0);
	return ex[exIx];
}

size_t GridStore::size(const Vector3i& ijk) const {
	checkIndices(ijk);
	const int cellSz=grid[ijk[0]][ijk[1]][ijk[2]][0];
	return cellSz;
}

shared_ptr<GridStore> GridStore::relativeComplement(const shared_ptr<GridStore>& g2) const{
	throw std::runtime_error("GridStore::relativeComplement: not yet implemented.");
	auto shape=grid.shape();
	// return non-locking GridStore
	shared_ptr<GridStore> ret=make_shared<GridStore>(Vector3i(shape[0],shape[1],shape[2]),shape[3]-1,/*allocMutexes*/false);
	return ret;
}

