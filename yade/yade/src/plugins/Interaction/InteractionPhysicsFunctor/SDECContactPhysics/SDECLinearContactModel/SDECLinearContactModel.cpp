/***************************************************************************
 *   Copyright (C) 2004 by Olivier Galizzi                                 *
 *   olivier.galizzi@imag.fr                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "SDECLinearContactModel.hpp"
#include "SDECContactGeometry.hpp"
#include "SDECContactPhysics.hpp"

#include "SDECLinkGeometry.hpp" // FIXME - I can't dispatch by SDECLinkGeometry <-> SDECContactGeometry !!?
#include "SDECLinkPhysics.hpp" // FIXME

#include "Omega.hpp"
#include "ComplexBody.hpp"
#include "SDECParameters.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void SDECLinearContactModel::go(	  const shared_ptr<BodyPhysicalParameters>& b1 // SDECParameters
					, const shared_ptr<BodyPhysicalParameters>& b2 // SDECParameters
					, const shared_ptr<Interaction>& interaction)
{
	SDECParameters* de1 = static_cast<SDECParameters*>(b1.get());
	SDECParameters* de2 = static_cast<SDECParameters*>(b2.get());
	SDECContactGeometry* interactionGeometry = dynamic_cast<SDECContactGeometry*>(interaction->interactionGeometry.get());
	
	if(interactionGeometry) // so it is SDECContactGeometry  - NON PERMANENT LINK
	{
		shared_ptr<SDECContactPhysics> contactPhysics;
		
		if ( interaction->isNew)
		{
			interaction->interactionPhysics = shared_ptr<SDECContactPhysics>(new SDECContactPhysics());
			contactPhysics = dynamic_pointer_cast<SDECContactPhysics>(interaction->interactionPhysics);
			
			contactPhysics->initialKn			= 2*(de1->kn*de2->kn)/(de1->kn+de2->kn);
			contactPhysics->initialKs			= 2*(de1->ks*de2->ks)/(de1->ks+de2->ks);
			contactPhysics->prevNormal 			= interactionGeometry->normal;
			contactPhysics->initialEquilibriumDistance	= interactionGeometry->radius1+interactionGeometry->radius2;
		}
		else
			contactPhysics = dynamic_pointer_cast<SDECContactPhysics>(interaction->interactionPhysics);
		
		contactPhysics->kn = contactPhysics->initialKn;
		contactPhysics->ks = contactPhysics->initialKs;
		contactPhysics->equilibriumDistance = contactPhysics->initialEquilibriumDistance;
		
		
	}
	else   // this is PERMANENT LINK because previous dynamic_cast failed, dispatcher should do this job
	{
		SDECLinkGeometry* sdecLinkGeometry =  dynamic_cast<SDECLinkGeometry*>(interaction->interactionGeometry.get());
		assert( sdecLinkGeometry );
		
		shared_ptr<SDECLinkPhysics> linkPhysics = dynamic_pointer_cast<SDECLinkPhysics>(interaction->interactionPhysics);
		
		linkPhysics->kn 			= linkPhysics->initialKn;
		linkPhysics->ks 			= linkPhysics->initialKs;
		linkPhysics->equilibriumDistance 	= linkPhysics->initialEquilibriumDistance;
	}
};
