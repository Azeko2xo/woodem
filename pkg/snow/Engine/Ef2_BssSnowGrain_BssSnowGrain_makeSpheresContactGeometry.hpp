/*************************************************************************
*  Copyright (C) 2008 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/InteractionGeometryFunctor.hpp>
#include<yade/pkg-dem/InteractingSphere2InteractingSphere4SpheresContactGeometry.hpp>

class Ef2_BssSnowGrain_BssSnowGrain_makeSpheresContactGeometry : public InteractionGeometryFunctor
{
	public :
		InteractingSphere2InteractingSphere4SpheresContactGeometry g;

		virtual bool go(const shared_ptr<InteractingGeometry>& cm1, const shared_ptr<InteractingGeometry>& cm2, const State& state1, const State& state2, const Vector3r& shift2, const shared_ptr<Interaction>& c);
		virtual bool goReverse(	const shared_ptr<InteractingGeometry>& cm1, const shared_ptr<InteractingGeometry>& cm2, const State& state1, const State& state2, const Vector3r& shift2, const shared_ptr<Interaction>& c);
					
		bool assist;
		Ef2_BssSnowGrain_BssSnowGrain_makeSpheresContactGeometry():assist(false){};
		
	REGISTER_CLASS_NAME(Ef2_BssSnowGrain_BssSnowGrain_makeSpheresContactGeometry);
	REGISTER_BASE_CLASS_NAME(InteractionGeometryFunctor);

	FUNCTOR2D(BssSnowGrain,BssSnowGrain);
	
	//FIXME: what is this good for?!
	DEFINE_FUNCTOR_ORDER_2D(BssSnowGrain,BssSnowGrain);
	REGISTER_ATTRIBUTES(InteractionGeometryFunctor,/* */);
};

REGISTER_SERIALIZABLE(Ef2_BssSnowGrain_BssSnowGrain_makeSpheresContactGeometry);


