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

  	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, diffuseColor);

	glColor3v(diffuseColor);
	glScalef(2*extents[0],2*extents[1],2*extents[2]);

	if (wire)
	{
		glDisable(GL_LIGHTING);
		glutWireCube(1);
	}
	else
	{
		//glEnable(GL_LIGHTING);
		glutSolidCube(1);
	}
}
