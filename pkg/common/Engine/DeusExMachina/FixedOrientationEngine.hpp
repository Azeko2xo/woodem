/*************************************************************************
*  Copyright (C) 2007 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef FIXED_ORIENTATION_ENGINE_HPP
#define FIXED_ORIENTATION_ENGINE_HPP

#include<yade/core/DeusExMachina.hpp>
#include <Wm3Vector3.h>
#include<yade/lib-base/yadeWm3.hpp>

class FixedOrientationEngine : public DeusExMachina
{
	public :
		Quaternionr fixedOrientation;
		void applyCondition(Body * body);

		FixedOrientationEngine();

	protected :
		virtual void postProcessAttributes(bool);
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(FixedOrientationEngine);
	REGISTER_BASE_CLASS_NAME(DeusExMachina);
};

REGISTER_SERIALIZABLE(FixedOrientationEngine,false);

#endif //  DISPLACEMENTENGINE_HPP

