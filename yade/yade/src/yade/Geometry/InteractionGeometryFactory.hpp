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

#ifndef __COLLISIONMODELFACTORY_H__
#define __COLLISIONMODELFACTORY_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "BodyGeometricalModel.hpp"
#include "BodyInteractionGeometry.hpp"
#include "Serializable.hpp"
#include "Se3.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

/*! \brief Abstract interface for all interaction model factories.
	It is used for creating a interaction model from a geometrical model during runtime.
	This is very useful when it is not trivial to build the interaction model from the geometrical model. For example if you want to build an sphere tree from a polyhedron, it is not easy to write by hand into the configuration file the center and size of all spheres. Instead you can use a InteractionGeometryFactory that will compute for you the correct values.
*/
class InteractionGeometryFactory : public Factorable
{	
	
	// construction
	public : InteractionGeometryFactory ();
	public : virtual ~InteractionGeometryFactory ();
	
	public : virtual shared_ptr<BodyInteractionGeometry> buildInteractionGeometry(const shared_ptr<BodyGeometricalModel> gm, const Se3r& se3) = 0;

};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __COLLISIONMODELFACTORY_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
