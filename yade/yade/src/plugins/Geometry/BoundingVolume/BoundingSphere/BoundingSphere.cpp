#include "BoundingSphere.hpp"
#include "OpenGLWrapper.hpp"

BoundingSphere::BoundingSphere (Real r) : BoundingVolume()
{
	radius = r;
	createIndex();
}


BoundingSphere::~BoundingSphere ()
{

}

void BoundingSphere::postProcessAttributes(bool)
{

}

void BoundingSphere::registerAttributes()
{
	BoundingVolume::registerAttributes();
	REGISTER_ATTRIBUTE(radius);
	REGISTER_ATTRIBUTE(center);
}

void BoundingSphere::glDraw()
{	
	glColor3v(color);
	glTranslate(center[0],center[1],center[2]);
	glutWireSphere(radius,10,10);
}

void BoundingSphere::update(Se3r& se3)
{
	Vector3r v(radius,radius,radius);
	center = se3.translation;
	min = center-v;
	max = center+v;	
}
