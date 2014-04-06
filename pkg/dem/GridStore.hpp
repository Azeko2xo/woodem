#pragma once
// © 2013 Václav Šmilauer <eu@doxos.eu>


#include<woo/lib/object/Object.hpp>
#include<boost/multi_array.hpp>
#include<woo/pkg/dem/Particle.hpp>
// http://stackoverflow.com/questions/8252909/how-to-define-a-vectorboostmutex-in-c
#include<boost/ptr_container/ptr_vector.hpp>

/* TODO:
	python inspection
	rudimentary self-tests
	boost::serialization of multi_array perhaps (storage is guaranteed to be contiguous)?

	3d grid for storing ids. Each cell stores a fixed number of ids in dense static array, and may store additional ids as necessary in several maps, mapping ijk (3d integral coordinate) to vector<id_t>. Those maps are split using modulo of the linear index so that access to those is more fine-grained than having to lock the whole structure.

	The static (dense) array stores number of items in that cell as the first element.

	Each cell might be protected by a mutex for writing, as well as each map ijk->ids (protected_append).

*/
struct GridStore: public Object{
	typedef Particle::id_t id_t;
	enum {ID_NONE=-1 };
	struct Vector3iComparator{ bool operator()(const Vector3i& a, const Vector3i& b) const { return (a[0]<b[0] || (a[0]==b[0] && (a[1]<b[1] || (a[1]==b[1] && a[2]<b[2])))); };	};
	typedef boost::multi_array<id_t,4> gridT;
	typedef std::map<Vector3i,vector<id_t>,Vector3iComparator> gridExT;
	typedef boost::ptr_vector<boost::mutex> mutexesT;
	std::unique_ptr<gridT> grid;
	vector<gridExT> gridExx;
	mutexesT mutexes;

	// obtain gridEx for given cell
	const gridExT& getGridEx(const Vector3i& ijk) const;
	// non-const reference, otherwise the same
	gridExT& getGridEx(const Vector3i& ijk);

	/* inlined one-liners */
	// convert linear index to grid indices
	Vector3i lin2ijk(size_t n) const { const auto& shape=grid->shape(); return Vector3i(n/(shape[1]*shape[2]),(n%(shape[1]*shape[2]))/shape[2],(n%(shape[1]*shape[2]))%shape[2]); }
	// convert grid indices to linear index
	size_t ijk2lin(const Vector3i& ijk) const { const auto& shape=grid->shape(); return ijk[0]*shape[1]*shape[2]+ijk[1]*shape[2]+ijk[2]; }
	// return 3d grid sizes (upper bound of grid indices)
	Vector3i sizes() const { return Vector3i(grid->shape()[0],grid->shape()[1],grid->shape()[2]); }
	// return grid storage size (upper bound of linear index)
	size_t linSize() const { return sizes().prod(); }
	// return false if the given index is out of range
	bool ijkOk(const Vector3i& ijk) const {  return (ijk.array()>=0).all() && (ijk.array()<sizes().array()).all(); }

	// say in which cell lies given spatial coordinate
	// no bound checking is done, caller must assure the result makes sense
	Vector3i xyz2ijk(const Vector3r& xyz) const { /* cast rounds down properly */ return ((xyz-lo).array()/cellSize.array()).cast<int>().matrix(); }
	AlignedBox3i xxyyzz2iijjkk(const AlignedBox3r& box) const{ return AlignedBox3i(xyz2ijk(box.min()),xyz2ijk(box.max())); }
	// bounding box for given cell
	AlignedBox3r ijk2box(const Vector3i& ijk) const { AlignedBox3r ret; ret.min()=ijk2boxMin(ijk); ret.max()=ret.min()+cellSize; return ret; }
	AlignedBox3r iijjkk2box(const Vector3i& lo, const Vector3i& hi) const { return AlignedBox3r(ijk2boxMin(lo),ijk2boxMin(hi+Vector3i::Ones())); }
	// lower corner of given cell
	Vector3r ijk2boxMin(const Vector3i& ijk) const { return lo+(ijk.cast<Real>().array()*cellSize.array()).matrix(); }
	Vector3r ijk2boxMiddle(const Vector3i& ijk) const { return lo+((ijk.cast<Real>()+.5*Vector3r::Ones()).array()*cellSize.array()).matrix(); }

	// clamp given AlignedBox3i so that all its cells are valid
	void clamp(AlignedBox3i& iijjkk){ iijjkk.clamp(AlignedBox3i(Vector3i::Zero(),/*inclusive upper bound*/sizes()-Vector3i::Ones())); }

