/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/Body.hpp>
#include<yade/core/State.hpp>
#include<yade/core/Functor.hpp>

class PhysicalActionApplierUnit: public Functor1D<void,TYPELIST_3(const shared_ptr<State>&,const Body*, World*)>{
	public: virtual ~PhysicalActionApplierUnit();
	REGISTER_CLASS_AND_BASE(PhysicalActionApplierUnit,Functor1D);
	REGISTER_ATTRIBUTES(Functor, /* nothing here */ );
};
REGISTER_SERIALIZABLE(PhysicalActionApplierUnit);

