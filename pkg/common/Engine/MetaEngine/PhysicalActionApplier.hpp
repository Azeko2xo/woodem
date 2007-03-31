/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef PHYSICALACTIONAPPLIER_HPP
#define PHYSICALACTIONAPPLIER_HPP


#include<yade/core/MetaDispatchingEngine2D.hpp>
#include<yade/lib-multimethods/DynLibDispatcher.hpp>
#include<yade/core/PhysicalAction.hpp>

#include "PhysicalActionApplierUnit.hpp"

class Body;

class PhysicalActionApplier :	public MetaDispatchingEngine2D
				<
					PhysicalAction ,					// base classe for dispatch
					PhysicalParameters,					// base classe for dispatch
					PhysicalActionApplierUnit,				// class that provides multivirtual call
					void,							// return type
					TYPELIST_3(	  const shared_ptr<PhysicalAction>&	// function arguments
							, const shared_ptr<PhysicalParameters>& 
							, const Body *
				    		  )
				>
{
	public :
		virtual void action(Body* body);

	REGISTER_CLASS_NAME(PhysicalActionApplier);
	REGISTER_BASE_CLASS_NAME(MetaDispatchingEngine2D);
};

REGISTER_SERIALIZABLE(PhysicalActionApplier,false);

#endif // PHYSICALACTIONAPPLIER_HPP

