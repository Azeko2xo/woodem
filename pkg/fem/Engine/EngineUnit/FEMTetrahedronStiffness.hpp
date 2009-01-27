/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

// FIXME - this file should be located in: /plugins/Interaction/InteractionPhysicsEngineUnit/FEMParameters/FEMTetrahedronStiffness

#include<yade/pkg-common/PhysicalParametersEngineUnit.hpp>

class FEMTetrahedronStiffness : public PhysicalParametersEngineUnit
{

	private :
		int	 nodeGroupMask
			,tetrahedronGroupMask;

	public :
		virtual void go(	  const shared_ptr<PhysicalParameters>&
					, Body*);

	FUNCTOR1D(FEMSetParameters);	
	REGISTER_CLASS_NAME(FEMTetrahedronStiffness);
	REGISTER_BASE_CLASS_NAME(PhysicalParametersEngineUnit);

};

REGISTER_SERIALIZABLE(FEMTetrahedronStiffness);


