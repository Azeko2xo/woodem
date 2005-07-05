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

#include "Sphere2Sphere4ErrorTolerant.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-common/Sphere.hpp>
#include <yade/yade-common/ErrorTolerantContactModel.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool Sphere2Sphere4ErrorTolerant::go(		const shared_ptr<InteractingGeometry>& cm1,
						const shared_ptr<InteractingGeometry>& cm2,
						const Se3r& se31,
						const Se3r& se32,
						const shared_ptr<Interaction>& c)
{
	shared_ptr<Sphere> s1 = dynamic_pointer_cast<Sphere>(cm1);
	shared_ptr<Sphere> s2 = dynamic_pointer_cast<Sphere>(cm2);

	Vector3r normal = se32.position-se31.position;
	float penetrationDepth = s1->radius+s2->radius-normal.normalize();

	if (penetrationDepth>0)
	{
		shared_ptr<ErrorTolerantContactModel> cm = shared_ptr<ErrorTolerantContactModel>(new ErrorTolerantContactModel());

		Vector3r pt1 = se31.position+normal*s1->radius;
		Vector3r pt2 = se32.position-normal*s2->radius;
		cm->closestPoints.push_back(std::pair<Vector3r,Vector3r>(pt1,pt2));
		cm->o1p1 = pt1-se31.position;
		cm->o2p2 = pt2-se32.position;
		cm->normal = normal;
		c->interactionGeometry = cm;
		return true;
	}
	else	
		return false;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool Sphere2Sphere4ErrorTolerant::goReverse(	const shared_ptr<InteractingGeometry>& cm1,
						const shared_ptr<InteractingGeometry>& cm2,
						const Se3r& se31,
						const Se3r& se32,
						const shared_ptr<Interaction>& c)
{
	return go(cm1,cm2,se31,se32,c);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

