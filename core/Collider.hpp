/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/Bound.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/core/GlobalEngine.hpp>

class Collider : public GlobalEngine
{
	public :
		virtual ~Collider();
		/*! To probe the Bound on a bodies presense.
		 *
		 * returns list of body ids with which there is potential overlap.
		 */
		virtual  vector<Body::id_t> probeBoundingVolume(const Bound&){throw;}
		/*! Tell whether given bodies may interact, for other than spatial reasons.
		 *
		 * Concrete collider implementations should call this function if
		 * the bodies are in potential interaction geometrically.
		 */
		bool mayCollide(const Body*, const Body*);

		/*! Invalidate all persistent data (if the collider has any), forcing reinitialization at next run.
		The default implementation does nothing, colliders should override it if it is applicable.

		Currently used from Shop::flipCell, which changes cell information for bodies.
		*/
		virtual void invalidatePersistentData(){}
	YADE_CLASS_BASE_DOC(Collider,GlobalEngine,"Abstract class for finding spatial collisions between bodies.");
};
REGISTER_SERIALIZABLE(Collider);

