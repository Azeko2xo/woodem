/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "Aabb.hpp"

Aabb::~Aabb(){}
YADE_PLUGIN(dem,(Aabb));

#ifdef YADE_OPENGL
YADE_PLUGIN(gl,(Gl1_Aabb));

#include<yade/lib/opengl/OpenGLWrapper.hpp>

void Gl1_Aabb::go(const shared_ptr<Bound>& bv){
	Aabb* aabb = static_cast<Aabb*>(bv.get());
	glColor3v(bv->color);
	// glDisable(GL_LIGHTING);
	if(!scene->isPeriodic){
		glTranslatev(Vector3r(.5*(aabb->min+aabb->max)));
		glScalev(Vector3r(aabb->max-aabb->min));
	} else {
		glTranslatev(Vector3r(scene->cell->shearPt(scene->cell->wrapPt(.5*(aabb->min+aabb->max)))));
		glMultMatrixd(scene->cell->getGlShearTrsfMatrix());
		glScalev(Vector3r(aabb->max-aabb->min));
	}
	glutWireCube(1);
}

#endif

