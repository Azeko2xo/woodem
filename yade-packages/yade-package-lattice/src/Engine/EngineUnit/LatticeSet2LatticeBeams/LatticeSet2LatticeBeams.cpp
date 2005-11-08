/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "LatticeSet2LatticeBeams.hpp"
#include "LatticeSetParameters.hpp"
#include "LatticeBeamParameters.hpp"
#include "LineSegment.hpp"
#include <yade/yade-core/MetaBody.hpp>

void LatticeSet2LatticeBeams::calcBeamsPositionOrientationNewLength(shared_ptr<Body>& body, const shared_ptr<BodyContainer>& bodies)
{

// FIXME - this copying of length between latticeBeam geometry and physics, inside MetaBody could
//         be done just once, if length was inside shared_ptr. This can be improved once we make
//         indexable Parameters: Velocity, Position, Orientation, ....
// FIXME - verify that this updating of length, position, orientation and color is in correct place/plugin

	LineSegment* line 		= dynamic_cast<LineSegment*>		(body->geometricalModel.get());
	LatticeBeamParameters* beam 	= dynamic_cast<LatticeBeamParameters*>  (body->physicalParameters.get());

	shared_ptr<Body>& bodyA 	= (*(bodies))[beam->id1];
	shared_ptr<Body>& bodyB 	= (*(bodies))[beam->id2];
	Se3r& se3A 			= bodyA->physicalParameters->se3;
	Se3r& se3B 			= bodyB->physicalParameters->se3;
	
	Se3r se3Beam;
	se3Beam.position 		= (se3A.position + se3B.position)*0.5;
	Vector3r dist 			= se3A.position - se3B.position;
	
	Real length 			= dist.normalize();
	beam->direction 		= dist;
	beam->length 			= length;
	
	se3Beam.orientation.align( Vector3r::UNIT_X , dist );
	beam->se3 			= se3Beam;
	
	
	line->length       		= beam->length;
	line->diffuseColor 		= Vector3f(0.8,0.8,0.8) + ((beam->length / beam->initialLength)-1.0) * Vector3f(-1.0,0.0,1.0) * 10.0;

}

void LatticeSet2LatticeBeams::go(	  const shared_ptr<PhysicalParameters>& ph
					, shared_ptr<GeometricalModel>& 
					, const Body* body)
{
	int beamGroupMask = dynamic_cast<const LatticeSetParameters*>(ph.get())->beamGroupMask;
	const MetaBody * ncb = dynamic_cast<const MetaBody*>(body);
	const shared_ptr<BodyContainer>& bodies = ncb->bodies;

	BodyContainer::iterator bi    = bodies->begin();
	BodyContainer::iterator biEnd = bodies->end();
	for(  ; bi!=biEnd ; ++bi )
	{
		shared_ptr<Body> b = *bi;	
		if( b->getGroupMask() & beamGroupMask )
			calcBeamsPositionOrientationNewLength(b,bodies);
	}
}
