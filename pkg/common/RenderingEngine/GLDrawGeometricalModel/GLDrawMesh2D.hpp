/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/GLDrawFunctors.hpp>

class GLDrawMesh2D : public GLDrawGeometricalModelFunctor
{	
	public :
		virtual void go(const shared_ptr<GeometricalModel>&, const shared_ptr<PhysicalParameters>&,bool);

	RENDERS(Mesh2D);
	REGISTER_CLASS_NAME(GLDrawMesh2D);
	REGISTER_BASE_CLASS_NAME(GLDrawGeometricalModelFunctor);

};

REGISTER_SERIALIZABLE(GLDrawMesh2D);


