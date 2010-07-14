/*************************************************************************
*  Copyright (C) 2007 by Bruno CHAREYRE                                 *
*  bruno.chareyre@hmg.inpg.fr                                        *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include"Ip2_FrictMat_FrictMat_FrictPhys.hpp"
#include<yade/pkg-dem/ScGeom.hpp>
#include<yade/pkg-dem/DemXDofGeom.hpp>
#include<yade/pkg-dem/FrictPhys.hpp>
#include<yade/core/Omega.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg-common/ElastMat.hpp>
#include <cassert>



void Ip2_FrictMat_FrictMat_FrictPhys::go( const shared_ptr<Material>& b1
					, const shared_ptr<Material>& b2
					, const shared_ptr<Interaction>& interaction)
{
	if(interaction->interactionPhysics) return;
	const shared_ptr<FrictMat>& mat1 = YADE_PTR_CAST<FrictMat>(b1);
	const shared_ptr<FrictMat>& mat2 = YADE_PTR_CAST<FrictMat>(b2);
	interaction->interactionPhysics = shared_ptr<FrictPhys>(new FrictPhys());
	const shared_ptr<FrictPhys>& contactPhysics = YADE_PTR_CAST<FrictPhys>(interaction->interactionPhysics);
	Real Ea 	= mat1->young;
	Real Eb 	= mat2->young;
	Real Va 	= mat1->poisson;
	Real Vb 	= mat2->poisson;
	
	Real Ra,Rb; Vector3r normal;	
	assert(dynamic_cast<ScGeom*>(interaction->interactionGeometry.get()));//only in debug mode
	ScGeom* sphCont=YADE_CAST<ScGeom*>(interaction->interactionGeometry.get());
	{Ra=sphCont->radius1>0?sphCont->radius1:sphCont->radius2; Rb=sphCont->radius2>0?sphCont->radius2:sphCont->radius1; normal=sphCont->normal;}
	
	//harmonic average of the two stiffnesses when (Ri.Ei/2) is the stiffness of a contact point on sphere "i"
	Real Kn = 2*Ea*Ra*Eb*Rb/(Ea*Ra+Eb*Rb);
	//same for shear stiffness
	Real Ks = 2*Ea*Ra*Va*Eb*Rb*Vb/(Ea*Ra*Va+Eb*Rb*Va);
	
	contactPhysics->frictionAngle = std::min(mat1->frictionAngle,mat2->frictionAngle);
	contactPhysics->tangensOfFrictionAngle = std::tan(contactPhysics->frictionAngle); 
	contactPhysics->prevNormal = normal;
	contactPhysics->kn = Kn;
	contactPhysics->ks = Ks;
};
YADE_PLUGIN((Ip2_FrictMat_FrictMat_FrictPhys));

