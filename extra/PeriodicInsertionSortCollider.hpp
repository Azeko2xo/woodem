// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#pragma once
#include<yade/core/Collider.hpp>
class InteractionContainer;

/*! Collider with periodic boundary conditions.

This colider is based on the code of InsertionSortCollider and it should be,
at some point, integrated back into its code (given that the performance impact
is negligible, which I think might be true).

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
relative to the 1st one. Dispatchers (InteractionGeometryMetaEngine and InteractionDispatchers)
use this information to pass modified position of the 2nd body to InteractionGeometryEngineUnits.
Since properly behaving InteractionGeometryEngineUnit's and ConstitutiveLaw's do not take positions
directly from Body::state, the interaction is computed with the periodic positions.

Positions of bodies (in the sense of Body::state) and their natural bboxes are not wrapped
to the periodic cell, they can be anywhere (but not "too far" in the sense of int overflow).

Since Interaction::cellDists holds cell coordinates, it is possible to change the cell boundaries
at runtime. This should make it easy to implement some stress control on the top.

Clumps do not interfere with periodicity in any way.

Rendering
---------
OpenGLRenderingEngine renders GeometricalModel at all periodic positions that touch the
periodic cell (i.e. BoundingVolume crosses its boundary).

It seems to affect body selection somehow, but that is perhaps not related at all.

Periodicity control
===================
c++:
	MetaBody::isPeriodic, MetaBody::cellMin, MetaBody::cellMax
python:
	O.periodicCell=((0,0,0),(10,10,10)  # activates periodic boundary
	O.periodicCell=() # deactivates periodic boundary

Requirements
============
* No body can have AABB larger than about .499*cellSize. Exception is thrown if that is false.
* Constitutive law must not get body positions from Body::state directly.
	If it does, it uses Interaction::cellDist to compute periodic position.
	Dem3Dof family of Ig2 functors and Law2_* engines are known to behave well.
* No body can get further away than MAXINT periods. It will do horrible things if there is overflow. Not checked at the moment.
* Body cannot move over distance > cellSize in one step. Since body size is limited as well, that would mean simulation explosion.
	Exception is thrown if the movement is upwards. If downwards, it is not handled at all.

Possible performance improvements & bugs
========================================

* PeriodicInsertionSortCollider::{cellWrap,cellWrapRel} OpenGLRenderingEngine::{wrapCell,wrapCellPt} Shop::PeriodicWrap
	are all very similar functions. They should be put into separate header and included from all those places.

* Until this code is integrated with plain InsertionSortCollider, it will not support striding via VelocityBins
	Those 2 features are orthogonal, the integration shouldn't be diffucult.

*/

// #define to turn on some tracing information
// all code under this can be probably removed at some point, when the collider will have been tested thoroughly
// #define PISC_DEBUG

class PeriodicInsertionSortCollider: public Collider{
	//! struct for storing bounds of bodies
	struct Bound{
		//! coordinate along the given sortAxis
		Real coord;
		//! id of the body this bound belongs to
		body_id_t id;
		//! periodic cell coordinate
		int period;
		//! is it the minimum (true) or maximum (false) bound?
		struct{ unsigned hasBB:1; unsigned isMin:1; } flags;
		Bound(Real coord_, body_id_t id_, bool isMin): coord(coord_), id(id_), period(0){ flags.isMin=isMin; }
		bool operator<(const Bound& b) const {
			/* handle special case of zero-width bodies, which could otherwise get min/max swapped in the unstable std::sort */
			if(id==b.id && coord==b.coord) return flags.isMin;
			return coord<b.coord;
		}
		bool operator>(const Bound& b) const {
			if(id==b.id && coord==b.coord) return b.flags.isMin;
			return coord>b.coord;
		}
	};
	struct VecBounds{
		std::vector<Bound> vec;
		Real cellMin, cellMax, cellDim;
		long size;
		// index of the lowest coordinate element, before which the container wraps
		long loIdx;
		Bound& operator[](long idx){ assert(idx<size && idx>=0); return vec[idx]; }
		const Bound& operator[](long idx) const { assert(idx<size && idx>=0); return vec[idx]; }
		// update periodic properties and size from MetaBody
		void update(MetaBody* rb, int axis);
		// normalize given index to the right range (wraps around)
		long norm(long i) const { if(i<0) i+=size; long ret=i%size; assert(ret>=0 && ret<size); return ret;}
		VecBounds(): size(0), loIdx(0){}
		void dump(ostream& os){ string ret; for(size_t i=0; i<vec.size(); i++) os<<((long)i==loIdx?"@@ ":"")<<vec[i].coord<<"(id="<<vec[i].id<<","<<(vec[i].flags.isMin?"min":"max")<<",p"<<vec[i].period<<") "; os<<endl;}
	};
	private:
	//! storage for bounds
	VecBounds XX,YY,ZZ;
	//! storage for bb maxima and minima
	std::vector<Real> maxima, minima;

