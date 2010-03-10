/*************************************************************************
*  Copyright (C) 2008 by Jerome Duriez                                   *
*  jerome.duriez@hmg.inpg.fr                                             *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/


#include "Disp2DPropLoadEngine.hpp"
#include<yade/core/State.hpp>
#include<yade/pkg-common/Force.hpp>
#include<yade/pkg-common/Box.hpp>
#include<yade/core/Scene.hpp>
// #include<yade/lib-base/yadeWm3Extra.hpp>
#include <yade/lib-miniWm3/Wm3Math.h>


YADE_REQUIRE_FEATURE(PHYSPAR);

Disp2DPropLoadEngine::Disp2DPropLoadEngine() : actionForce(new Force), leftbox(new Body), rightbox(new Body), frontbox(new Body), backbox(new Body), topbox(new Body), boxbas(new Body)
{
	firstIt=true;
	v=0.0;
	alpha=Mathr::PI/2.0;;
	id_topbox=3;
	id_boxbas=1;
	id_boxleft=0;
	id_boxright=2;
	id_boxfront=5;
	id_boxback=4;
	Key="";
}

void Disp2DPropLoadEngine::postProcessAttributes(bool deserializing)
{
	if(deserializing)
	{
		std::string outputFile="DirSearch" + Key + "Yade";
		bool file_exists = std::ifstream (outputFile.c_str()); //if file does not exist, we will write colums titles
		ofile.open(outputFile.c_str(), std::ios::app);
		if (!file_exists) ofile<<"theta (!angle ds plan (gamma,-du) )dtau (kPa) dsigma (kPa) dgamma (m) du (m) tau0 (kPa) sigma0 (kPa) d2W coordSs0 coordTot0 coordSsF coordTotF (Yade)" << endl;
	}
}


void Disp2DPropLoadEngine::registerAttributes()
{
	DeusExMachina::registerAttributes();
	REGISTER_ATTRIBUTE(id_topbox);
	REGISTER_ATTRIBUTE(id_boxbas);
	REGISTER_ATTRIBUTE(id_boxleft);
	REGISTER_ATTRIBUTE(id_boxright);
	REGISTER_ATTRIBUTE(id_boxfront);
	REGISTER_ATTRIBUTE(id_boxback);
	REGISTER_ATTRIBUTE(v);
	REGISTER_ATTRIBUTE(theta);
	REGISTER_ATTRIBUTE(nbre_iter);
	REGISTER_ATTRIBUTE(Key);
	REGISTER_ATTRIBUTE(LOG);
}


void Disp2DPropLoadEngine::applyCondition(Scene* ncb)
{
	if(LOG) cerr << "debut applyCondi !!" << endl;
	leftbox = Body::byId(id_boxleft);
	rightbox = Body::byId(id_boxright);
	frontbox = Body::byId(id_boxfront);
	backbox = Body::byId(id_boxback);
	topbox = Body::byId(id_topbox);
	boxbas = Body::byId(id_boxbas);

	if(firstIt)
	{
		it_begin=Omega::instance().getCurrentIteration();
		H0=topbox->state->pos.Y();
		X0=topbox->state->pos.X();
		Vector3r F_sup=ncb->forces.getForce(id_topbox);
		Fn0=F_sup.Y();
		Ft0=F_sup.X();

		Real	OnlySsInt=0	// the half number of real sphere-sphere (only) interactions, at the beginning of the perturbation
			,TotInt=0	// the half number of all the real interactions, at the beginning of the perturbation
			;

		InteractionContainer::iterator ii    = ncb->interactions->begin();
		InteractionContainer::iterator iiEnd = ncb->interactions->end();
        	for(  ; ii!=iiEnd ; ++ii ) 
        	{
        		if ((*ii)->isReal)
                	{
				TotInt++;
				const shared_ptr<Body>& b1 = Body::byId( (*ii)->getId1() );
				const shared_ptr<Body>& b2 = Body::byId( (*ii)->getId2() );
				if ( (b1->isDynamic) && (b2->isDynamic) )
					OnlySsInt++;
			}
		}
	
		coordSs0 = OnlySsInt/8590;	// 8590 is the number of spheres in the CURRENT case
		coordTot0 = TotInt / 8596;	// 8596 is the number of bodies in the CURRENT case

		firstIt=false;
	}


	if ( (Omega::instance().getCurrentIteration()-it_begin) < nbre_iter)
	{	letDisturb(ncb);
	}
	else if ( (Omega::instance().getCurrentIteration()-it_begin) == nbre_iter)
	{
		stopMovement();
		string fileName=Key + "DR"+lexical_cast<string> (nbre_iter)+"ItAtV_"+lexical_cast<string> (v)+"done.xml";
// 		Omega::instance().saveSimulation ( fileName );
		saveData(ncb);
	}
}

void Disp2DPropLoadEngine::letDisturb(Scene* ncb)
{
// 	shared_ptr<BodyContainer> bodies = ncb->bodies;

	Real dt = Omega::instance().getTimeStep();
	dgamma=cos(theta*Mathr::PI/180.0)*v*dt;
	dh=sin(theta*Mathr::PI/180.0)*v*dt;

	Real Ysup = topbox->state->pos.Y();
	Real Ylat = leftbox->state->pos.Y();

// 	Changes in vertical and horizontal position :

	topbox->state->pos += Vector3r(dgamma,dh,0);

	leftbox->state->pos += Vector3r(dgamma/2.0,dh/2.0,0);
	rightbox->state->pos += Vector3r(dgamma/2.0,dh/2.0,0);

	Real Ysup_mod = topbox->state->pos.Y();
	Real Ylat_mod = leftbox->state->pos.Y();

//	with the corresponding velocities :
	topbox->state->vel=Vector3r(dgamma/dt,dh/dt,0);

	leftbox->state->vel = Vector3r((dgamma/dt)/2.0,dh/(2.0*dt),0);

	rightbox->state->vel = Vector3r((dgamma/dt)/2.0,dh/(2.0*dt),0);

//	Then computation of the angle of the rotation to be done :
	computeAlpha();
	if (alpha == Mathr::PI/2.0)	// Case of the very beginning
	{
		dalpha = - Mathr::ATan( dgamma / (Ysup_mod -Ylat_mod) );
	}
	else
	{
		Real A = (Ysup_mod - Ylat_mod) * 2.0*Mathr::Tan(alpha) / (2.0*(Ysup - Ylat) + dgamma*Mathr::Tan(alpha) );
		dalpha = Mathr::ATan( (A - Mathr::Tan(alpha))/(1.0 + A * Mathr::Tan(alpha)));
	}

	Quaternionr qcorr;
	qcorr.FromAxisAngle(Vector3r(0,0,1),dalpha);
	if(LOG)
		cout << "Quaternion associe a la rotation incrementale : " << qcorr.W() << " " << qcorr.X() << " " << qcorr.Y() << " " << qcorr.Z() << endl;

// On applique la rotation en changeant l'orientation des plaques, leurs vang et en affectant donc alpha
/*	rb = dynamic_cast<RigidBodyParameters*>(leftbox->physicalParameters.get());
	rb->se3.orientation	= qcorr*rb->se3.orientation;*/
	leftbox->state->ori = qcorr*leftbox->state->ori;
	leftbox->state->angVel = Vector3r(0,0,1)*dalpha/dt;


	rb = dynamic_cast<RigidBodyParameters*>(rightbox->physicalParameters.get());
	rightbox->state->ori = qcorr*leftbox->state->ori;
	rightbox->state->angVel = Vector3r(0,0,1)*dalpha/dt;

}


	
void Disp2DPropLoadEngine::computeAlpha()
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


