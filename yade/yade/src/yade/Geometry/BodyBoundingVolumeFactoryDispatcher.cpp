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

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "BodyBoundingVolumeFactoryDispatcher.hpp"
#include "ComplexBody.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

//FIXME : should this class be abstract interface
BodyBoundingVolumeFactoryDispatcher::BodyBoundingVolumeFactoryDispatcher () : Actor()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

BodyBoundingVolumeFactoryDispatcher::~BodyBoundingVolumeFactoryDispatcher ()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool BodyBoundingVolumeFactoryDispatcher::isActivated()
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void BodyBoundingVolumeFactoryDispatcher::updateBoundingVolume(shared_ptr<Body> b)
{
	if (b->containSubBodies)
	{
		shared_ptr<ComplexBody>  ncb = dynamic_pointer_cast<ComplexBody>(b);
		shared_ptr<BodyContainer> bodies = ncb->bodies;
		for( bodies->gotoFirst() ; bodies->notAtEnd() ; bodies->gotoNext())
		{
			updateBoundingVolume(bodies->getCurrent());
		}
	}
	
	bvFactoriesManager(b->interactionGeometry,b->boundingVolume,b->physicalParameters->se3);

//	buildBoundingVolumeDispatcher(cm,se3,bv);
		
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void BodyBoundingVolumeFactoryDispatcher::updateBoundingVolume(Body* b)
{
	if (b->containSubBodies)
	{
		ComplexBody * ncb = dynamic_cast<ComplexBody*>(b);
		shared_ptr<BodyContainer> bodies = ncb->bodies;
		for( bodies->gotoFirst() ; bodies->notAtEnd() ; bodies->gotoNext())
		{
			updateBoundingVolume(bodies->getCurrent());
		}
	}
	
	bvFactoriesManager(b->interactionGeometry,b->boundingVolume,b->physicalParameters->se3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void BodyBoundingVolumeFactoryDispatcher::action(Body* b)
{
	updateBoundingVolume(b);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void BodyBoundingVolumeFactoryDispatcher::postProcessAttributes(bool deserializing)
{
	if(deserializing)
	{
		for(unsigned int i=0;i<bvFactories.size();i++)
			bvFactoriesManager.add(bvFactories[i][0],bvFactories[i][1],bvFactories[i][2]);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void BodyBoundingVolumeFactoryDispatcher::registerAttributes()
{
	REGISTER_ATTRIBUTE(bvFactories);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void BodyBoundingVolumeFactoryDispatcher::addBVFactories(const string& str1,const string& str2,const string& str3)
{
	vector<string> v;
	v.push_back(str1);
	v.push_back(str2);
	v.push_back(str3);
	bvFactories.push_back(v);

}
