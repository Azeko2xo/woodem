/*************************************************************************
*  Copyright (C) 2008 by Jerome Duriez                                   *
*  jerome.duriez@hmg.inpg.fr                                             *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include<yade/core/Scene.hpp>
#include<yade/lib-base/yadeWm3Extra.hpp>
#include<yade/lib-miniWm3/Wm3Math.h>

#include"CinemDNCEngine.hpp"


CinemDNCEngine::CinemDNCEngine() : leftbox(new Body), rightbox(new Body), topbox(new Body)
{
	gamma_save.resize(0);
	temoin_save.resize(0);
	temoinfin=0;
	shearSpeed=0;
	gamma=0;
	gammalim=0;
	id_boxhaut=3;
	id_boxleft=0;
	id_boxright=2;
}


void CinemDNCEngine::applyCondition(Scene * ncb)
{
	leftbox = Body::byId(id_boxleft);
	rightbox = Body::byId(id_boxright);
	topbox = Body::byId(id_boxhaut);

	if( ((shearSpeed > 0) && (gamma<=gammalim)) || ((shearSpeed < 0) /*&& (gamma>=gammalim)*/ ) )
	{
		if(temoinfin!=0)
			temoinfin=0;
		letMove(ncb);
	}
	else
	{
		stopMovement();
		if(temoinfin==0)
		{
			Omega::instance().saveSimulation(Key + "finCis.xml");
			temoinfin=1;
		}
	}

	for(unsigned int j=0;j<gamma_save.size();j++)
	{
		if ( ( ( (shearSpeed>0)&&(gamma > gamma_save[j]) ) || ((shearSpeed<0)&&(gamma < gamma_save[j])) ) && (temoin_save[j]==0) )
		{
			stopMovement();		// reset of all the speeds before the save
			Omega::instance().saveSimulation(Key+"_"+lexical_cast<string> (floor(gamma*1000)) + "mmsheared.xml");
			temoin_save[j]=1;
		}
	}
	
}


void CinemDNCEngine::letMove(Scene * ncb)
{
	shared_ptr<BodyContainer> bodies = ncb->bodies;
	Real dt = Omega::instance().getTimeStep();
	Real dx = shearSpeed * dt;

	topbox->state->pos += Vector3r(dx,0,0);

	leftbox->state->pos += Vector3r(dx/2.0,0,0);
	rightbox->state->pos += Vector3r(dx/2.0,0,0);

	Real Ysup = topbox->state->pos.Y();
	Real Ylat = leftbox->state->pos.Y();


//	with the corresponding velocities :
	topbox->state->vel = Vector3r(shearSpeed,0,0);
	leftbox->state->vel = Vector3r(shearSpeed/2.0,0,0);
	rightbox->state->vel = Vector3r(shearSpeed/2.0,0,0);

//	Then computation of the angle of the rotation to be done :
	computeAlpha();
	if (alpha == Mathr::PI/2.0)	// Case of the very beginning
	{
		dalpha = - Mathr::ATan( dx / (Ysup -Ylat) );
	}
	else
	{
		Real A = (Ysup - Ylat) * 2.0*Mathr::Tan(alpha) / (2.0*(Ysup - Ylat) + dx*Mathr::Tan(alpha) );
		dalpha = Mathr::ATan( (A - Mathr::Tan(alpha))/(1.0 + A * Mathr::Tan(alpha)));
	}
	
	Quaternionr qcorr;
	qcorr.FromAxisAngle(Vector3r(0,0,1),dalpha);

// On applique la rotation en changeant l'orientation des plaques, leurs vang et en affectant donc alpha
	leftbox->state->ori	= qcorr*leftbox->state->ori;
	leftbox->state->angVel	= Vector3r(0,0,1)*dalpha/dt;

	rightbox->state->ori	= qcorr*rightbox->state->ori;
	rightbox->state->angVel	= Vector3r(0,0,1)*dalpha/dt;

	gamma+=dx;
}


void CinemDNCEngine::computeAlpha()
{
	Quaternionr orientationLeftBox,orientationRightBox;
	orientationLeftBox = leftbox->state->ori;
	orientationRightBox = rightbox->state->ori;
	if(orientationLeftBox!=orientationRightBox)
	{
		cout << "WARNING !!! your lateral boxes have not the same orientation, you're not in the case of a box imagined for creating these engines" << endl;
	}
	Vector3r axis;
	Real angle;
	orientationLeftBox.ToAxisAngle(axis,angle);
	alpha=Mathr::PI/2.0-angle;		// right if the initial orientation of the body (on the beginning of the simulation) is q =(1,0,0,0) = FromAxisAngle((0,0,1),0)
}

void CinemDNCEngine::stopMovement()
{
	// annulation de la vitesse de la plaque du haut
	topbox->state->vel	=  Vector3r(0,0,0);

	// de la plaque gauche
	leftbox->state->vel	=  Vector3r(0,0,0);
	leftbox->state->angVel	=  Vector3r(0,0,0);

	// de la plaque droite
	rightbox->state->vel	=  Vector3r(0,0,0);
	rightbox->state->angVel	=  Vector3r(0,0,0);
}


// YADE_REQUIRE_FEATURE(PHYSPAR);

