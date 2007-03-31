/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@mail.berlios.de                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef ELASTICBODYSIMPLERELATIONSHIP_HPP
#define ELASTICBODYSIMPLERELATIONSHIP_HPP

#include "InteractionPhysicsEngineUnit.hpp"

class ElasticBodySimpleRelationship : public InteractionPhysicsEngineUnit
{
	public :
		virtual void go(	const shared_ptr<PhysicalParameters>& b1,
					const shared_ptr<PhysicalParameters>& b2,
					const shared_ptr<Interaction>& interaction);

	protected :
		virtual void registerAttributes();

	REGISTER_CLASS_NAME(ElasticBodySimpleRelationship);
	REGISTER_BASE_CLASS_NAME(InteractionPhysicsEngineUnit);

};

REGISTER_SERIALIZABLE(ElasticBodySimpleRelationship,false);

#endif // 

