// © 2013 Václav Šmilauer <eu@doxos.eu>

#include<woo/pkg/dem/GridStore.hpp>
#include<boost/function_output_iterator.hpp>
#include<boost/range/algorithm/set_algorithm.hpp>

WOO_PLUGIN(dem,(GridStore));

CREATE_LOGGER(GridStore);


GridStore::GridStore(const Vector3i& _gridSize, int _cellLen, bool _locking, int _exIniSize, int _exNumMaps): gridSize(_gridSize), cellLen(_cellLen), locking(_locking), exIniSize(_exIniSize), exNumMaps(_exNumMaps){
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
	if(locking){
		size_t n=gridSize.prod();
		mutexes.reserve(n);
		// mutexes will be deleted automatically
		for(size_t i=0; i<n+exNumMaps; i++) mutexes.push_back(new boost::mutex);
		LOG_TRACE(mutexes.size()<<" mutexes, N="<<n<<", "<<exNumMaps<<" extension maps");
		assert(mutexes.size()==n+exNumMaps);
	}
}

Vector3i GridStore::lin2ijk(size_t n) const{
	const auto& shape=grid->shape();
	return Vector3i(n/(shape[1]*shape[2]),(n%(shape[1]*shape[2]))/shape[2],(n%(shape[1]*shape[2]))%shape[2]);
}

size_t GridStore::ijk2lin(const Vector3i& ijk) const{
	const auto& shape=grid->shape();
	return ijk[0]*shape[1]*shape[2]+ijk[1]*shape[2]+ijk[2];
}

Vector3i GridStore::sizes() const{ return Vector3i(grid->shape()[0],grid->shape()[1],grid->shape()[2]); }
size_t GridStore::linSize() const{ return sizes().prod(); }

bool GridStore::ijkOk(const Vector3i& ijk) const { return (ijk.array()>=0).all() && (ijk.array()<sizes().array()).all(); }



Vector3i GridStore::xyz2ijk(const Vector3r& xyz) const {
	// cast rounds down properly
	return ((xyz-lo).array()/cellSize.array()).cast<int>().matrix();
};

Vector3r GridStore::ijk2boxMin(const Vector3i& ijk) const{
	return lo+(ijk.cast<Real>().array()*cellSize.array()).matrix();
}

AlignedBox3r GridStore::ijk2box(const Vector3i& ijk) const{
	AlignedBox3r ret; ret.min()=ijk2boxMin(ijk); ret.max()=ret.min()+cellSize;
	return ret;
};

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

void GridStore::makeCompatible(shared_ptr<GridStore>& g, int l, bool _locking, int _exIniSize, int _exNumMaps) const {
	auto shape=grid->shape();
	l=l>0?l:shape[3]; // set to the real desired value of l
	assert(l>0);
	Vector3i gridSize2=(g?g->gridSize:Vector3i::Zero());
	// create new grid if none or must be resized
	// (multi_array preserves elements when resizing, which is a copy operation we don't need at all;
	//  just create a new array from scratch in that case)
	if(!g || gridSize!=gridSize2 || g->grid->shape()[3]!=(size_t)l || g->mutexes.size()!=(size_t)(locking?gridSize.prod()+exNumMaps:0) || (_exIniSize>0 && _exIniSize!=exIniSize) || (_exNumMaps>0 && _exNumMaps!=exNumMaps)){
		g=make_shared<GridStore>(Vector3i(shape[0],shape[1],shape[2]),l>0?l:shape[3],/*locking*/_locking,/*exIniSize*/_exIniSize>0?_exIniSize:exIniSize,/*exNumMaps*/_exNumMaps>0?_exNumMaps:exNumMaps);
		return;
	}
	assert(g);
	assert(gridSize==g->gridSize && g->cellLen==l);
	assert((!locking && g->mutexes.size()==0) || (locking && (int)g->mutexes.size()==gridSize.prod()+exNumMaps));
	assert(gridExx.size()==(size_t)exNumMaps);
	g->lo=lo;
	g->cellSize=cellSize;
}

