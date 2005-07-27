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

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ACTION_PARAMETER_INITIALIZER_HPP
#define ACTION_PARAMETER_INITIALIZER_HPP 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-core/Engine.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class Body;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class PhysicalActionContainerInitializer : public Engine
{
	public    : std::vector<std::string> actionParameterNames;
	
	public    : PhysicalActionContainerInitializer();
	public    : virtual ~PhysicalActionContainerInitializer();
	public    : virtual void action(Body* body);
	
	protected : virtual void registerAttributes();
	REGISTER_CLASS_NAME(PhysicalActionContainerInitializer);
	REGISTER_BASE_CLASS_NAME(Engine);
};

REGISTER_SERIALIZABLE(PhysicalActionContainerInitializer,false);

#endif // ACTION_PARAMETER_INITIALIZER_HPP

