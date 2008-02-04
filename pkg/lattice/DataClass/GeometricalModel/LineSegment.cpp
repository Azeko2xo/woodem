/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "LineSegment.hpp"

LineSegment::LineSegment () : GeometricalModel()
{		
	createIndex();
}

LineSegment::~LineSegment ()
{		
}

void LineSegment::registerAttributes()
{
	GeometricalModel::registerAttributes();
//	REGISTER_ATTRIBUTE(length); // no need to save it
}

YADE_PLUGIN();
