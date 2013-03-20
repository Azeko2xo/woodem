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

	3d grid for storing ids. Each cell stores a fixed number of ids in dense static array, and may store additional ids as necessary in several maps, mapping ijk (3d integral coordinate) to vector<id_t>. Those maps are split using modulo of the linear index so that access to those is more fin-egrained than having to lock the whole structure.

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

	Vector3i lin2ijk(size_t n) const;
	size_t ijk2lin(const Vector3i& ijk) const;

	/*
		the first ID is not actually ID, but the number of IDs for that cell;
		it makes iteration faster, especially there is no lookup into the map
		if there is nothing
	*/

	// ctor; allocate grid and locks (if desired)
	GridStore(const Vector3i& ijk, int l, bool locking, int _mapInitSz, int _mutexExMod);

	void postLoad(GridStore&,void*);
	

	// set g to be same-shape grid witout mutexes and gridEx 
	// if l is given and positive, use that value instead of the current shape[3]
	// grid may contain garbage data!
	void makeCompatible(shared_ptr<GridStore>& g, int l=0, bool locking=true, int _exIniSize=-1, int _exNumMaps=-1) const;

	// return lock pointer for given cell
	boost::mutex* getMutex(const Vector3i& ijk, bool mutexEx=false);
	// assert that indices are OK, no-op in optimizes build
	void checkIndices(const Vector3i& ijk) const;

	// thread unsafe: clear dense storage
	void clear_dense(const Vector3i& ijk);
	// thread unsafe: add element to the cell (in grid, or, if full, in gridEx)
	void append(const Vector3i& ijk, const id_t&, bool lockEx=false);
	// thread safe: lock and append
	void protected_append(const Vector3i& ijk, const id_t&);

	// compute relative complements (this\B) and (B\this)
	// (i.e. return grid with ids in g2 but NOT in this and vice versa)
	// this function is internally parallelized, and needs no locking
	//
	// A_B contains only elements in B but not in A
	// B_A contains only elements in A but not in B
	void computeRelativeComplements(GridStore& B, shared_ptr<GridStore>& A_B, shared_ptr<GridStore>& B_A) const;
	py::tuple pyComputeRelativeComplements(GridStore& B) const;

	// return i-th element, from grid or gridEx depending on l
	id_t get(const Vector3i& ijk, int l) const;
	// return number of elements in grid+gridEx
	size_t size(const Vector3i& ijk) const;

	py::list pyGetItem(const Vector3i& ijk) const;
	void pySetItem(const Vector3i& ijk, const vector<id_t>& ids);
	void pyClear(const Vector3i& ijk);
	void pyAppend(const Vector3i& ijk, id_t id);
	py::dict pyExCounts() const;
	py::tuple pyRawData(const Vector3i& ijk);

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(GridStore,Object,"3d grid storing scalar (particles ids) in partially dense array; the grid is actually 4d (gridSize×cellSize), and each cell may contain additional items in separate mapped storage, if the cellSize is not big enough to accomodate required number of items. If instance may synchronize (with *locking*=`True`) access via per-cell mutexes (or per-map mutexes) if it is written from multiple threads. Write acces from python should be used for testing exclusively.",
		((Vector3i,gridSize,Vector3i(1,1,1),AttrTrait<>().readonly(),"Dimension of the grid."))
		((int,cellSize,4,AttrTrait<>().readonly(),"Size of the dense storage in each cell"))
		((bool,locking,true,AttrTrait<>().readonly(),"Whether this grid locks elements on access"))
		((int,exIniSize,4,AttrTrait<>().readonly(),"Initial size of extension vectors, and step of their growth if needed."))
		((int,exNumMaps,10,AttrTrait<>().readonly(),"Number of maps for extra items not fitting the dense storage (it affects how fine-grained is locking for those extra elements)"))
		, /*ctor*/
		, /*py*/
			.def("__getitem__",&GridStore::pyGetItem)
			.def("__setitem__",&GridStore::pySetItem)
			.def("append",&GridStore::pyAppend,(py::arg("ijk"),py::arg("id")),"Append new element; uses mutexes if the instance is `locking`")
			.def("clear",&GridStore::pyClear,py::arg("ijk"),"Clear both dense and map storage for given cell; uses mutexes if the instance is :obj:`locking`.")
			.def("lin2ijk",&GridStore::lin2ijk)
			.def("ijk2lin",&GridStore::ijk2lin)
			.def("exCounts",&GridStore::pyExCounts,"Return dictionary mapping ijk to number of items in the extra storage.")
			.def("_rawData",&GridStore::pyRawData,"Return raw data, as tuple of dense store and extra store.")
			.def("computeRelativeComplements",&GridStore::pyComputeRelativeComplements)
			// .def("makeCompatible")
			;
	);
};
