/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/Bound.hpp>

/*! Representation of bound by min and max points.

This class is redundant, since it has no data members; don't delete it, though,
as Bound::{min,max} might move here one day.

*/
class Aabb : public Bound{
	public :
		virtual ~Aabb();
	
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(Aabb,Bound,"Axis-aligned bounding box, for use with :yref:`InsertionSortCollider`. (This class is quasi-redundant since min,max are already contained in :yref:`Bound` itself. That might change at some point, though.)",/*attrs*/,/*ctor*/createIndex(););
	REGISTER_CLASS_INDEX(Aabb,Bound);
};
REGISTER_SERIALIZABLE(Aabb);


