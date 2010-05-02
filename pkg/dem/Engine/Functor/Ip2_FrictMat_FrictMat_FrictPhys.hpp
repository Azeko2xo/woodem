/*************************************************************************
*  Copyright (C) 2007 by Bruno CHAREYRE                                 *
*  bruno.chareyre@hmg.inpg.fr                                        *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/InteractionPhysicsFunctor.hpp>

class Ip2_FrictMat_FrictMat_FrictPhys: public InteractionPhysicsFunctor{
	public:
		virtual void go(const shared_ptr<Material>& b1,
			const shared_ptr<Material>& b2,
			const shared_ptr<Interaction>& interaction);
	FUNCTOR2D(FrictMat,FrictMat);
	YADE_CLASS_BASE_DOC(Ip2_FrictMat_FrictMat_FrictPhys,InteractionPhysicsFunctor,"Create a :yref:`FrictPhys` from two :yref:`FrictMats<FrictMat>`. The compliance of one sphere under symetric point loads is defined here as 1/(E.D), with E the stiffness of the sphere and D its diameter, and corresponds to a compliance 1/(2.E.D) from each contact point. The compliance of the contact itself will be the sum of compliances from each sphere, i.e. 1/(2.E.D1)+1/(2.E.D1) in the general case, or 1/(E.D) in the special case of equal sizes. Note that summing compliances corresponds to an harmonic average of stiffnesss, which is how kn is actually computed in the :yref:`Ip2_FrictMat_FrictMat_FrictPhys` functor. The friction angle of the contact is defined as the minimum angle of the two materials in contact.\n\nOnly interactions with :yref:`ScGeom` or :yref:`Dem3DofGeom` geometry are meaningfully accepted; run-time typecheck can make this functor unnecessarily slow in general. Such design is problematic in itself, though -- from http://www.mail-archive.com/yade-dev@lists.launchpad.net/msg02603.html:\n\n\tYou have to suppose some exact type of InteractionGeometry in the Ip2 functor, but you don't know anything about it (Ip2 only guarantees you get certain InteractionPhysics types, via the dispatch mechanism).\n\n\tThat means, unless you use Ig2 functor producing the desired type, the code will break (crash or whatever). The right behavior would be either to accept any type (what we have now, at least in principle), or really enforce InteractionGeometry type of the interation passed to that particular Ip2 functor.\n\nEtc.");
};
REGISTER_SERIALIZABLE(Ip2_FrictMat_FrictMat_FrictPhys);


