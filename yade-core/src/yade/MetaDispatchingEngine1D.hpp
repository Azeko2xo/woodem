/***************************************************************************
 *   Copyright (C) 2004 by Olivier Galizzi                                 *
 *   olivier.galizzi@imag.fr                                               *
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

#ifndef __METADISPATCHINGENGINE1D_HPP__
#define __METADISPATCHINGENGINE1D_HPP__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "MetaDispatchingEngine.hpp"

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-lib-multimethods/DynLibDispatcher.hpp>

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

template
<
	class baseClass,
	class EngineUnitType,
	class EngineUnitReturnType,
	class EngineUnitArguments,
	bool autoSymmetry=true
>
class MetaDispatchingEngine1D : public MetaDispatchingEngine,
				public DynLibDispatcher
				<	  TYPELIST_1(baseClass)		// base classes for dispatch
					, EngineUnitType		// class that provides multivirtual call
					, EngineUnitReturnType		// return type
					, EngineUnitArguments
					, autoSymmetry
				>
{

	public : virtual void add( string baseClassName1, string libName, shared_ptr<EngineUnit> eu = shared_ptr<EngineUnitType>())
	{
		storeFunctorName(baseClassName1,libName,static_pointer_cast<EngineUnitType>(eu));
		add1DEntry(baseClassName1,libName,static_pointer_cast<EngineUnitType>(eu));
	}

	void postProcessAttributes(bool deserializing)
	{
		MetaDispatchingEngine::postProcessAttributes(deserializing);
	
		if(deserializing)
		{
			for(unsigned int i=0;i<functorNames.size();i++)
				add1DEntry(functorNames[i][0],functorNames[i][1],static_pointer_cast<EngineUnitType>(findFunctorArguments(functorNames[i][1])));	
		}
	}
	
	void registerAttributes()
	{
		MetaDispatchingEngine::registerAttributes();
	}


	public    : int getDimension() { return 1; }
	
	public    : virtual string getEngineUnitType() 
	{
		shared_ptr<EngineUnitType> eu(new EngineUnitType);
		return eu->getClassName();
	}

	public    : virtual string getBaseClassType(unsigned int i)
	{
		if (i==0)
		{
			shared_ptr<baseClass> bc(new baseClass);
			return bc->getClassName();
		}
		else
			return "";
	}

	REGISTER_CLASS_NAME(MetaDispatchingEngine1D);
	REGISTER_BASE_CLASS_NAME(MetaDispatchingEngine DynLibDispatcher);


};

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __METADISPATCHINGENGINE1D_HPP__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
