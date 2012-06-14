// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#pragma once
#include<yade/pkg/dem/Collision.hpp>
#include<yade/core/Scene.hpp>


/*! Periodic collider notes.

Use
===
* scripts/test/periodic-simple.py
* In the future, triaxial compression working by growing/shrinking the cell should be implemented.

Architecture
============
Values from bounding boxes are added information about period in which they are.
Their container (VecBounds) holds position of where the space wraps.
The sorting algorithm is changed in such way that periods are changed when body crosses cell boundary.

Interaction::cellDist holds information about relative cell coordinates of the 2nd body
relative to the 1st one. Dispatchers (IGeomDispatcher and InteractionLoop)
use this information to pass modified position of the 2nd body to IGeomFunctors.
Since properly behaving IGeomFunctor's and LawFunctor's do not take positions
directly from Body::state, the interaction is computed with the periodic positions.

Positions of bodies (in the sense of Body::state) and their natural bboxes are not wrapped
to the periodic cell, they can be anywhere (but not "too far" in the sense of int overflow).

Since Interaction::cellDists holds cell coordinates, it is possible to change the cell boundaries
at runtime. This should make it easy to implement some stress control on the top.

Clumps do not interfere with periodicity in any way.

Rendering
---------
OpenGLRenderer renders Shape at all periodic positions that touch the
periodic cell (i.e. Bounds crosses its boundary).

It seems to affect body selection somehow, but that is perhaps not related at all.

Periodicity control
===================
c++:
	Scene::isPeriodic, Scene::cellSize
python:
	O.periodicCell=((0,0,0),(10,10,10)  # activates periodic boundary
	O.periodicCell=() # deactivates periodic boundary

Requirements
============
* No body can have Aabb larger than about .499*cellSize. Exception is thrown if that is false.
* Constitutive law must not get body positions from Body::state directly.
	If it does, it uses Interaction::cellDist to compute periodic position.
	Dem3Dof family of Ig2 functors and Law2_* engines are known to behave well.
* No body can get further away than MAXINT periods. It will do horrible things if there is overflow. Not checked at the moment.
* Body cannot move over distance > cellSize in one step. Since body size is limited as well, that would mean simulation explosion.
	Exception is thrown if the movement is upwards. If downwards, it is not handled at all.

Possible performance improvements & bugs
========================================

* PeriodicInsertionSortCollider::{cellWrap,cellWrapRel} OpenGLRenderer::{wrapCell,wrapCellPt} Shop::PeriodicWrap
	are all very similar functions. They should be put into separate header and included from all those places.

*/


// #define this macro to enable timing within this engine
//#define ISC_TIMING

// #define to turn on some tracing information for the periodic part
// all code under this can be probably removed at some point, when the collider will have been tested thoroughly
// #define PISC_DEBUG


#ifdef ISC_TIMING
	#define ISC_CHECKPOINT(cpt) timingDeltas->checkpoint(cpt)
#else
	#define ISC_CHECKPOINT(cpt)
#endif

// #define YADE_VBINS

#ifdef YADE_VBINS
class Leapfrog;
#endif

class ParticleContainer;

