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

#include "OpenGLWrapper.hpp"
#include "Box.hpp"
#include <iostream>

Box::Box (Vector3r e) : CollisionGeometry()
{
	extents = e;
}

Box::Box () : CollisionGeometry()
{	
}

Box::~Box ()
{

}

void Box::postProcessAttributes(bool deserializing)
{
	CollisionGeometry::postProcessAttributes(deserializing);
}

void Box::registerAttributes()
{
	CollisionGeometry::registerAttributes();
	REGISTER_ATTRIBUTE(extents);
}

bool Box::collideWith(CollisionGeometry* )
{
	return true;

}

bool Box::loadFromFile(char * )
{
	return false;
}

void Box::glDraw()
{

  	glMaterialv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, diffuseColor);

	glColor3v(diffuseColor);
	glScalef(2*extents[0],2*extents[1],2*extents[2]);

	if (wire)
	{
		glDisable(GL_LIGHTING);
		glutWireCube(1);
	}
	else
	{
		glEnable(GL_LIGHTING);
		glutSolidCube(1);
	}
}

void Box::renderShadowVolumes(const Se3r& se3, const Vector3r& lightPos)
{
	Vector3r pos = lightPos-se3.translation;
	unsigned char zone=0; // bits 7 and 8 are always equal to 0
	
	if (pos[0]>extents[0])
		zone |= 0x01;
	else if (pos[0]<-extents[0])
		zone |= 0x02;
		
	if (pos[1]>extents[1])
		zone |= 0x04;
	else if (pos[1]<-extents[1])
		zone |= 0x08;
		
	if (pos[2]>extents[2])
		zone |= 0x10;
	else if (pos[2]<-extents[2])
		zone |= 0x20;
	
	
	vector<Vector3r> vertices;
	
	vertices.push_back(Vector3r(-extents[0],extents[1],extents[2]));
	vertices.push_back(Vector3r(extents[0],extents[1],extents[2]));
	vertices.push_back(Vector3r(extents[0],extents[1],-extents[2]));
	vertices.push_back(Vector3r(-extents[0],extents[1],-extents[2]));
	vertices.push_back(Vector3r(-extents[0],-extents[1],extents[2]));
	vertices.push_back(Vector3r(extents[0],-extents[1],extents[2]));
	vertices.push_back(Vector3r(extents[0],-extents[1],-extents[2]));
	vertices.push_back(Vector3r(-extents[0],-extents[1],-extents[2]));
	
	vector<int> hull;
	hull.clear();	
	switch (zone)
	{
		// light is inside box
		case 0x00 :
		break;
		
		// light see one face : 
		case 0x01 : // see right face
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(6);
		break;
		case 0x02 : // see left face
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(5);
		break;		
		case 0x04 : // see top face
			hull.push_back(1);
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(4);
		break;
		case 0x08 : // see bottom face
			hull.push_back(5);
			hull.push_back(6);
			hull.push_back(7);
			hull.push_back(8);
		break;
		case 0x10 : // see front face
			hull.push_back(1);
			hull.push_back(2);
			hull.push_back(6);
			hull.push_back(5);
		break;
		case 0x20 : // see back face
			hull.push_back(3);
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(7);
		break;
		
		// light see one vertex : 
		case 0x16 : // see vertex 1
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(5);
			hull.push_back(6);
		break;
		case 0x15 : // see vertex 2
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(6);
			hull.push_back(5);
		break;
		case 0x25 : // see vertex 3
			hull.push_back(2);
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(7);
			hull.push_back(6);
		break;
		case 0x26 : // see vertex 4
			hull.push_back(1);
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(8);
			hull.push_back(5);
		break;
		case 0x1c : // see vertex 5
			hull.push_back(1);
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(8);
			hull.push_back(4);
		break;
		case 0x19 : // see vertex 6
			hull.push_back(1);
			hull.push_back(5);
			hull.push_back(8);
			hull.push_back(7);
			hull.push_back(3);
			hull.push_back(4);
		break;
		case 0x29 : // see vertex 7
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(5);
			hull.push_back(6);
		break;
		case 0x2c : // see vertex 8
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(6);
			hull.push_back(5);
		break;

		
		// light see one edge	
		case 0x14 : // see edge 1-2
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(3);
			hull.push_back(2);
			hull.push_back(6);
			hull.push_back(5);
		break;
		case 0x05 : // see edge 2-3
			hull.push_back(2);
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(6);
		break;
		case 0x24 : // see edge 3-4
			hull.push_back(3);
			hull.push_back(2);
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(7);
		break;
		case 0x06 : // see edge 4-1
			hull.push_back(2);
			hull.push_back(1);
			hull.push_back(5);
			hull.push_back(8);
			hull.push_back(4);
			hull.push_back(3);
		break;
		case 0x18 : // see edge 5-6
			hull.push_back(1);
			hull.push_back(5);
			hull.push_back(8);
			hull.push_back(7);
			hull.push_back(6);
			hull.push_back(2);
		break;
		case 0x09 : // see edge 6-7
			hull.push_back(5);
			hull.push_back(6);
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(8);
		break;
		case 0x28 : // see edge 7-8
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(5);
			hull.push_back(6);
			hull.push_back(7);
			hull.push_back(3);
		break;
		case 0x0a : // see edge 8-5
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(7);
			hull.push_back(6);
			hull.push_back(5);
			hull.push_back(1);
		break;
		case 0x12 : // see edge 1-5
			hull.push_back(4);
			hull.push_back(1);
			hull.push_back(2);
			hull.push_back(6);
			hull.push_back(5);
			hull.push_back(8);
		break;
		case 0x11 : // see edge 2-6
			hull.push_back(1);
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(6);
			hull.push_back(5);
		break;
		case 0x21 : // see edge 3-7
			hull.push_back(2);
			hull.push_back(3);
			hull.push_back(4);
			hull.push_back(8);
			hull.push_back(7);
			hull.push_back(6);
		break;
		case 0x22 : // see edge 4-8
			hull.push_back(1);
			hull.push_back(4);
			hull.push_back(3);
			hull.push_back(7);
			hull.push_back(8);
			hull.push_back(5);
		break;
	}
	
	
	
	glPushMatrix();
	
	Real angle;
	Vector3r axis;	
	se3.rotation.toAxisAngle(axis,angle);	
	glTranslatef(se3.translation[0],se3.translation[1],se3.translation[2]);
	glRotatef(angle*Mathr::RAD_TO_DEG,axis[0],axis[1],axis[2]);	

	Vector3r p1,p2;
	glBegin(GL_QUAD_STRIP);
		p1 = vertices[hull[0]-1];
		p2 = p1 + (p1-pos)*10;
		glVertex3fv(p2);
		glVertex3fv(p1);
		for(int i=1;i<=hull.size();i++)
		{
			p1 = vertices[hull[i%hull.size()]-1];
			p2 = p1 + (p1-pos)*10;
			glVertex3fv(p2);
			glVertex3fv(p1);
		}
	glEnd();
	
	glPopMatrix();
}
