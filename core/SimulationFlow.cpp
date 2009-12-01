/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "SimulationFlow.hpp"
#include "World.hpp"
#include "Omega.hpp"

void SimulationFlow::singleAction()
{
	Omega& OO=Omega::instance();
	if (OO.getWorld()) // FIXME - would it contain the loop in the private variables, this check would be unnecessary
	{
		OO.getWorld()->moveToNextTimeStep();
		if(OO.getWorld()->stopAtIteration>0 && OO.getCurrentIteration()==OO.getWorld()->stopAtIteration){
			setTerminate(true);
		}
	}
};

