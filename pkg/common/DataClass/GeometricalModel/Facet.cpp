/*************************************************************************
*  Copyright (C) 2008 by Sergei Dorofeenko				 *
*  sega@users.berlios.de                                                 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "Facet.hpp"
#include<yade/lib-opengl/OpenGLWrapper.hpp>

Facet::Facet () : GeometricalModel()
{
	createIndex();
}


Facet::~Facet ()
{
}


void Facet::registerAttributes()
{
	GeometricalModel::registerAttributes();
	REGISTER_ATTRIBUTE(vertices);
}

YADE_PLUGIN();
