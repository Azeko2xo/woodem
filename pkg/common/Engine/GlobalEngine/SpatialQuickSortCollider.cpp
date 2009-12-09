/*************************************************************************
*  Copyright (C) 2008 by Sergei Dorofeenko				 *
*  sega@users.berlios.de                                                 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#include "SpatialQuickSortCollider.hpp"
#include <yade/core/Scene.hpp>
#include <yade/core/BodyContainer.hpp>
#include <math.h>
#include <algorithm>

YADE_PLUGIN((SpatialQuickSortCollider));

SpatialQuickSortCollider::SpatialQuickSortCollider() : Collider()
{
}

SpatialQuickSortCollider::~SpatialQuickSortCollider()
{

}

void SpatialQuickSortCollider::action(Scene* ncb)
{
	if(ncb->isPeriodic){ throw runtime_error("SpatialQuickSortCollider doesn't handle periodic boundaries."); }
	const shared_ptr<BodyContainer>& bodies = ncb->bodies;

	// This collider traverses all interactions at every step, therefore all interactions
	// that were requested for erase might be erased here and will be recreated if necessary.
	ncb->interactions->unconditionalErasePending();

	size_t nbElements=bodies->size();
	if (nbElements!=rank.size())
	{
		size_t n = rank.size();
		rank.resize(nbElements);
		for (; n<nbElements; ++n)
			rank[n] = shared_ptr<AABBBound>(new AABBBound);
	}

	Vector3r min,max;
	shared_ptr<Body> b;
	int i=0;
	FOREACH(const shared_ptr<Body>& b, *bodies){
		if(!b || !b->bound) continue;
	   
	   min = b->bound->min;
	   max = b->bound->max;

	   rank[i]->id = b->getId();
	   rank[i]->min = min;
	   rank[i]->max = max;

		i++;
	}
	
	const shared_ptr<InteractionContainer>& interactions=ncb->interactions;
	ncb->interactions->iterColliderLastRun=ncb->currentIteration;

	sort(rank.begin(), rank.end(), xBoundComparator()); // sotring along X

	int id,id2; size_t j;
	shared_ptr<Interaction> interaction;
	for(int i=0,e=nbElements-1; i<e; ++i)
	{
	    id  = rank[i]->id;
	    min = rank[i]->min; 
	    max = rank[i]->max; 
	    j=i;
	    while(++j<nbElements)
	    {
			if ( rank[j]->min[0] > max[0]) break;
			if ( rank[j]->min[1] < max[1]
			&& rank[j]->max[1] > min[1]
			&& rank[j]->min[2] < max[2]
			&& rank[j]->max[2] > min[2])
			{
				id2=rank[j]->id;
				if ( (interaction = interactions->find(body_id_t(id),body_id_t(id2))) == 0)
				{
					interaction = shared_ptr<Interaction>(new Interaction(id,id2) );
					interactions->insert(interaction);
				}
				interaction->iterLastSeen=ncb->currentIteration; 
			}
	    }
	}
}

