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

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __TIMEINTEGRATORDISPATCHER_HPP__
#define __TIMEINTEGRATORDISPATCHER_HPP__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Actor.hpp"

#include "Body.hpp"
#include "DynLibDispatcher.hpp"
#include "TimeIntegratorFunctor.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class TimeIntegratorDispatcher : public Actor
{

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Attributes										      ///
///////////////////////////////////////////////////////////////////////////////////////////////////

	protected: DynLibDispatcher
		< 	BodyPhysicalParameters , 
		  	TimeIntegratorFunctor  ,
			void ,
		 	TYPELIST_2( const shared_ptr<BodyPhysicalParameters>&, unsigned int ) 
		> timeIntegratorDispatcher;
	
	private : vector<vector<string> > timeIntegratorFunctors;
	public  : void addTimeIntegratorFunctor(const string& str1,const string& str2);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Methods										      ///
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	public : virtual void action(Body* body);

	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();
	REGISTER_CLASS_NAME(TimeIntegratorDispatcher);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_SERIALIZABLE(TimeIntegratorDispatcher,false);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __TIMEINTEGRATORDISPATCHER_HPP__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
