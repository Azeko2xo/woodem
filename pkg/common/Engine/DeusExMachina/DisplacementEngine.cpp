/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "DisplacementEngine.hpp"
#include<yade/core/MetaBody.hpp>

void DisplacementEngine::postProcessAttributes(bool deserializing)
{
	if(deserializing)
	{
		translationAxis.Normalize();

	/*	
		if(displacement==0)
			displacement=6e-9;
		else
			displacement=0;
			//displacement=3e-8;
		
		//displacement *= 20.0;
	*/
	//	std::cerr << "displacement 0.00000000375*2: " << displacement << "\n";
		std::cerr << "displacement: " << displacement << "\n";
	}
}


void DisplacementEngine::registerAttributes()
{
	DeusExMachina::registerAttributes();
	REGISTER_ATTRIBUTE(displacement);
	REGISTER_ATTRIBUTE(translationAxis);
	REGISTER_ATTRIBUTE(active);
}

bool DisplacementEngine::isActivated()
{
   return active;
}

void DisplacementEngine::applyCondition(Body * body)
{

/// FIXME - that's a hack! more control needed from the GUI !
//
	static int oldSec;
	static int count=0;
	static bool initialized=false;
	if(!initialized)
	{
		Omega::instance().isoSec=0;
		oldSec=Omega::instance().isoSec=0;
		initialized=true;
	}
	if(oldSec!=Omega::instance().isoSec)
	{
		std::cerr << "multiplication by dt, before: " << displacement;
		displacement*=Omega::instance().getTimeStep();
		std::cerr << "after: " << displacement << "\n";
		if((count++)%6==0) oldSec=Omega::instance().isoSec;
	}




	MetaBody * ncb = static_cast<MetaBody*>(body);
	shared_ptr<BodyContainer>& bodies = ncb->bodies;

	std::vector<int>::const_iterator ii = subscribedBodies.begin();
	std::vector<int>::const_iterator iiEnd = subscribedBodies.end();

//
// FIXME - we really need to set intervals for engines.
//

//	if(Omega::instance().getCurrentIteration() > 2000)
//		return;


//      {


	for(;ii!=iiEnd;++ii)
		if( bodies->exists(*ii) )
			((*bodies)[*ii]->physicalParameters.get())->se3.position += displacement*translationAxis;
		
		
//	}
//	else
//	{
//	for(;ii!=iiEnd;++ii)
//		((*bodies)[*ii])->isDynamic = true;
//	}

}

YADE_PLUGIN();
