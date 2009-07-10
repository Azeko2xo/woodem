/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include<yade/core/TimeStepper.hpp>
#include<yade/core/StandAloneEngine.hpp>
#include<yade/core/MetaBody.hpp>

TimeStepper::TimeStepper() : StandAloneEngine()
{	
	active = true;
	timeStepUpdateInterval = 1;
}


bool TimeStepper::isActivated(MetaBody* mb)
{
	return (active && (mb->currentIteration % timeStepUpdateInterval == 0));
}


void TimeStepper::registerAttributes()
{
	StandAloneEngine::registerAttributes();
	REGISTER_ATTRIBUTE(active);
	REGISTER_ATTRIBUTE(timeStepUpdateInterval);
}


void TimeStepper::setActive(bool a, int nb)
{
	active = a; 
	if (nb>0)
		timeStepUpdateInterval = (unsigned int)nb;
}


