/*************************************************************************
*  Copyright (C) 2007 by Janek Kozicki                                   *
*  cosurgi@mail.berlios.de                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "GLDrawSpheresContactGeometry.hpp"
#include "SpheresContactGeometry.hpp"
#include "SimpleElasticInteraction.hpp"
#include "ElasticContactInteraction.hpp"

#include <yade/yade-lib-opengl/OpenGLWrapper.hpp>

GLDrawSpheresContactGeometry::GLDrawSpheresContactGeometry() : midMax(0), forceMax(0)
{
}

void GLDrawSpheresContactGeometry::go(
		const shared_ptr<InteractionGeometry>& ig,
		const shared_ptr<Interaction>& ip,
		const shared_ptr<Body>& b1,
		const shared_ptr<Body>& b2,
		bool wireFrame)
{
	SpheresContactGeometry*    sc = static_cast<SpheresContactGeometry*>(ig.get());
	ElasticContactInteraction* el = static_cast<ElasticContactInteraction*>(ip->interactionPhysics.get());
	

	if(wireFrame)
	{
		midMax=0;
		forceMax=0;

		glTranslatev(sc->contactPoint);
		glBegin(GL_LINES);
			glColor3(0.0,1.0,0.0);
			glVertex3(0.0,0.0,0.0);
			glVertex3v(-1.0*(sc->normal*sc->radius1));
			glVertex3(0.0,0.0,0.0);
			glVertex3v( 1.0*(sc->normal*sc->radius2));
		glEnd();
	}
	else
	{
		Quaternionr q;
		q.Align(Vector3r(1.0,0.0,0.0),sc->normal);
		Vector3r axis;	
		Real angle;
		q.ToAxisAngle(axis,angle);

		Real mid = (sc->radius1 + sc->radius2);
		Real dif = (sc->radius1 - sc->radius2)*0.5;
		midMax = std::max(mid,midMax);

		glTranslatev(sc->contactPoint-(sc->normal*dif));
		glRotatef(angle*Mathr::RAD_TO_DEG,axis[0],axis[1],axis[2]);
		
	// FIXME - we need a way to give parameters from outside, again.... so curerntly this scale is hardcoded here
		if( (!ip->isNew) && /*ip->isReal &&*/ ip->interactionPhysics)
		{
			Real force = el->normalForce.Length()/600;
			forceMax = std::max(force,forceMax);

			Real scale = midMax*(force/forceMax)*0.2;
			glScalef(mid,scale,scale);
		}
		else
			glScalef(mid,mid*0.05,mid*0.05);
		glColor3(0.7,0.7,0.7);
	// ///////////

		glEnable(GL_LIGHTING);
		glutSolidCube(1.0);

	}
}

