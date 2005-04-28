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
 
#include "Box2AABB.hpp"
#include "InteractingBox.hpp"
#include "AABB.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void Box2AABB::go(	const shared_ptr<InteractingGeometry>& cm,
				shared_ptr<BoundingVolume>& bv,
				const Se3r& se3,
				const Body*	)
{
	InteractingBox* box = static_cast<InteractingBox*>(cm.get());
	AABB* aabb = static_cast<AABB*>(bv.get());
	
	aabb->center = se3.position;

	Matrix3r r;
	se3.orientation.toRotationMatrix(r);
	aabb->halfSize = Vector3r(0,0,0);
	for( int i=0; i<3; ++i )
		for( int j=0; j<3; ++j )
			aabb->halfSize[i] += fabs( r[i][j] * box->extents[j] );
	
	aabb->min = aabb->center-aabb->halfSize;
	aabb->max = aabb->center+aabb->halfSize;
}
	
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
