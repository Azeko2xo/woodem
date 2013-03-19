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
	gridT grid;
	// initial size of vector in the map
	const int mapInitSz=3;
	// use several maps so that write-access can be mutually exclusive in a finer way
	const int mutexExMod=10;
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
	GridStore(const Vector3i& ijk, int l, bool locking=true);

	// set g to be same-shape grid witout mutexes and gridEx 
	// if l is given and positive, use that value instead of the current shape[3]
	// grid may contain garbage data!
	void makeCompatible(shared_ptr<GridStore>& g, int l=0, bool locking=true) const;

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
	void computeRelativeComplements(GridStore& B, shared_ptr<GridStore>& A_B, shared_ptr<GridStore>& B_A);

	// return i-th element, from grid or gridEx depending on l
	id_t get(const Vector3i& ijk, int l) const;
	// return number of elements in grid+gridEx
	size_t size(const Vector3i& ijk) const;
};
