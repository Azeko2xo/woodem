/*************************************************************************
*  Copyright (C) 2009 by Sergei Dorofeenko                               *
*  sega@users.berlios.de                                                 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/ConstitutiveLaw.hpp>

/// This class provides linear viscoelastic contact model
class ef2_Spheres_Viscoelastic_SimpleViscoelasticContactLaw : public ConstitutiveLaw
{
	public :
		virtual void go(shared_ptr<InteractionGeometry>&, shared_ptr<InteractionPhysics>&, Interaction*, MetaBody*);
		FUNCTOR2D(SpheresContactGeometry,ViscoelasticInteraction);
		REGISTER_CLASS_AND_BASE(ef2_Spheres_Viscoelastic_SimpleViscoelasticContactLaw,ConstitutiveLaw);
};
REGISTER_SERIALIZABLE(ef2_Spheres_Viscoelastic_SimpleViscoelasticContactLaw);


