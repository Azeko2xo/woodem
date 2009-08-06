/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include"Body.hpp"
#include"BodyContainer.hpp"
#include"Engine.hpp"
#include"DisplayParameters.hpp"
#include"BexContainer.hpp"
#include"GroupRelationData.hpp"

class MetaBody : public Body
{
	public :
		shared_ptr<BodyContainer>		bodies;
		vector<shared_ptr<Engine> >		engines;
		vector<shared_ptr<Engine> >		initializers; // FIXME: see MovingSupport:50
		shared_ptr<InteractionContainer> interactions;
		// only alias for interactions, will be removed
		shared_ptr<InteractionContainer>&	transientInteractions;	// disappear, reappear according to spatial criterion

		BexContainer bex;

		vector<shared_ptr<Serializable> > miscParams; // will set static parameters during deserialization (primarily for GLDraw functors which otherwise have no attribute access)
		//! tags like mp3 tags: author, date, version, description etc.
		list<string> tags;
		//! "hash maps" of display parameters
		vector<shared_ptr<DisplayParameters> > dispParams;

		shared_ptr<GroupRelationData>           grpRelationData;

		MetaBody();

		void moveToNextTimeStep();

		void setTimeSteppersActive(bool a);
		shared_ptr<Engine> engineByName(string s);
		shared_ptr<Engine> engineByLabel(string s);

		Real dt;
		long currentIteration;
		Real simulationTime;
		long stopAtIteration;
		Real stopAtVirtTime;
		Real stopAtRealTime;
		Vector3r cellMin, cellMax;
		bool isPeriodic;

		bool needsInitializers;
		// for GL selection
		body_id_t selectedBody;
	protected :
		virtual void postProcessAttributes(bool deserializing);
	REGISTER_ATTRIBUTES(Body,
		(tags)
		(grpRelationData)
		(engines)
		(initializers)
		(bodies)
		(transientInteractions)
		(miscParams)
		(dispParams)
		(dt)
		(currentIteration)
		(simulationTime)
		(stopAtIteration)
		(cellMin)
		(cellMax)
		(isPeriodic)
	);
	REGISTER_CLASS_AND_BASE(MetaBody,Body);
};

REGISTER_SERIALIZABLE(MetaBody);


