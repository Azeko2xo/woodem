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
 
#ifndef __BOUNDINGVOLUMEUPDATOR_HPP__
#define __BOUNDINGVOLUMEUPDATOR_HPP__

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Actor.hpp"
#include "DynLibDispatcher.hpp"
#include "InteractionDescription.hpp"
#include "BoundingVolume.hpp"
#include "BoundingVolumeFunctor.hpp"
#include "Body.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class BoundingVolumeDispatcher : public Actor
{
	private : DynLibDispatcher
		<	TYPELIST_2( InteractionDescription , BoundingVolume ) ,		// base classess for dispatch
			BoundingVolumeFunctor,						// class that provides multivirtual call
			void ,								// return type
			TYPELIST_4(
					  const shared_ptr<InteractionDescription>&	// arguments
					, const shared_ptr<BoundingVolume>&
					, const Se3r&
					, const Body*
				)
		> boundingVolumeDispatcher;

	private : vector<vector<string> > boundingVolumeFunctors;
	public  : void addBoundingVolumeFunctors(const string& str1,const string& str2,const string& str3);

	public : virtual void action(Body* b);
	
	public : virtual void postProcessAttributes(bool deserializing);
	public : virtual void registerAttributes();
	REGISTER_CLASS_NAME(BoundingVolumeDispatcher);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_SERIALIZABLE(BoundingVolumeDispatcher,false);

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __BOUNDINGVOLUMEUPDATOR_HPP__

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