	/* bigger, not inlined */
	// bounding box shrunk by dimensionless *shrink* (relative size)
	AlignedBox3r ijk2boxShrink(const Vector3i& ijk, const Real& shrink) const;
	// point nearest to given point *from* in cell *ijk*
	Vector3r xyzNearXyz(const Vector3r& xyz, const Vector3i& ijk) const;
	// point nearest to given cell *from* in cell *ijk*
	Vector3r xyzNearIjk(const Vector3i& from, const Vector3i& ijk) const;
	// squared distance between Aabb and cell *ijk*
	Real boxCellDistSq(const AlignedBox3r& box, const Vector3i& ijk) const { return box.squaredExteriorDistance(ijk2box(ijk)); }


	DECLARE_LOGGER;

	/*
		the first ID is not actually ID, but the number of IDs for that cell;
		it makes iteration faster, especially there is no lookup into the map
		if there is nothing
	*/

	// ctor; allocate grid and locks (if desired)
	GridStore(const Vector3i& ijk, int l, bool locking, int _exIniSize, int _exNumMaps);

	void postLoad(GridStore&,void*);
	

	// set g to be same-shape grid witout mutexes and gridEx 
	// if l is given and positive, use that value instead of the current shape[3]
	// grid may contain garbage data in the dense part, call clear() or clear_ex()+clear_dense() before writing in there!
	void makeCompatible(shared_ptr<GridStore>& g, int l=0, bool locking=true, int _exIniSize=-1, int _exNumMaps=-1) const;
	bool isCompatible(shared_ptr<GridStore>& other);
	// clear both dense and extra storage
	// dense storage cleared with memset, should be very fast
	void clear();
	// clear all extra storage (length in dense stoage may contain garbage then)
	void clear_ex(); 

	// return lock pointer for given cell
	template<bool mutexEx>
	boost::mutex* getMutex(const Vector3i& ijk){
		checkIndices(ijk); size_t ix=ijk2lin(ijk);
		if(!mutexEx){ assert(denseLock); assert(ix<(size_t)gridSize.prod()); assert(gridSize.prod()+exNumMaps==(int)mutexes.size()); return &mutexes[exNumMaps+ix];
		} else { assert(exNumMaps>0); int mIx=ix%exNumMaps; assert(mIx<(int)mutexes.size() && mIx>=0); return &mutexes[mIx]; }
	}


	// inlined, disappears in optimized builds (no-op)
	void checkIndices(const Vector3i& ijk) const {
		assert(grid->shape()[0]==(size_t)gridSize[0]);
		assert(grid->shape()[1]==(size_t)gridSize[1]);
		assert(grid->shape()[2]==(size_t)gridSize[2]);
		assert(ijk[0]>=0); assert(ijk[0]<(int)grid->shape()[0]);
		assert(ijk[1]>=0); assert(ijk[1]<(int)grid->shape()[1]);
		assert(ijk[2]>=0); assert(ijk[2]<(int)grid->shape()[2]);
	}


	// thread safe: clear dense storage (set count to 0), don't touch extra storage
	void clear_dense(const Vector3i& ijk);
	// thread unsafe: add element to the cell (in grid, or, if full, in gridEx)
	// noSizeInc only used internally
	void append(const Vector3i& ijk, const id_t&, bool noSizeInc=false);
	// thread safe: lock (always) and append
	void protected_append(const Vector3i& ijk, const id_t&);

	/* compute relative complement (B\this)≡ relative complement of this with respect to B and vice versa
		(i.e. return grid with ids in g2 but NOT in this)
		this function is internally parallelized, and needs no locking
	
		setMinSize: if positive, use trivial nested loops for finding the difference as long as both sets length is
		smaller than setMinSize. If negative, always use std::set algorithms (may be slower)
	
		A_B will contain elements in B but not in A
		B_A will contain elements in B but not in A
	*/
	void complements(const shared_ptr<GridStore>& B, shared_ptr<GridStore>& A_B, shared_ptr<GridStore>& B_A, const int& setMinSize=-1, const shared_ptr<TimingDeltas>& timingDeltas=shared_ptr<TimingDeltas>()) const;
	py::tuple pyComplements(const shared_ptr<GridStore>& B, const int& setMinSize=-1) const;

	/* put those get() and size() inline, since they are used rather frequently */

	// return i-th element, from grid or gridEx depending on l
	inline const id_t& get(const Vector3i& ijk, const int& l) const{
		checkIndices(ijk); assert(l>=0);
		const int& i(ijk[0]), &j(ijk[1]), &k(ijk[2]);
		const int denseSz=grid->shape()[3]-1; // first item is total cell size
		assert(l<(int)size(ijk));
		if(l<denseSz) return (*grid)[i][j][k][l+1];
		const auto& gridEx=getGridEx(ijk);
		const auto& ijkI=gridEx.find(ijk);
		if(ijkI==gridEx.end()) LOG_FATAL("ijk="<<ijk.transpose()<<", l="<<l<<", denseSz="<<denseSz);
		assert(ijkI!=gridEx.end());
		const vector<id_t>& ex=ijkI->second;
		int exIx=l-denseSz; /* int so that we detect underflow when debugging */
		assert(exIx<(int)ex.size() && (l-denseSz>=0));
		return ex[(size_t)exIx];
	}