struct InsertionSortCollider: public Collider {
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }

	private:

	//! struct for storing bounds of bodies
	struct Bounds{
		//! coordinate along the given sortAxis
		Real coord;
		//! id of the particle this bound belongs to
		Particle::id_t id;
		//! periodic cell coordinate
		int period;
		//! is it the minimum (true) or maximum (false) bound?
		struct{ unsigned hasBB:1; unsigned isMin:1; } flags;
		Bounds(Real coord_, Particle::id_t id_, bool isMin): coord(coord_), id(id_), period(0){ flags.isMin=isMin; }
		bool operator<(const Bounds& b) const {
			/* handle special case of zero-width bodies, which could otherwise get min/max swapped in the unstable std::sort */
			if(id==b.id && coord==b.coord) return flags.isMin;
			return coord<b.coord;
		}
		bool operator>(const Bounds& b) const {
			if(id==b.id && coord==b.coord) return !flags.isMin;
			return coord>b.coord;
		}
	};
	#ifdef PISC_DEBUG
		int watch1, watch2;
		bool watchIds(Particle::id_t id1,Particle::id_t id2) const { return (watch1<0 &&(watch2==id1||watch2==id2))||(watch2<0 && (watch1==id1||watch1==id2))||(watch1==id1 && watch2==id2)||(watch1==id2 && watch2==id1); }
	#endif
		#ifdef YADE_VBINS
			// we need this to find out about current maxVelocitySq
			shared_ptr<Leapfrog> leapfrog;
		#endif
			// if False, no type of striding is used
			// if True, then either verletDist XOR nBins is set
			bool strideActive;
	struct VecBounds{
		// axis set in the ctor
		int axis;
		std::vector<Bounds> vec;
		Real cellDim;
		// cache vector size(), update at every step in run()
		long size;
		// index of the lowest coordinate element, before which the container wraps
		long loIdx;
		Bounds& operator[](long idx){ assert(idx<size && idx>=0); return vec[idx]; }
		const Bounds& operator[](long idx) const { assert(idx<size && idx>=0); return vec[idx]; }
		// update number of bodies, periodic properties and size from Scene
		void updatePeriodicity(Scene* scene){
			assert(scene->isPeriodic);
			assert(axis>=0 && axis <=2);
			cellDim=scene->cell->getSize()[axis];
		}
		// normalize given index to the right range (wraps around)
		long norm(long i) const { if(i<0) i+=size; long ret=i%size; assert(ret>=0 && ret<size); return ret;}
		VecBounds(): axis(-1), size(0), loIdx(0){}
		void dump(std::ostream& os){ string ret; for(size_t i=0; i<vec.size(); i++) os<<((long)i==loIdx?"@@ ":"")<<vec[i].coord<<"(id="<<vec[i].id<<","<<(vec[i].flags.isMin?"min":"max")<<",p"<<vec[i].period<<") "; os<<endl;}
	};
	private:
	//! storage for bounds
	VecBounds BB[3];
	//! storage for bb maxima and minima
	std::vector<Real> maxima, minima;
	//! Whether the Scene was periodic (to detect the change, which shouldn't happen, but shouldn't crash us either)
	bool periodic;

	protected:
	// updated at every step
	ParticleContainer* particles;
	DemField* dem;

	// return python representation of the BB struct, as ([...],[...],[...]).
	py::tuple dumpBounds();
	py::object dbgInfo();

	/*! sorting routine; insertion sort is very fast for strongly pre-sorted lists, which is our case
  	    http://en.wikipedia.org/wiki/Insertion_sort has the algorithm and other details
	*/
	Vector3i countInversions(); // for debugging only
	void insertionSort(VecBounds& v,bool doCollide=true, int ax=0);
	void handleBoundInversion(Particle::id_t,Particle::id_t);
	bool spatialOverlap(Particle::id_t,Particle::id_t) const;

	// periodic variants
	void insertionSortPeri(VecBounds& v,bool doCollide=true, int ax=0);
	void handleBoundInversionPeri(Particle::id_t,Particle::id_t);
	bool spatialOverlapPeri(Particle::id_t,Particle::id_t,Scene*,Vector3i&) const;
	static Real cellWrap(const Real, const Real, const Real, int&);
	static Real cellWrapRel(const Real, const Real, const Real);


	public:
	//! Predicate called from loop within ContactContainer::erasePending
	bool shouldBeRemoved(const shared_ptr<Contact> &C, Scene* scene) const {
		Particle::id_t id1=C->pA->id, id2=C->pB->id;
		if(!periodic) return !spatialOverlap(id1,id2);
		else { Vector3i periods; return !spatialOverlapPeri(id1,id2,scene,periods); }
	}
	virtual bool isActivated();



	// force reinitialization at next run
	virtual void invalidatePersistentData(){ for(int i=0; i<3; i++){ BB[i].vec.clear(); BB[i].size=0; }}
	// initial setup (reused in derived classes)
	bool prologue_doFullRun(); 
	// check whether bounding boxes are bounding
	bool updateBboxes_doFullRun();

	vector<Particle::id_t> probeAabb(const Vector3r& mn, const Vector3r& mx);

	virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(InsertionSortCollider,Collider,"\
		Collider with O(n log(n)) complexity, using :yref:`Aabb` for bounds.\
		\n\n\
		At the initial step, Bodies' bounds (along sortAxis) are first std::sort'ed along one axis (sortAxis), then collided. The initial sort has :math:`O(n^2)` complexity, see `Colliders' performance <https://yade-dem.org/index.php/Colliders_performace>`_ for some information (There are scripts in examples/collider-perf for measurements). \
		\n\n \
		Insertion sort is used for sorting the bound list that is already pre-sorted from last iteration, where each inversion	calls checkOverlap which then handles either overlap (by creating interaction if necessary) or its absence (by deleting interaction if it is only potential).	\
		\n\n \
		Bodies without bounding volume (such as clumps) are handled gracefully and never collide. Deleted bodies are handled gracefully as well.\
		\n\n \
		This collider handles periodic boundary conditions. There are some limitations, notably:\
		\n\n \
			#. No body can have Aabb larger than cell's half size in that respective dimension. You get exception it it does and gets in interaction.\
			\n\n \
			#. No body can travel more than cell's distance in one step; this would mean that the simulation is numerically exploding, and it is only detected in some cases.\
		\n\n \
		**Stride** can be used to avoid running collider at every step by enlarging the particle's bounds, tracking their velocities and only re-run if they might have gone out of that bounds (see `Verlet list <http://en.wikipedia.org/wiki/Verlet_list>`_ for brief description and background) . This requires cooperation from :yref:`Leapfrog` as well as :yref:`BoundDispatcher`, which will be found among engines automatically (exception is thrown if they are not found).\
		\n\n \
		If you wish to use strides, set ``verletDist`` (length by which bounds will be enlarged in all directions) to some value, e.g. 0.05 × typical particle radius. This parameter expresses the tradeoff between many potential interactions (running collider rarely, but with longer exact interaction resolution phase) and few potential interactions (running collider more frequently, but with less exact resolutions of interactions); it depends mainly on packing density and particle radius distribution.\
		\n\n \
		If you additionally set ``nBins`` to >=1, not all particles will have their bound enlarged by ``verletDist``; instead, they will be put to bins (in the statistical sense) based on magnitude of their velocity; ``verletDist`` will only be used for particles in the fastest bin, whereas only proportionally smaller length will be used for slower particles; The coefficient between bin's velocities is given by ``binCoeff``.\
	",
		((bool,forceInitSort,false,,"When set to true, full sort will be run regardless of other conditions. This flag is then reset automatically to false"))
		((int,sortAxis,0,,"Axis for the initial contact detection."))
		((bool,sortThenCollide,false,,"Separate sorting and colliding phase; it is MUCH slower, but all interactions are processed at every step; this effectively makes the collider non-persistent, not remembering last state. (The default behavior relies on the fact that inversions during insertion sort are overlaps of bounding boxes that just started/ceased to exist, and only processes those; this makes the collider much more efficient.)"))
		((Real,verletDist,((void)"Automatically initialized",-.05),,"Length by which to enlarge particle bounds, to avoid running collider at every step. Stride disabled if zero. Negative value will trigger automatic computation, so that the real value will be |verletDist| × minimum spherical particle radius; if there are no spherical particles, it will be disabled."))
		((Real,maxVel2,0,AttrTrait<Attr::readonly>(),"Maximum encountered velocity of a particle, to compute bounding box shift."))
		((int,nFullRuns,0,,"Number of full runs, when collision detection is needed; only informative."))
		((int,numReinit,0,AttrTrait<Attr::readonly>(),"Cumulative number of bound array re-initialization."))
		((Vector3i,stepInvs,Vector3i::Zero(),,"Number of inversions in insertion sort in the last step; always zero in the non-debug builds"))
		((Vector3i,numInvs,Vector3i::Zero(),,"Cumulative number of inversions in insertion sort; always zero in the non-debug builds"))
		, /*deprec*/
		, /* init */
		,
		/* ctor */
			#ifdef ISC_TIMING
				timingDeltas=shared_ptr<TimingDeltas>(new TimingDeltas);
			#endif 
			#ifdef PISC_DEBUG
				watch1=watch2=-1; // disable watching
			#endif
			for(int i=0; i<3; i++) BB[i].axis=i;
			periodic=false;
			strideActive=false;
			,
		/* py */
		.def_readonly("strideActive",&InsertionSortCollider::strideActive,"Whether striding is active (read-only; for debugging). |yupdate|")
		.def_readonly("periodic",&InsertionSortCollider::periodic,"Whether the collider is in periodic mode (read-only; for debugging) |yupdate|")
		.def("dumpBounds",&InsertionSortCollider::dumpBounds,"Return representation of the internal sort data. The format is ``([...],[...],[...])`` for 3 axes, where each ``...`` is a list of entries (bounds). The entry is a tuple with the fllowing items:\n\n* coordinate (float)\n* body id (int), but negated for negative bounds\n* period numer (int), if the collider is in the periodic regime.")
		.def("dbgInfo",&InsertionSortCollider::dbgInfo,"Return python distionary with information on some internal structures (debugging only)")
		#ifdef PISC_DEBUG
			.def_readwrite("watch1",&InsertionSortCollider::watch1,"debugging only: watched body Id.")
			.def_readwrite("watch2",&InsertionSortCollider::watch2,"debugging only: watched body Id.")
		#endif
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(InsertionSortCollider);
