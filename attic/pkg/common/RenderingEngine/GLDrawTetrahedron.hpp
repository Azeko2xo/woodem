/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/GLDrawFunctors.hpp>

class GLDrawTetrahedron : public GlGeometricalModelFunctor
{
	public :
		virtual void go(const shared_ptr<GeometricalModel>&, const shared_ptr<PhysicalParameters>&,bool);

	RENDERS(Tetrahedron);
	REGISTER_CLASS_NAME(GLDrawTetrahedron);
	REGISTER_BASE_CLASS_NAME(GlGeometricalModelFunctor);
};

REGISTER_SERIALIZABLE(GLDrawTetrahedron);


