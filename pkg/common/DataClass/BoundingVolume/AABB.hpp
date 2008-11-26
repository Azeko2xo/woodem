/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef AABB_HPP
#define AABB_HPP

#include<yade/core/BoundingVolume.hpp>

class AABB : public BoundingVolume
{
	public :
		Vector3r	 halfSize
				,center;

		AABB();
		virtual ~AABB();
	
/// Serialization
	
	REGISTER_CLASS_NAME(AABB);	
	REGISTER_BASE_CLASS_NAME(BoundingVolume);

/// Indexable
	
	REGISTER_CLASS_INDEX(AABB,BoundingVolume);
};

REGISTER_SERIALIZABLE(AABB);

#endif // __AABB_H__

