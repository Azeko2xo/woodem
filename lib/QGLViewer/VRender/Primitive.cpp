/*
 This file is part of the VRender library.
 Copyright (C) 2005 Cyril Soler (Cyril.Soler@imag.fr)
 Version 1.0.0, released on June 27, 2005.

 http://artis.imag.fr/Members/Cyril.Soler/VRender

 VRender is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 VRender is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with VRender; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

/****************************************************************************

 Copyright (C) 2002-2007 Gilles Debunne (Gilles.Debunne@imag.fr)

 This file is part of the QGLViewer library.
 Version 2.3.0, released on June 29, 2008.

 http://artis.imag.fr/Members/Gilles.Debunne/QGLViewer

 libQGLViewer is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 libQGLViewer is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libQGLViewer; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*****************************************************************************/

#include <math.h>
#include <assert.h>
#include "Primitive.h"
#include "Types.h"

using namespace vrender ;
using namespace std ;

Point::Point(const Feedback3DColor& f)
	: _position_and_color(f)
{
}

const Vector3& Point::vertex(int) const
{
	return _position_and_color.pos() ;
}

const Feedback3DColor& Point::sommet3DColor(int) const
{
	return _position_and_color ;
}

const Feedback3DColor& Segment::sommet3DColor(int i) const
{
	return ( (i&1)==0 )?P1:P2 ;
}

AxisAlignedBox_xyz Point::bbox() const
{
	return AxisAlignedBox_xyz(_position_and_color.pos(),_position_and_color.pos()) ;
}

const Vector3& Segment::vertex(int i) const
{
	return ( (i&1)==0 )?P1.pos():P2.pos() ;
}

AxisAlignedBox_xyz Segment::bbox() const
{
	AxisAlignedBox_xyz B(P1.pos());
	B.include(P2.pos()) ;

	return B ;
}

const Feedback3DColor& Polygone::sommet3DColor(int i) const
{
	return _vertices[i % nbVertices()] ;
}

const Vector3& Polygone::vertex(int i) const
{
	return _vertices[i % nbVertices()].pos() ;
}


Polygone::Polygone(const vector<Feedback3DColor>& fc)
	: _vertices(fc)
{
	initNormal() ;

	for(unsigned int i=0;i<fc.size();i++)
		_bbox.include(fc[i].pos()) ;
}

AxisAlignedBox_xyz Polygone::bbox() const
{
	return _bbox ;
}

double Polygone::equation(const Vector3& v) const
{
	return v * _normal - _c ;
}

void Polygone::initNormal()
{
	FLOAT anglemax = 0.0 ;
	Vector3 normalmax = Vector3(0.0,0.0,0.0) ;
	FLOAT v12norm = (vertex(1)-vertex(0)).norm() ;

	for(int i=0;i<nbVertices();i++)
	{
		Vector3 v1(vertex(i)) ;
		Vector3 v2(vertex(i+1));
		Vector3 v3(vertex(i+2)) ;

		Vector3 normal_tmp((v3-v2)^(v1-v2)) ;

		FLOAT v32norm = (v3-v2).norm() ;

		if(normal_tmp.z() > 0)
			normal_tmp *= -1.0 ;

		if((v32norm > 0.0)&&(v12norm > 0.0))
		{
			double anglemaxtmp = normal_tmp.norm()/v32norm/v12norm ;

			if(anglemaxtmp > anglemax)
			{
				anglemax = anglemaxtmp ;
				normalmax = normal_tmp ;
			}
		}

		v12norm = v32norm ;

		if(anglemax > FLAT_POLYGON_EPS)	// slight optimization
			break ;
	}

	if(normalmax.infNorm() != 0.0)
		_normal = NVector3(normalmax) ;

	anglefactor = anglemax ;
	_c = _normal*vertex(0) ;
}

std::ostream& vrender::operator<<(std::ostream& o,const Feedback3DColor& f)
{
	o << "(" << f.pos() << ") + (" << f.red() << "," << f.green() << "," << f.blue() << "," << f.alpha() << ")" << endl ;
	return o ;
}


