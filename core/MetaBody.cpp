/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include <Wm3Math.h>
#include<yade/lib-base/yadeWm3.hpp>
#include "MetaBody.hpp"
#include "Engine.hpp"
#include "TimeStepper.hpp"
#include<boost/foreach.hpp>


/* this is meant to improve usability: MetaBody is ready by default (so is Omega by that token)
 * and different type of containers can still be used instead by explicit assignment */
#include<yade/core/BodyRedirectionVector.hpp>
#include<yade/core/InteractionVecSet.hpp>
#include<yade/core/PhysicalActionVectorVector.hpp>

MetaBody::MetaBody() :
	  Body(),bodies(new BodyRedirectionVector),persistentInteractions(new InteractionVecSet),transientInteractions(new InteractionVecSet),physicalActions(new PhysicalActionVectorVector)
{	
	engines.clear();
	initializers.clear();
	recover=false;

	// I must assign something to avoid "nan" when loading. When recover=false, those can be "nan" and lead to crash.
	recoverCurrentIteration=1;
	recoverStopAtIteration=1;
	recoverSimulationTime=1;

	dt=1e-8;
}


void MetaBody::runInitializers(){
	FOREACH(shared_ptr<Engine> e, initializers){
		if(e->isActivated()) e->action(this);
	}
}

void MetaBody::postProcessAttributes(bool deserializing)
{
	runInitializers();	
	//	initializers.clear(); // FIXME - we want to delate ONLY some of them!
	//                                       because when you save and load file, you still want some initializers, but not all of them. Eg - you don't want VRML loader, or FEM loader, but you want BoundingVolumeMetaEngine. Maybe we need two list of initilizers? One that 'survive' between load and save, and others that are deleted on first time?
}


void MetaBody::registerAttributes()
{
	Body::registerAttributes();
	REGISTER_ATTRIBUTE(engines);
	REGISTER_ATTRIBUTE(initializers);
	REGISTER_ATTRIBUTE(bodies);
	REGISTER_ATTRIBUTE(transientInteractions);
	REGISTER_ATTRIBUTE(persistentInteractions);
	REGISTER_ATTRIBUTE(physicalActions); // FIXME - needed or not ? - Olivier : yes it is needed if there is no default initialization into constructor
	REGISTER_ATTRIBUTE(miscParams);
	REGISTER_ATTRIBUTE(dt);

	REGISTER_ATTRIBUTE(recover);
	REGISTER_ATTRIBUTE(recoverCurrentIteration);
	REGISTER_ATTRIBUTE(recoverStopAtIteration);
	REGISTER_ATTRIBUTE(recoverSimulationTime);
}


void MetaBody::moveToNextTimeStep()
{
	vector<shared_ptr<Engine> >::iterator ai    = engines.begin();
	vector<shared_ptr<Engine> >::iterator aiEnd = engines.end();
	for( ; ai!=aiEnd ; ++ai )
	{
		if ((*ai)->isActivated())
			(*ai)->action(this);
	}
}

shared_ptr<Engine> MetaBody::engineByName(string s){
	FOREACH(shared_ptr<Engine> e, engines){
		if(e->getClassName()==s) return e;
	}
	return shared_ptr<Engine>();
}


void MetaBody::setTimeSteppersActive(bool a)
{
	FOREACH(shared_ptr<Engine> e, engines){
		if (Omega::instance().isInheritingFrom(e->getClassName(),"TimeStepper"))
			(dynamic_pointer_cast<TimeStepper>(e))->setActive(a);
	}
}