void GridStore::clear() {
	// TODO: get id_t as a typedef from inside gridT?
	std::memset((void*)grid->data(),0,grid->num_elements()*sizeof(id_t)); 
	// std::fill(grid->origin(),grid->origin()+grid->num_elements(),0);
	for(auto gridEx: gridExx) gridEx.clear();
	#if 0
		cerr<<"Cleared grid, sizes are: "; for(const auto& c: pyCounts()) cerr<<c<<" "; cerr<<endl;
	#endif
};

void GridStore::checkIndices(const Vector3i& ijk) const {
	assert(grid->shape()[0]==(size_t)gridSize[0]);
	assert(grid->shape()[1]==(size_t)gridSize[1]);
	assert(grid->shape()[2]==(size_t)gridSize[2]);
	assert(ijk[0]>=0); assert(ijk[0]<(int)grid->shape()[0]);
	assert(ijk[1]>=0); assert(ijk[1]<(int)grid->shape()[1]);
	assert(ijk[2]>=0); assert(ijk[2]<(int)grid->shape()[2]);
}

boost::mutex* GridStore::getMutex(const Vector3i& ijk, bool mutexEx){
	checkIndices(ijk);
	size_t ix=ijk2lin(ijk);
	if(!mutexEx){
		assert(ix<mutexes.size()-exNumMaps);
		assert(gridSize.prod()+exNumMaps==(int)mutexes.size());
		return &mutexes[ix];
	} else {
		assert(exNumMaps>0);
		int mIx=mutexes.size()-(ix%exNumMaps)-1;
		assert(mIx<(int)mutexes.size() && mIx>=0);
		return &mutexes[mIx];
	}
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
	boost::mutex::scoped_lock lock(*getMutex(ijk));
	append(ijk,id,/*lockEx*/true);
}

void GridStore::append(const Vector3i& ijk, const GridStore::id_t& id, bool lockEx){
	checkIndices(ijk);
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	// 0th element is the number of elements
	int& cellSz=(*grid)[i][j][k][0];
	assert(cellSz>=0);
	const int &denseSz=grid->shape()[3]-1; // decrement as size itself takes space
	if(cellSz<denseSz){
		// store in the dense part, shift by one because of size at the beginning
		(*grid)[i][j][k][cellSz+1]=id;
	} else {
		assert(exIniSize>0);
		auto& gridEx=getGridEx(ijk);
		if(cellSz==denseSz){ 
			// new extension vector; gridEx[ijk] default-constructs vector<id_t>
			auto lock(locking?new boost::mutex::scoped_lock(*getMutex(ijk,/*mutexEx*/true),boost::defer_lock):NULL);
			if(locking) lock->lock();
			assert(gridEx.find(ijk)==gridEx.end());
			vector<id_t>& ex=gridEx[ijk];
			ex.resize(exIniSize);
			ex[0]=id;
			if(locking){ lock->unlock(); delete lock; }
		} else {
			// existing extension vector
			auto lock(locking?new boost::mutex::scoped_lock(*getMutex(ijk,/*mutexEx*/true),boost::defer_lock):NULL);
			if(locking) lock->lock();
			auto exI=gridEx.find(ijk);
			assert(exI!=gridEx.end());
			vector<id_t>& ex(exI->second);
			size_t exIx=cellSz-denseSz; // first unused index
			assert(exIx>0);
			assert(exIx<=ex.size());
			if(exIx==ex.size()) ex.resize(exIx+exIniSize);
			ex[exIx]=id;
			if(locking){ lock->unlock(); delete lock; }
		}
	}
	cellSz++;
}

void GridStore::clear_dense(const Vector3i& ijk){
	checkIndices(ijk);
	(*grid)[ijk[0]][ijk[1]][ijk[2]][0]=0;
}

void GridStore::pyAppend(const Vector3i& ijk, GridStore::id_t id) {
	if(locking) protected_append(ijk, id);
	else append(ijk,id);
}

