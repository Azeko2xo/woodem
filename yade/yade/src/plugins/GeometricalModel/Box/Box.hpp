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

#ifndef __BOX_H__
#define __BOX_H__

#include "CollisionGeometry.hpp"

class Box : public CollisionGeometry
{
	public : Vector3r extents;

	// construction
	public : Box (Vector3r e);
	public : Box ();
	public : ~Box ();

	public : bool collideWith(CollisionGeometry* collisionGeometry);
	public : bool loadFromFile(char * fileName);
	public : void glDraw();

	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();

	REGISTER_CLASS_NAME(Box);
	REGISTER_CLASS_INDEX(Box);

};

#include "ArchiveTypes.hpp"
using namespace ArchiveTypes;

REGISTER_SERIALIZABLE(Box,false);
#endif // __BOX_H__
