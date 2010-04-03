/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/PartialEngine.hpp>
#include<yade/lib-base/Math.hpp>

class TranslationEngine : public PartialEngine {
	public:
		virtual void action();
		virtual void postProcessAttributes(bool deserializing){ if(deserializing) translationAxis.Normalize(); }
	YADE_CLASS_BASE_DOC_ATTRS(TranslationEngine,PartialEngine,"This engine is the base class for different engines, which require any kind of motion.",
		((Real,velocity,,"Velocity [m/s]"))
		((Vector3r,translationAxis,,"Direction [Vector3]"))
	);
};
REGISTER_SERIALIZABLE(TranslationEngine);


