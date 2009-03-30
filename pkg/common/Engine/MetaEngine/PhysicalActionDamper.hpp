/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/MetaEngine2D.hpp>
#include<yade/core/MetaEngine1D.hpp>
#include<yade/lib-multimethods/DynLibDispatcher.hpp>
#include<yade/pkg-common/PhysicalActionDamperUnit.hpp>

class Body;
class MetaBody;
class PhysicalActionDamper: public MetaEngine1D<PhysicalParameters,PhysicalActionDamperUnit,void,TYPELIST_3(const shared_ptr<PhysicalParameters>&, const Body*, MetaBody*)>{
	public: virtual void action(MetaBody*);
	REGISTER_CLASS_AND_BASE(PhysicalActionDamper,MetaEngine1D);
};
REGISTER_SERIALIZABLE(PhysicalActionDamper);
