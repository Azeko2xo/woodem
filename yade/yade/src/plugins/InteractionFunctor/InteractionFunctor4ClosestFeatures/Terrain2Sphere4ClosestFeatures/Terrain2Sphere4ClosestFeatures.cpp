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

#include "Terrain2Sphere4ClosestFeatures.hpp"
#include "Sphere.hpp"
#include "Terrain.hpp"
#include "ClosestFeatures.hpp"
#include "Distances3D.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool Terrain2Sphere4ClosestFeatures::go(	const shared_ptr<CollisionGeometry>& cm1,
						const shared_ptr<CollisionGeometry>& cm2,
						const Se3r& se31,
						const Se3r& se32,
						shared_ptr<Interaction>& c)
{
	Vector3r pt;
	std::vector<Vector3r> tri;
	bool inCollision =  false;
	std::vector<int> faces;
	
	shared_ptr<Terrain> t = dynamic_pointer_cast<Terrain>(cm1);
	shared_ptr<Sphere> s  = dynamic_pointer_cast<Sphere>(cm2);
	
	Vector3r min,max;
	Vector3r radius = Vector3r(s->radius,s->radius,s->radius);
	min = se32.translation-radius;
	max = se32.translation+radius;
	
	t->getFaces(AABB((max-min)*0.5,(min+max)*0.5),faces);
	tri.resize(3);
	
	shared_ptr<ClosestFeatures> cf = shared_ptr<ClosestFeatures>(new ClosestFeatures());
	
	for(unsigned int i=0;i<faces.size();i++)
	{
		int faceId = faces[i];
		tri[0] = t->vertices[t->faces[faceId][0]]+se31.translation;
		tri[1] = t->vertices[t->faces[faceId][1]]+se31.translation;
		tri[2] = t->vertices[t->faces[faceId][2]]+se31.translation;
		Real d = sqrDistTriPoint(se32.translation, tri, pt);
				
		if (d<s->radius*s->radius)
		{
			Vector3r v = pt-se32.translation;
			v.normalize();
			cf->closestsPoints.push_back(std::pair<Vector3r,Vector3r>(pt,se32.translation+v*s->radius));
			inCollision =  true;
		}
	}
		
	if (inCollision)
		c->interactionGeometry = cf;

	return inCollision;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool Terrain2Sphere4ClosestFeatures::goReverse(	const shared_ptr<CollisionGeometry>& cm1,
						const shared_ptr<CollisionGeometry>& cm2,
						const Se3r& se31,
						const Se3r& se32,
						shared_ptr<Interaction>& c)
{
	bool isColliding = go(cm2,cm1,se32,se31,c);
	if (isColliding)
	{
		shared_ptr<ClosestFeatures> cf = dynamic_pointer_cast<ClosestFeatures>(c->interactionGeometry);
		Vector3r tmp = cf->closestsPoints[0].first;
		cf->closestsPoints[0].first = cf->closestsPoints[0].second;		
		cf->closestsPoints[0].second = tmp;
	}
	return isColliding;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