	// return number of elements in grid+gridEx
	size_t size(const Vector3i& ijk) const{
		checkIndices(ijk);
		const int cellSz=(*grid)[ijk[0]][ijk[1]][ijk[2]][0];
		return cellSz;
	}

	// predicate whether given id is in a cell at ijk (traverses all elements)
	bool contains(const Vector3i& ijk, const id_t& id) const {
		size_t sz=size(ijk);
		for(size_t i=0; i<sz; i++){ if(get(ijk,i)==id) return true; }
		return false;
	}

	py::list pyGetItem(const Vector3i& ijk) const;
	void pySetItem(const Vector3i& ijk, const vector<id_t>& ids);
	void pyDelItem(const Vector3i& ijk);
	void pyAppend(const Vector3i& ijk, id_t id);
	py::dict pyCountEx() const;
	py::tuple pyRawData(const Vector3i& ijk);
	vector<int> pyCounts() const;

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(GridStore,Object,"3d grid storing scalar (particles ids) in partially dense array; the grid is actually 4d (gridSize×cellLen), and each cell may contain additional items in separate mapped storage, if the cellLen is not big enough to accomodate required number of items. Appending to cells is guarded via mutexes (with :obj:`denseLock`) optionally, appending to extra storage is mutex-protected always. Write acces from python should be used for testing exclusively.",
		((Vector3i,gridSize,Vector3i(1,1,1),AttrTrait<>().readonly(),"Dimension of the grid."))
		((int,cellLen,4,AttrTrait<>().readonly(),"Size of the dense storage in each cell"))
		((bool,denseLock,true,AttrTrait<>().readonly(),"Whether this grid supports per-cell dense storage locking for appending (must use protected_append)"))
		((int,exIniSize,4,AttrTrait<>().readonly(),"Initial size of extension vectors, and step of their growth if needed."))
		((int,exNumMaps,10,AttrTrait<>().readonly(),"Number of maps for extra items not fitting the dense storage (it affects how fine-grained is locking for those extra elements)"))
		((Vector3r,lo,Vector3r(NaN,NaN,NaN),,"Lower corner of the domain."))
		((Vector3r,cellSize,Vector3r(NaN,NaN,NaN),,"Spatial size of the grid cell."))
		, /*ctor*/
		, /*py*/
			.def("__getitem__",&GridStore::pyGetItem)
			.def("__setitem__",&GridStore::pySetItem)
			.def("__delitem__",&GridStore::pyDelItem)
			.def("size",&GridStore::size)
			.def("append",&GridStore::pyAppend,(py::arg("ijk"),py::arg("id")),"Append new element; uses mutexes with :obj:`denseLock`")
			.def("countEx",&GridStore::pyCountEx,"Return dictionary mapping ijk to number of items in the extra storage.")
			.def("_rawData",&GridStore::pyRawData,"Return raw data, as tuple of dense store and extra store.")
			.def("complements",&GridStore::pyComplements,(py::arg("B"),py::arg("setMinSize")=-1))
			.def("counts",&GridStore::pyCounts,"Return array with number of cells with given number of elements: [number of cells with 0 elements, number of cells with 1 elements, ...]")
			// .def("clear",&GridStore::pyClear,py::arg("ijk"),"Clear both dense and map storage for given cell; uses mutexes if the instance is :obj:`locking`.")
			.def("lin2ijk",&GridStore::lin2ijk)
			.def("ijk2lin",&GridStore::ijk2lin)
			.def("xyz2ijk",&GridStore::xyz2ijk,"Convert spatial coordinates to cell coordinate (no bound checking is done)")
			.def("xyzNearXyz",&GridStore::xyzNearXyz,(py::arg("from"),py::arg("ijk")),"Return point nearest to *from*(given in spatial coords) in cell *ijk* (corner, at a face, on an edge, inside)")
			.def("xyzNearIjk",&GridStore::xyzNearIjk,(py::arg("from"),py::arg("ijk")),"Return point nearest to *from* (given in cell coords) in cell *ijk*")
			.def("ijk2box",&GridStore::ijk2box,(py::arg("ijk")),"Box for given cell")
			// .def("makeCompatible")
			;
	);
};
WOO_REGISTER_OBJECT(GridStore);