void GridStore::pyDelItem(const Vector3i& ijk){
	auto lock(locking?new boost::mutex::scoped_lock(*getMutex(ijk),boost::defer_lock):NULL);
	if(locking) lock->lock();
	clear_dense(ijk);
	if(locking){ lock->unlock(); delete lock; }
	auto lockEx(locking?new boost::mutex::scoped_lock(*getMutex(ijk,/*mutexEx*/true),boost::defer_lock):NULL);
	if(locking) lockEx->lock();
	auto& gridEx=getGridEx(ijk);
	gridEx.erase(ijk);
	if(locking){ lockEx->unlock(); delete lockEx; }
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


GridStore::id_t GridStore::get(const Vector3i& ijk, int l) const {
	checkIndices(ijk);
	assert(l>=0);
	const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
	const int denseSz=grid->shape()[3]-1;
	__attribute__((unused)) const int cellSz=(*grid)[i][j][k][0];
	assert(l<cellSz);
	if(l<denseSz) return (*grid)[i][j][k][l+1];
	auto& gridEx=getGridEx(ijk);
	assert(gridEx.find(ijk)!=gridEx.end());
	auto ijkI=gridEx.find(ijk);
	assert(ijkI!=gridEx.end());
	const vector<id_t>& ex=ijkI->second;
	int exIx=l-denseSz; // int so that we detect underflow when debugging
	assert(exIx<(int)ex.size() && (l-denseSz>=0));
	return ex[(size_t)exIx];
}

size_t GridStore::size(const Vector3i& ijk) const {
	checkIndices(ijk);
	const int cellSz=(*grid)[ijk[0]][ijk[1]][ijk[2]][0];
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
	if(B.gridSize!=gridSize){
		std::ostringstream oss; oss<<"GridStore::computeRelativeComplements: gridSize mismatch: this "<<gridSize<<", other "<<B.gridSize;
		throw std::runtime_error(oss.str());
	}
	assert(A.gridSize==B.gridSize);
	// use the same value of L for now
	makeCompatible(A_B,/*locking*/false); makeCompatible(B_A,/*locking*/false);
	// traverse all cells in parallel
	// http://stackoverflow.com/questions/5572464/how-to-traverse-a-boostmulti-array - ??
	// do it the old way :|
	size_t N=linSize();
	// TODO: when parallelized, locking must be True and protected_append called!!
	#if 0
		#ifdef WOO_OPENMP
			#pragma omp parallel for schedule(guided)	
		#endif
	#endif
	for(size_t n=0; n<N; n++){
		// static_assert(std::is_same<gridT::storage_order_type,boost::c_storage_order>::value,"Storage order of boost::multi_array is not row-major?! Code should be adjusted, as memory access pattern might be iniefficient!");
		// TODO: assert c storage order here
		// checked against http://stackoverflow.com/a/12113479/761090
		Vector3i ijk=lin2ijk(n);
		checkIndices(ijk);
		A_B->clear_dense(ijk); B_A->clear_dense(ijk);
		size_t sizeA=A.size(ijk), sizeB=B.size(ijk);
		// if one of the sets is empty, just copy things quickly
		if(sizeA==0 || sizeB==0){
			if(sizeA!=0) for(size_t a=0; a<sizeA; a++) A_B->append(ijk,A.get(ijk,a));
			if(sizeB!=0) for(size_t b=0; b<sizeB; b++) B_A->append(ijk,B.get(ijk,b));
			continue;
		}
		#if 1
			// uset std::set and set_difference
			// TODO: for very small sets, just two nested loops might be faster (should be tried)
			std::set<id_t> setA; for(size_t a=0; a<sizeA; a++) setA.insert(A.get(ijk,a));
			std::set<id_t> setB; for(size_t b=0; b<sizeB; b++) setB.insert(B.get(ijk,b));
			// push to the grid directly from within the algo,
			// rather than storing the difference in a teporary and copying
			// http://stackoverflow.com/a/8018943/761090
			boost::range::set_difference(setA,setB,boost::make_function_output_iterator([&](id_t id){ A_B->append(ijk,id); }));
			boost::range::set_difference(setB,setA,boost::make_function_output_iterator([&](id_t id){ B_A->append(ijk,id); }));
		#endif
	}
}

