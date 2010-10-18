/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#pragma once

#include<iostream>
#include"Shape.hpp"
#include"Bound.hpp"
#include"State.hpp"
#include"Material.hpp"

#include<yade/lib-base/Math.hpp>
#include<yade/lib-serialization/Serializable.hpp>
#include<yade/lib-multimethods/Indexable.hpp>

class Scene;

class Body: public Serializable{
	public:
		typedef int id_t;
		// bits for Body::flags
		enum { FLAG_DYNAMIC=1, FLAG_BOUNDED=2, FLAG_ASPHERICAL=4 }; /* add powers of 2 as needed */
		//! symbolic constant for body that doesn't exist.
		static const Body::id_t ID_NONE;
		//! get Body pointer given its id. 
		static const shared_ptr<Body>& byId(Body::id_t _id,Scene* rb=NULL);
		static const shared_ptr<Body>& byId(Body::id_t _id,shared_ptr<Scene> rb);

		
		//! Whether this Body is a Clump.
		//! @note The following is always true: \code (Body::isClump() XOR Body::isClumpMember() XOR Body::isStandalone()) \endcode
		bool isClump() const {return clumpId!=ID_NONE && id==clumpId;}
		//! Whether this Body is member of a Clump.
		bool isClumpMember() const {return clumpId!=ID_NONE && id!=clumpId;}
		//! Whether this body is standalone (neither Clump, nor member of a Clump)
		bool isStandalone() const {return clumpId==ID_NONE;}

		//! Whether this body has all DOFs blocked
		// inline accessors
		// logic: http://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit-in-c
		bool isDynamic() const {return flags & FLAG_DYNAMIC; }
		void setDynamic(bool d){ if(d) flags|=FLAG_DYNAMIC; else flags&=~(FLAG_DYNAMIC); }
		bool isBounded() const {return flags & FLAG_BOUNDED; }
		void setBounded(bool d){ if(d) flags|=FLAG_BOUNDED; else flags&=~(FLAG_BOUNDED); }
		bool isAspherical() const {return flags & FLAG_ASPHERICAL; }
		void setAspherical(bool d){ if(d) flags|=FLAG_ASPHERICAL; else flags&=~(FLAG_ASPHERICAL); }

		/*! Hook for clump to update position of members when user-forced reposition and redraw (through GUI) occurs.
		 * This is useful only in cases when engines that do that in every iteration are not active - i.e. when the simulation is paused.
		 * (otherwise, GLViewer would depend on Clump and therefore Clump would have to go to core...) */
		virtual void userForcedDisplacementRedrawHook(){return;}

		Body::id_t getId() const {return id;};

		int getGroupMask() {return groupMask; };
		bool maskOk(int mask){return (mask==0 || (groupMask&mask));}

		// only BodyContainer can set the id of a body
		friend class BodyContainer;

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Body,Serializable,"A particle, basic element of simulation; interacts with other bodies.",
		((Body::id_t,id,Body::ID_NONE,Attr::readonly,"Unique id of this body."))

		((int,groupMask,1,,"Bitmask for determining interactions."))
		((int,flags,FLAG_DYNAMIC|FLAG_BOUNDED,Attr::readonly,"Bits of various body-related flags. *Do not access directly*. In c++, use isDynamic/setDynamic, isBounded/setBounded. In python, use :yref:`Body.dynamic` and :yref:`Body.bounded`."))

		((shared_ptr<Material>,material,,,":yref:`Material` instance associated with this body."))
		((shared_ptr<State>,state,new State,,"Physical :yref:`state<State>`."))
		((shared_ptr<Shape>,shape,,,"Geometrical :yref:`Shape`."))
		((shared_ptr<Bound>,bound,,,":yref:`Bound`, approximating volume for the purposes of collision detection."))

		((int,clumpId,Body::ID_NONE,Attr::readonly,"Id of clump this body makes part of; invalid number if not part of clump; see :yref:`Body::isStandalone`, :yref:`Body::isClump`, :yref:`Body::isClumpMember` properties. \n\n This property is not meant to be modified directly from Python, use :yref:`O.bodies.appendClumped<BodyContainer.appendClumped>` instead.")),
		/* ctor */,
		/* py */
		//
		.def_readwrite("mat",&Body::material,"Shorthand for :yref:`Body::material`")
		.add_property("isDynamic",&Body::isDynamic,&Body::setDynamic,"Deprecated synonym for :yref:`Body::dynamic` |ydeprecated|")
		.add_property("dynamic",&Body::isDynamic,&Body::setDynamic,"Whether this body will be moved by forces. (In c++, use ``Body::isDynamic``/``Body::setDynamic``) :ydefault:`true`")
		.add_property("bounded",&Body::isBounded,&Body::setBounded,"Whether this body should have :yref:`Body.bound` created. Note that bodies without a :yref:`bound <Body.bound>` do not participate in collision detection. (In c++, use ``Body::isBounded``/``Body::setBounded``) :ydefault:`true`")
		.add_property("aspherical",&Body::isAspherical,&Body::setAspherical,"Whether this body has different inertia along principal axes; :yref:`NewtonIntegrator` makes use of this flag to call rotation integration routine for aspherical bodies, which is more expensive. :ydefault:`false`")
		.def_readwrite("mask",&Body::groupMask,"Shorthand for :yref:`Body::groupMask`")
		.add_property("isStandalone",&Body::isStandalone,"True if this body is neither clump, nor clump member; false otherwise.")
		.add_property("isClumpMember",&Body::isClumpMember,"True if this body is clump member, false otherwise.")
		.add_property("isClump",&Body::isClump,"True if this body is clump itself, false otherwise.");
	);
};
REGISTER_SERIALIZABLE(Body);
