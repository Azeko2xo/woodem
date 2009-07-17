/*************************************************************************
*  Copyright (C) 2008 by Vincent Richefeu                                *
*  vincent.richefeu@hmg.inpg.fr                                          *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once


#include<yade/core/InteractingGeometry.hpp>


class BssSweptSphereLineSegment : public InteractingGeometry
{
	public :
                Real     radius, length; // FIXME halfLength
		Vector3r position;    // This position is different from the se3 one FIXME (usefull?)
                Vector3r orientation; // This is a normalized vector


                BssSweptSphereLineSegment ();
                virtual ~BssSweptSphereLineSegment ();	
	
	REGISTER_ATTRIBUTES(InteractingGeometry,(radius)(length)(position)(orientation));
	REGISTER_CLASS_INDEX(BssSweptSphereLineSegment,InteractingGeometry);
	REGISTER_CLASS_NAME(BssSweptSphereLineSegment);
	REGISTER_BASE_CLASS_NAME(InteractingGeometry);
};

REGISTER_SERIALIZABLE(BssSweptSphereLineSegment);


