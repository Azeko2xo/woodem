#include "BoundingSphere.hpp"

BoundingSphere::BoundingSphere (float r) : BoundingVolume()
{
	radius = r;
	type = 1;
	
}


BoundingSphere::~BoundingSphere ()
{

}


void BoundingSphere::move(Se3& )
{


}
void BoundingSphere::processAttributes()
{

}

void BoundingSphere::registerAttributes()
{
	BoundingVolume::registerAttributes();
	REGISTER_ATTRIBUTE(radius);
	REGISTER_ATTRIBUTE(center);
}

bool BoundingSphere::loadFromFile(char * )
{
	return false; 
}

void BoundingSphere::glDraw()
{	
	glColor3fv(color);
	glTranslatef(center[0],center[1],center[2]);
	glutWireSphere(radius,10,10);
}

void BoundingSphere::update(Se3& se3)
{
	Vector3 v(radius,radius,radius);
	center = se3.translation;
	min = center-v;
	max = center+v;	
}
