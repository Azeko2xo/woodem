/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@mail.berlios.de                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "MyTetrahedronLaw.hpp"
#include<yade/pkg-dem/InteractionOfMyTetrahedron.hpp>
#include<yade/pkg-common/ElasticBodyParameters.hpp>
#include<yade/pkg-common/SimpleElasticInteraction.hpp>
#include<yade/core/Omega.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/pkg-common/Force.hpp>
#include<yade/pkg-common/Momentum.hpp>
#include<yade/core/PhysicalAction.hpp>


MyTetrahedronLaw::MyTetrahedronLaw() : InteractionSolver() , actionForce(new Force) , actionMomentum(new Momentum)
{
}


void MyTetrahedronLaw::registerAttributes()
{
	InteractionSolver::registerAttributes();
	// ...
}


void MyTetrahedronLaw::action(Body* body)
{
	MetaBody * ncb = YADE_CAST<MetaBody*>(body);
	shared_ptr<BodyContainer>& bodies = ncb->bodies;

//	Real dt = Omega::instance().getTimeStep();

	InteractionContainer::iterator ii    = ncb->transientInteractions->begin();
	InteractionContainer::iterator iiEnd = ncb->transientInteractions->end();
	for(  ; ii!=iiEnd ; ++ii )
	{
		if ((*ii)->isReal)	// isReal means that InteractingMyTetrahedron2InteractingMyTetrahedron4InteractionOfMyTetrahedron returned true
					//                or InteractingMyTetrahedron2InteractingBox4InteractionOfMyTetrahedron           returned true
		{
			const shared_ptr<Interaction>& contact = *ii;
			int id1 = contact->getId1();
			int id2 = contact->getId2();
			
// you can use groups if you want for example a simulation that has different
// types of InteractionSolvers that talk with different bodies, for example
// if you want together MyTetrahedronLaw (solving tetrahedrons) and ElasticContactLaw (solving spheres)
//
// I already have a better solution for that, so you will not need to check
// groups inside this function, but they will be checked outside. I just need
// time to implement this :)
//
//			if( !( (*bodies)[id1]->getGroupMask() & (*bodies)[id2]->getGroupMask() & sdecGroupMask)  )
//				continue;
	
			ElasticBodyParameters* de1 				= YADE_CAST<ElasticBodyParameters*>((*bodies)[id1]->physicalParameters.get());
			ElasticBodyParameters* de2 				= YADE_CAST<ElasticBodyParameters*>((*bodies)[id2]->physicalParameters.get());
			InteractionOfMyTetrahedron* currentContactGeometry	= YADE_CAST<InteractionOfMyTetrahedron*>(contact->interactionGeometry.get());
			SimpleElasticInteraction* currentContactPhysics		= YADE_CAST<SimpleElasticInteraction*>(contact->interactionPhysics.get());

			for(int i=0 ; i<4 ; ++i )
				for(int j=0 ; j<4 ; ++j)
				{
					Real un					= currentContactGeometry->penetrationDepths[i][j];
					if(un > 0)
					{
						Vector3r force			= currentContactPhysics->kn*un*currentContactGeometry->normals[i][j];

						Vector3r x			= currentContactGeometry->contactPoints[i][j];
						Vector3r c1x			= (x - de1->se3.position);
						Vector3r c2x			= (x - de2->se3.position);

						// it will be some macro(	body->physicalActions,	ActionType , bodyId )
						static_cast<Force*>   ( ncb->physicalActions->find( id1 , actionForce   ->getClassIndex() ).get() )->force    -= force;
						static_cast<Force*>   ( ncb->physicalActions->find( id2 , actionForce   ->getClassIndex() ).get() )->force    += force;

						static_cast<Momentum*>( ncb->physicalActions->find( id1 , actionMomentum->getClassIndex() ).get() )->momentum -= c1x.Cross(force);
						static_cast<Momentum*>( ncb->physicalActions->find( id2 , actionMomentum->getClassIndex() ).get() )->momentum += c2x.Cross(force);
					}
				}

		}
	}
}


