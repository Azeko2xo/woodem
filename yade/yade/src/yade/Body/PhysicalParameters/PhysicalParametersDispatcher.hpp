/***************************************************************************
 *   Copyright (C) 2005 by Janek Kozicki                                   *
 *   cosurgi@berlios.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
 
#ifndef BODY_PHYSICAL_PARAMETERS_DISPATCHER_HPP
#define BODY_PHYSICAL_PARAMETERS_DISPATCHER_HPP 

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Engine.hpp"
#include "DynLibDispatcher.hpp"
#include "PhysicalParameters.hpp"
#include "PhysicalParametersFunctor.hpp"
#include "Body.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class PhysicalParametersMetaEngine :
	  public Engine 
	, public DynLibDispatcher
		<	PhysicalParameters ,
			PhysicalParametersFunctor,
			void ,
			TYPELIST_2(
					  const shared_ptr<PhysicalParameters>&
					, Body*
				  )
		>
{
	public		: virtual void action(Body* b);
	public		: virtual void registerAttributes();
	public		: virtual void postProcessAttributes(bool deserializing);
	REGISTER_CLASS_NAME(PhysicalParametersMetaEngine);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_SERIALIZABLE(PhysicalParametersMetaEngine,false);

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // BODY_PHYSICAL_PARAMETERS_DISPATCHER_HPP 

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
