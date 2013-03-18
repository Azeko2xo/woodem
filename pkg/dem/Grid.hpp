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
*/
struct GridStore: public Object{
	typedef Particle::id_t id_t;
	enum {ID_NONE=-1 };
	struct Vector3iComparator{ bool operator()(const Vector3i& a, const Vector3i& b){ return (a[0]<b[0] || (a[0]==b[0] && (a[1]<b[1] || (a[1]==b[1] && a[2]<b[2])))); };	};
	typedef boost::multi_array<id_t,4> gridT;
	typedef std::map<Vector3i,vector<id_t>,Vector3iComparator> gridExT;
	typedef boost::ptr_vector<boost::mutex> mutexesT;
	gridT grid;
	gridExT gridEx;
	mutexesT mutexes;
	const int mapInitSz=3;
	/*
		the first ID is not actually ID, but the number of IDs for that cell;
		it makes iteration faster, especially there is no lookup into the map
		if there is nothing
	*/

	// ctor; allocate grid and locks (if desired)
	GridStore(const Vector3i& ijk, int l, bool locking=true);
	// return lock pointer for given cell
	boost::mutex* getMutex(const Vector3i& ijk);
	// assert that indices are OK, no-op in optimizes build
	void checkIndices(const Vector3i& ijk) const;

	// lock and add element to the cell (in grid, or, if full, in gridEx)
	void append(const Vector3i& ijk, const id_t&);

	// compute relative complement of this WRT g2, and return it
	// (i.e. return grid with ids in g2 but NOT in this)
	// this function is internally parallelized, and needs no locking
	shared_ptr<GridStore> relativeComplement(const shared_ptr<GridStore>& g2) const;

	// return i-th element, from grid or gridEx depending on l
	id_t get(const Vector3i& ijk, int l);
	// return number of elements in grid+gridEx
	size_t size(const Vector3i& ijk) const;
};
