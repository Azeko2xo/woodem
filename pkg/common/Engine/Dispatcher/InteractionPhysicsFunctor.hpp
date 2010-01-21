/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/Material.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/core/Functor.hpp>
#include <string>

/*! \brief
	Abstract interface for all classes that build InteractionPhysics from two interacting Body's PhysicalParameters

	When an interaction is more complicated than its geometry, a class InteractionPhysics can be build to describe
	the stiffness or other parmeters of this interaction.
	 
	Currently it is only used for MacroMicroElasticRelationships

	\param const shared_ptr<PhysicalParameters>&	first interacting Body
	\param const shared_ptr<PhysicalParameters>&	second interacting Body
	\return shared_ptr<Interaction>&		it returns the InteractionPhysic's part of Interaction (given as last argument to the function)
	
*/

class InteractionPhysicsFunctor : 	public Functor2D
					<
		 				void ,
		 				TYPELIST_3(	  const shared_ptr<Material>&
								, const shared_ptr<Material>&
								, const shared_ptr<Interaction>&
			   				  ) 
					>
{
	public: virtual ~InteractionPhysicsFunctor();
	//REGISTER_CLASS_AND_BASE(InteractionPhysicsFunctor,Functor2D);
	//REGISTER_ATTRIBUTES(Functor, /* no attributes here */ );
	YADE_CLASS_BASE_ATTRS(InteractionPhysicsFunctor,Functor,/*no attrs*/);

};
REGISTER_SERIALIZABLE(InteractionPhysicsFunctor);


