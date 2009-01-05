/*************************************************************************
*  Copyright (C) 2007 by Bruno CHAREYRE                                 *
*  bruno.chareyre@hmg.inpg.fr                                        *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef SIMPLECONTACTMODELWATER_HPP
#define SIMPLECONTACTMODELWATER_HPP

#include<yade/pkg-common/InteractionPhysicsEngineUnit.hpp>

class SimpleElasticRelationshipsWater : public InteractionPhysicsEngineUnit
{
	public :
		SimpleElasticRelationshipsWater();

		virtual void go(	const shared_ptr<PhysicalParameters>& b1,
					const shared_ptr<PhysicalParameters>& b2,
					const shared_ptr<Interaction>& interaction);

	protected :
		virtual void registerAttributes();

	FUNCTOR2D(BodyMacroParameters,BodyMacroParameters);
	REGISTER_CLASS_NAME(SimpleElasticRelationshipsWater);
	REGISTER_BASE_CLASS_NAME(InteractionPhysicsEngineUnit);

};

REGISTER_SERIALIZABLE(SimpleElasticRelationshipsWater);

#endif // SIMPLECONTACTMODEL_HPP

