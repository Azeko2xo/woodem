/*************************************************************************
*  Copyright (C) 2006 by Luc Scholtes                                    *
*  luc.scholtes@hmg.inpg.fr                                              *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "SampleCapillaryPressureEngine.hpp"
#include <yade/pkg-dem/CapillaryCohesiveLaw.hpp>
#include<yade/core/Scene.hpp>
#include<yade/core/Omega.hpp>
#include<yade/pkg-dem/FrictPhys.hpp>
#include<yade/lib-base/Math.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost;
using namespace std;

SampleCapillaryPressureEngine::SampleCapillaryPressureEngine()
{
	capillaryCohesiveLaw = new CapillaryCohesiveLaw;
	capillaryCohesiveLaw->CapillaryPressure= 0;
	capillaryCohesiveLaw->sdecGroupMask = 2;	
	StabilityCriterion=0.01;
	SigmaPrecision = 0.001;
	Phase1=false;
	Phase1End = "Phase1End";
	Iteration = 0;
	UnbalancedForce = 0.01;
	pressureVariationActivated=false;
	Pressure = 0;
	PressureVariation = 1000;
	fusionDetection = true;
	binaryFusion = true;	
	pressureIntervalRec = 10000;
}

SampleCapillaryPressureEngine::~SampleCapillaryPressureEngine()
{
}


void SampleCapillaryPressureEngine::updateParameters(Scene * scene)
{
	UnbalancedForce = ComputeUnbalancedForce(scene);
	if (Omega::instance().getCurrentIteration() % 100 == 0) cerr << "UnbalancedForce=" << UnbalancedForce << endl;

	if (!Phase1 && UnbalancedForce<=StabilityCriterion && !pressureVariationActivated)
	{	
		internalCompaction = false;
		Phase1 = true;}
	
	if ( Phase1 && UnbalancedForce<=StabilityCriterion && !pressureVariationActivated)	
	{
		Real S = meanStress;
		cerr << "Smoy = " << meanStress << endl;
		if ((S > (sigma_iso - (sigma_iso*SigmaPrecision))) && (S < (sigma_iso + (sigma_iso*SigmaPrecision))))
		{
			string fileName = "../data/" + Phase1End + "_" + 
			lexical_cast<string>(Omega::instance().getCurrentIteration()) + ".xml";
			cerr << "saving snapshot: " << fileName << " ...";
			Omega::instance().saveSimulation(fileName);
			pressureVariationActivated = true;
		}	
	}
}
	

void SampleCapillaryPressureEngine::action(Scene*)
{	
	updateParameters(scene);
	TriaxialStressController::action(scene);
	if (pressureVariationActivated)		
		{
			if (Omega::instance().getCurrentIteration() % 100 == 0) cerr << "pressure variation!!" << endl;
		
			if ((Pressure>=0) && (Pressure<=1000000000)) Pressure += PressureVariation;
			capillaryCohesiveLaw->CapillaryPressure = Pressure;

 			capillaryCohesiveLaw->fusionDetection = fusionDetection;
			capillaryCohesiveLaw->binaryFusion = binaryFusion;
		}		
		else { capillaryCohesiveLaw->CapillaryPressure = Pressure;
		       capillaryCohesiveLaw->fusionDetection = fusionDetection;
		       capillaryCohesiveLaw->binaryFusion = binaryFusion;}
		if (Omega::instance().getCurrentIteration() % 100 == 0) cerr << "capillary pressure = " << Pressure << endl;
		capillaryCohesiveLaw->action(scene);
		UnbalancedForce = ComputeUnbalancedForce(scene);
}
