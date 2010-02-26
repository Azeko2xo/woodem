/*************************************************************************
*  Copyright (C) 2008 by Jérôme DURIEZ                                   *
*  duriez@geo.hmg.inpg.fr                                                *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-dem/FrictPhys.hpp>

/*! \brief Interaction for using NormalInelasticityLaw

This interaction is similar to CohesiveFrictionalContactInteraction. Among the differences are the unMax, previousun and previousFn (allowing to describe the inelastic unloadings in compression), no more shear and tension Adhesion, no more "fragile", "cohesionBroken" and "cohesionDisablesFriction"
 */

class NormalInelasticityPhys : public FrictPhys
{
	public :
	
// 		NormalInelasticityPhys();
		virtual ~NormalInelasticityPhys();
		void SetBreakingState ();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(NormalInelasticityPhys,FrictPhys,
				 "Physics (of interaction) for using NormalInelasticityLaw",
				 ((Real,unMax,0.0,"the maximum value of penetration depth of the history of this interaction"))
				 ((Real,previousun,0.0,"the value of this un at the last time step"))
				 ((Real,previousFn,0.0,"the value of the normal force at the last time step"))
				 ((Quaternionr,initialOrientation1,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Quaternionr,initialOrientation2,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Quaternionr,orientationToContact1,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Quaternionr,orientationToContact2,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Quaternionr,currentContactOrientation,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Quaternionr,initialContactOrientation,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Vector3r,initialPosition1,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Vector3r,initialPosition2,Quaternionr(1.0,0.0,0.0,0.0),""))
				 ((Real,forMaxMoment,1.0,"parameter stored for each interaction, and allowing to compute the maximum value of the exchanged torque : TorqueMax= forMaxMoment * NormalForce"))
				 ((Real,kr,0.0,"the rolling stiffness of the rigidity")),
				 createIndex();
				 );
	REGISTER_CLASS_INDEX(NormalInelasticityPhys,FrictPhys);
};

REGISTER_SERIALIZABLE(NormalInelasticityPhys);