	void insertionSort(VecBounds& v,InteractionContainer*,MetaBody*,bool doCollide=true);
	void handleBoundInversion(body_id_t,body_id_t,InteractionContainer*,MetaBody*);
	bool spatialOverlap(body_id_t,body_id_t, MetaBody*, Vector3<int>&) const;
	static Real cellWrap(const Real, const Real, const Real, int&);
	static Real cellWrapRel(const Real, const Real, const Real);
	#ifdef PISC_DEBUG
		bool watchIds(body_id_t id1, body_id_t id2) const { body_id_t i1=2,i2=14; return ((id1==i1)&&(id2==i2))||((id1==i2)&&(id2==i1)); }
	#endif


	public:
	//! axis for the initial sort
	int sortAxis;
	//! Predicate called from loop within InteractionContainer::erasePending
	bool shouldBeErased(body_id_t id1, body_id_t id2, MetaBody* rb) const { Vector3<int> foo;
		#ifdef PISC_DEBUG
			if(watchIds(id1,id2)) LOG_INFO("Requesting erase of #"<<id1<<"+#"<<id2<<", result: "<<!spatialOverlap(id1,id2,rb,foo));
		#endif
		return !spatialOverlap(id1,id2,rb,foo); }

	PeriodicInsertionSortCollider(): sortAxis(0) { }
	virtual void action(MetaBody*);
	REGISTER_CLASS_AND_BASE(PeriodicInsertionSortCollider,Collider);
	REGISTER_ATTRIBUTES(Collider,(sortAxis));
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(PeriodicInsertionSortCollider);

/* Simple engine for compressing random cloud of spheres until
it reaches given average stress and then goes back to given residual stress
*/
class PeriIsoCompressor: public StandAloneEngine{
	Real avgStiffness; Real maxDisplPerStep; Vector3r sumForces, sigma; 
	public:
	//! Stresses that should be reached, one after another
	vector<Real> stresses;
	//! Characteristic length, should be something like mean particle diameter (default -1=invalid value))
	Real charLen;
	//! Maximum body span in terms of bbox, to prevent periodic cell getting too small; is computed automatically at the beginning
	Real maxSpan;
	//! if actual unbalanced force is smaller than this number, the packing is considered stable (default 1e-4)
	Real maxUnbalanced, currUnbalanced;
	//! how often to recompute average stress, stiffness and unbalanced force (default 100)
	int globalUpdateInt;
	//! Where are we at in the process
	size_t state;
	//! python command to be run when reaching the last specified stress
	string doneHook;
	//! whether to exactly keep proportions of the cell
	bool keepProportions;
	void action(MetaBody*);
	PeriIsoCompressor(): avgStiffness(-1), maxDisplPerStep(-1), sumForces(Vector3r::ZERO), sigma(Vector3r::ZERO), charLen(-1), maxSpan(-1), maxUnbalanced(1e-4), currUnbalanced(-1), globalUpdateInt(20), state(0), keepProportions(true){}
	REGISTER_ATTRIBUTES(StandAloneEngine,(stresses)(charLen)(maxUnbalanced)(globalUpdateInt)(state)(doneHook)(maxSpan)(keepProportions));
	DECLARE_LOGGER;
	REGISTER_CLASS_AND_BASE(PeriIsoCompressor,StandAloneEngine);
};
REGISTER_SERIALIZABLE(PeriIsoCompressor);