void Disp2DPropLoadEngine::stopMovement()
{
	// annulation de la vitesse de la plaque du haut
// 	RigidBodyParameters * rb = YADE_CAST<RigidBodyParameters*>(topbox->physicalParameters.get());
	topbox->state->vel	=  Vector3r(0,0,0);

	// de la plaque gauche
// 	rb = YADE_CAST<RigidBodyParameters*>(leftbox->physicalParameters.get());
	leftbox->state->vel	=  Vector3r(0,0,0);
	leftbox->state->angVel	=  Vector3r(0,0,0);

	// de la plaque droite
	rightbox->state->vel	=  Vector3r(0,0,0);
	rightbox->state->angVel	=  Vector3r(0,0,0);
}


void Disp2DPropLoadEngine::saveData(Scene* ncb)
{
	Real Xleft = leftbox->state->position.X() + (YADE_CAST<Box*>(leftbox->shape.get()))->extents.X();

	Real Xright = rightbox->state->pos.X() - (YADE_CAST<Box*>(rightbox->shape.get()))->extents.X();

	Real Zfront = frontbox->state->pos.Z() - YADE_CAST<Box*>(frontbox->shape.get())->extents.Z();
	Real Zback = backbox->state->pos.Z() + (YADE_CAST<Box*>(backbox->shape.get()))->extents.Z();

	Real Scontact = (Xright-Xleft)*(Zfront-Zback);	// that's so the value of section at the middle of the height of the box

	InteractionContainer::iterator ii    = ncb->interactions->begin();
        InteractionContainer::iterator iiEnd = ncb->interactions->end();

	Real	OnlySsInt=0	// the half number of real sphere-sphere (only) interactions, at the end of the perturbation
		,TotInt=0	// the half number of all the real interactions, at the end of the perturbation
		;
        for(  ; ii!=iiEnd ; ++ii ) 
        {
        	if ((*ii)->isReal)
                {
			TotInt++;
			const shared_ptr<Body>& b1 = Body::byId( (*ii)->getId1() );
			const shared_ptr<Body>& b2 = Body::byId( (*ii)->getId2() );
			if ( (b1->isDynamic) && (b2->isDynamic) )
				OnlySsInt++;
		}
	}
	
	Real	coordSs = OnlySsInt/8590,	// 8590 is the number of spheres in the CURRENT case
		coordTot = TotInt / 8596;	// 8596 is the number of bodies in the CURRENT case

	Vector3r& F_sup = ncb->forces.getForce(id_topbox);

	Real	dFn=F_sup.Y()-Fn0	// OK pour le signe
		,dFt=(F_sup.X()-Ft0)
		,du=-( topbox->state->pos.Y() - H0 )	// OK pour le signe (>0 en compression)
		,dgamma=topbox->state->pos.X()-X0
		,SigN0 = (Fn0/Scontact)/1000	// en kPa, pour comparer à Fortran
		,Tau0 = -(Ft0/Scontact)/1000	// en kPa, pour comparer à Fortran, Ok pour le signe, cf p. Yade29
		,dSigN= (dFn/Scontact)/1000	// Ok pour le signe
		,dTau = -(dFt/Scontact)/1000	// Ok pour le signe, idem que Tau0
		,d2W = dSigN * du + dTau * dgamma
		;

	ofile << lexical_cast<string> (theta) << " "<< lexical_cast<string> (dTau) << " " << lexical_cast<string> (dSigN) << " "
		<< lexical_cast<string> (dgamma)<<" " << lexical_cast<string> (du) << " " << lexical_cast<string> (Tau0) << " "
		<< lexical_cast<string> (SigN0) << " " << lexical_cast<string> (d2W) << " " 
		<< lexical_cast<string> (coordSs0) << " " << lexical_cast<string> (coordTot0) << " "
		<< lexical_cast<string> (coordSs) << " " << lexical_cast<string> (coordTot) <<endl;
}



