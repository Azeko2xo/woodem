/***************************************************************************
 *   Copyright (C) 2004 by Olivier Galizzi                                 *
 *   olivier.galizzi@imag.fr                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SPHERE2SPHERE4ERRORTOLERANT_H__
#define __SPHERE2SPHERE4ERRORTOLERANT_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CollisionFunctor.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

/*! \brief Provide collision handling between a axis aligned box and a sphere in terms of ClosestFeatures */
class Sphere2Sphere4ErrorTolerant : public CollisionFunctor
{
	protected : virtual bool go(		const shared_ptr<CollisionGeometry>& cm1,
						const shared_ptr<CollisionGeometry>& cm2,
						const Se3r& se31,
						const Se3r& se32,
						shared_ptr<Interaction>& c);
	protected : virtual bool goReverse(	const shared_ptr<CollisionGeometry>& cm1,
						const shared_ptr<CollisionGeometry>& cm2,
						const Se3r& se31,
						const Se3r& se32,
						shared_ptr<Interaction>& c);

	DEFINE_FUNCTOR_ORDER_2D(Sphere,Sphere);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_FACTORABLE(Sphere2Sphere4ErrorTolerant);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __SPHERE2SPHERE4ERRORTOLERANT_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
