/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/lib-base/yadeWm3Extra.hpp>
#include<Wm3Vector3.h>
#include<yade/lib-base/yadeWm3.hpp>
#include<yade/lib-serialization/Serializable.hpp>
#include<yade/lib-multimethods/Indexable.hpp>

class InteractingGeometry;

class GeometricalModel : public Serializable, public Indexable
{
	public :
		bool highlight,wire,shadowCaster;
		Vector3r	diffuseColor;
		GeometricalModel(): highlight(false),wire(false),shadowCaster(false),diffuseColor(Vector3r(1,1,1)){}

	REGISTER_ATTRIBUTES(/*no base*/,(highlight)(wire)(shadowCaster)(diffuseColor));
	REGISTER_CLASS_AND_BASE(GeometricalModel,Serializable Indexable);
	REGISTER_INDEX_COUNTER(GeometricalModel);
};

REGISTER_SERIALIZABLE(GeometricalModel);


