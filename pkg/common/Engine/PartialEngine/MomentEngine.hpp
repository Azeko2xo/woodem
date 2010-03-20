/*************************************************************************
*  Copyright (C) 2008 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/PartialEngine.hpp>

class MomentEngine: public PartialEngine{
	public:
		virtual void action();
	YADE_CLASS_BASE_DOC_ATTRS(MomentEngine,PartialEngine,"Apply given torque (momentum) value at every subscribed particle, at every step.",
		((Vector3r,moment,Vector3r::ZERO,"Torque value to be applied."))
	);
};
REGISTER_SERIALIZABLE(MomentEngine);


