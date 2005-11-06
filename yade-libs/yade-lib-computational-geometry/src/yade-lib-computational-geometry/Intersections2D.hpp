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

#ifndef __INTERSECTION2D_H__
#define __INTERSECTION2D_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-lib-wm3-math/Vector3.hpp>
#include <yade/yade-lib-wm3-math/Vector2.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <vector>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	bool segments2DIntersect(Vector2r& p1,Vector2r& p2,Vector2r& p3,Vector2r& p4);
	bool lines2DIntersection(Vector2r p1, Vector2r d1, Vector2r p2,Vector2r d2, bool& same, Vector2r& iPoint);
	bool segments2DIntersection(Vector2r p1, Vector2r d1, Vector2r p2,Vector2r d2, bool& same, Vector2r& iPoint);

	bool pointOnSegment2D(Vector2r& s1, Vector2r& s2, Vector2r& p, Real& c);
	
	int clipPolygon(Vector3r quad,const std::vector<Vector3r>& polygon, std::vector<Vector3r>& clipped);
	void clipLeft(Real sizeX, std::vector<Vector3r> &polygon, Vector3r v1, Vector3r v2);
	void clipRight(Real sizeX, std::vector<Vector3r>& polygon, Vector3r v1, Vector3r v2);
	void clipTop(Real sizeY, std::vector<Vector3r>& polygon, Vector3r v1, Vector3r v2);
	void clipBottom(Real sizeY, std::vector<Vector3r> &polygon, Vector3r v1, Vector3r v2);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __INTERSECTION2D_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
