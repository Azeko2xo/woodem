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

#include "PhysicalActionApplier.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-core/MetaBody.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// void PhysicalActionApplier::postProcessAttributes(bool deserializing)
// {
// 	postProcessDispatcher2D(deserializing);
// }
// 
// ///////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////
// 
// void PhysicalActionApplier::registerAttributes()
// {
// 	REGISTER_ATTRIBUTE(functorNames);
// 	REGISTER_ATTRIBUTE(functorArguments);
// }

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalActionApplier::action(Body* body)
{
	MetaBody * ncb = dynamic_cast<MetaBody*>(body);
	shared_ptr<BodyContainer>& bodies = ncb->bodies;

	PhysicalActionContainer::iterator pai    = ncb->actionParameters->begin();
	PhysicalActionContainer::iterator paiEnd = ncb->actionParameters->end(); 
	for( ; pai!=paiEnd ; ++pai)
	{
		shared_ptr<PhysicalAction> action = *pai;
		int id = pai.getCurrentIndex();
		// FIXME - solve the problem of Body's id
		operator()( action , (*bodies)[id]->physicalParameters , (*bodies)[id].get() );
	}
}
