/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "NewtonsMomentumLaw.hpp"
#include "RigidBodyParameters.hpp"
#include "Momentum.hpp"


void NewtonsMomentumLaw::go(   const shared_ptr<PhysicalAction>& a
					, const shared_ptr<PhysicalParameters>& b
					, const Body*)
{
	Momentum * am = static_cast<Momentum*>(a.get());
	RigidBodyParameters * rb = static_cast<RigidBodyParameters*>(b.get());
	
	//FIXME : should be += and we should add an Engine that reset acceleration at the beginning
	rb->angularAcceleration = am->momentum.multDiag(rb->invInertia);
}


