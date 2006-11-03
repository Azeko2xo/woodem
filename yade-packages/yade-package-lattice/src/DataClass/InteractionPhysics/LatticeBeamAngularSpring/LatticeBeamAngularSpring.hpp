/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef LATTICEBEAMANGULARSPRING_HPP
#define LATTICEBEAMANGULARSPRING_HPP

#include <yade/yade-core/InteractionPhysics.hpp>
#include <vector>

class LatticeBeamAngularSpring : public InteractionPhysics
{
	public :
		Vector3r		 lastCrossProduct;
		bool			 planeSwap180;
		Real			 initialPlaneAngle  // 2D only for now
			 		,planeAngle
					,initialOffPlaneAngle
					,offPlaneAngle;
		
		LatticeBeamAngularSpring();
		virtual ~LatticeBeamAngularSpring();

/// Serialization
	protected :
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(LatticeBeamAngularSpring);
	REGISTER_BASE_CLASS_NAME(InteractionPhysics);
	
/// Indexable
//	REGISTER_CLASS_INDEX(LatticeBeamAngularSpring,InteractionPhysics);

};

REGISTER_SERIALIZABLE(LatticeBeamAngularSpring,false);

#endif //  LATTICEBEAMANGULARSPRING_HPP

