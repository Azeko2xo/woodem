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

#ifndef __NONCONNEXBODY_H__
#define __NONCONNEXBODY_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Body.hpp"
#include "BodyContainer.hpp"
//#include "BroadInteractor.hpp"
//#include "KinematicEngine.hpp"
//#include "NarrowInteractor.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//FIXME : NonConnexBody is abstract interface and add SetOfBodies as default simple NonConnexBody
class NonConnexBody : public Body
{
	//public : vector<shared_ptr<Body> > bodies;

	//public : shared_ptr<NarrowInteractor> narrowCollider;
	//public : shared_ptr<BroadInteractor> broadCollider;
	//public : shared_ptr<KinematicEngine> kinematic;

	//public : vector<shared_ptr<Interaction> > permanentInteractions;
	public	: shared_ptr<BodyContainer> bodies;
	public	: shared_ptr<InteractionContainer> permanentInteractions;

	// construction
	public	: NonConnexBody ();

	public	: virtual ~NonConnexBody ();

	public : virtual void glDrawGeometricalModel();
	public : virtual void glDrawBoundingVolume();
	public : virtual void glDrawCollisionGeometry();

	public	: void moveToNextTimeStep();

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Serialization										///
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	REGISTER_CLASS_NAME(NonConnexBody);
	protected : virtual void postProcessAttributes(bool deserializing);
	public	: void registerAttributes();

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Indexable											///
///////////////////////////////////////////////////////////////////////////////////////////////////

	REGISTER_CLASS_INDEX(NonConnexBody);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_SERIALIZABLE(NonConnexBody,false);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __NONCONNEXBODY_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
