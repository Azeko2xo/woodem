/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@mail.berlios.de                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "GLDrawSimpleElasticInteraction.hpp"
#include "SimpleElasticInteraction.hpp"


#include <yade/yade-lib-opengl/OpenGLWrapper.hpp>


void GLDrawSimpleElasticInteraction::go(
		const shared_ptr<InteractionPhysics>& ip,
		const shared_ptr<Interaction>&,
		const shared_ptr<Body>& b1,
		const shared_ptr<Body>& b2,
		bool wireFrame)
{
	SimpleElasticInteraction* p = static_cast<SimpleElasticInteraction*>(ip.get());
//	FIXME: so there must be a way for *not* storing the color inside those classes like BodyBoundingVolume and BodyState
//	       and simultaneously easy to set up the colors from the GUI according to some usful criterions:
//	          - to which group it belongs
//	          - to reflect some features of the drawed element
//	          - according to some certaion selection
//	          - etc...
	//glColor3v(bv->diffuseColor);
	Vector3r mid = 0.5*(b1->physicalParameters->se3.position+b2->physicalParameters->se3.position);
	glTranslatev(mid);
	glColor3(1.0,0.0,0.0);
	
	glPushMatrix();
	glRasterPos2i(0,0);
	std::string str=std::string("  kn=") + boost::lexical_cast<std::string>((float)(p->kn));
	for(unsigned int i=0;i<str.length();i++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[i]);
	glPopMatrix();
}

