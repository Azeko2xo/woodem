/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef __PERSISTENTSAPCOLLIDER_HPP__
#define __PERSISTENTSAPCOLLIDER_HPP__

#include<yade/core/BroadInteractor.hpp>
#include<yade/core/InteractionContainer.hpp>
#include <list>
#include <set>
#include <vector>
#include <algorithm>

using namespace std;

class PersistentSAPCollider : public BroadInteractor
{
	private :
		// represent an extrmity of an Axis ALigned bounding box
		struct AABBBound
		{
			AABBBound(int i, char l) : id(i),lower(l) {};
			int		id;		// AABB of the "id" shpere
			char		lower;		// is it the lower or upper bound of the AABB
			Real		value;		// value of the bound
		};
		// strucuture that compare 2 AABBBounds => used in the sort algorithm
		struct AABBBoundComparator
		{
			bool operator() (shared_ptr<AABBBound> b1, shared_ptr<AABBBound> b2)
			{
				return b1->value<b2->value;
			}
		};

		MetaBody* rootBody; // refreshed at every iteration; to avoid passing it to called members on stack

	protected :
		/// number of potential transientInteractions = number of interacting AABB
		int nbPotentialInteractions;

		/// number of AABB
		unsigned int nbObjects;

		/// AABB extremity of the sphere number "id" projected onto the X axis
		vector<shared_ptr<AABBBound> > xBounds;

		/// AABB extremity of the sphere number "id" projected onto the Y axis
		vector<shared_ptr<AABBBound> > yBounds;

		/// AABB extremity of the sphere number "id" projected onto the Z axis
		vector<shared_ptr<AABBBound> > zBounds;

		// collection of AABB that are in interaction
		//protected : vector< set<unsigned int> > overlappingBB;
		shared_ptr<InteractionContainer> transientInteractions;
		shared_ptr<InteractionContainer> persistentInteractions;
		/// upper right corner of the AABB of the objects =>  for spheres = center[i]-radius
		vector<Real> maxima;

		/// lower left corner of the AABB of the objects =>  for spheres = center[i]+radius
		vector<Real> minima;

		/// Used the first time broadInteractionTest is called, to initialize and sort the xBounds, yBounds,
		/// and zBounds arrays and to initialize the overlappingBB collection
		void updateIds(unsigned int nbElements);

		/// Permutation sort the xBounds, yBounds, zBounds arrays according to the "value" field
		/// Calls updateOverlapingBBSet every time a permutation between 2 AABB i and j occurs
		void sortBounds(vector<shared_ptr<AABBBound> >& bounds, int nbElements);

		/// Tests if the AABBi and AABBj really overlap.
		/// If yes, adds the pair (id1,id2) to the collection of overlapping AABBs
		/// If no, removes the (id1,id2) to the collection of overlapping AABBs if necessary
		void updateOverlapingBBSet(int id1,int id2);

		/// update the "value" field of the xBounds, yBounds, zBounds arrays
		void updateBounds(int nbElements);

		/// Used the first time broadInteractionTest is called
		/// It is necessary to initialise the overlapping AABB collection because this collection is only
		/// incrementally udated each time step
		void findOverlappingBB(vector<shared_ptr<AABBBound> >& bounds, int nbElements);

	public :
		PersistentSAPCollider();

		virtual ~PersistentSAPCollider();

		/// return a list "transientInteractions" of pairs of Body which Bounding volume are in potential interaction
		void action(MetaBody *);

		//! When creating transient interaction, look first if a persistent link between the pair in question exists; in that case, skip it.
		bool noTransientIfPersistentExists;
		//! Don't break transient interaction once bodies don't overlap anymore; material law will be responsible for breaking it.
		bool haveDistantTransient;

		void registerAttributes(){
			BroadInteractor::registerAttributes();
			REGISTER_ATTRIBUTE(noTransientIfPersistentExists);
			REGISTER_ATTRIBUTE(haveDistantTransient);
		}


	REGISTER_CLASS_NAME(PersistentSAPCollider);
	REGISTER_BASE_CLASS_NAME(BroadInteractor);

};

REGISTER_SERIALIZABLE(PersistentSAPCollider,false);

#endif // __PERSISTENTSAPCOLLIDER_HPP__

