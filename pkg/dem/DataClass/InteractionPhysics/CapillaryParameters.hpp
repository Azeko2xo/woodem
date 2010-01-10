/*************************************************************************
*  Copyright (C) 2006 by luc Scholtes                                    *
*  luc.scholtes@hmg.inpg.fr                                              *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#pragma once
//#include <yade/core/InteractionPhysics.hpp>
#include<yade/pkg-dem/FrictPhys.hpp>

class CapillaryParameters : public FrictPhys
{
	public :	Real	 Vmeniscus
				,CapillaryPressure
				,Delta1
				,Delta2;
				
		Vector3r	Fcap;
						
		bool 		meniscus;
		short int	fusionNumber;//number of meniscii that are overlaping with this one
		int		currentIndexes [4];// used for faster interpolation (stores previous positions in tables)
				
		CapillaryParameters();
		virtual ~CapillaryParameters();
	REGISTER_ATTRIBUTES(FrictPhys,(Vmeniscus)(CapillaryPressure)(Fcap)(Delta1)(Delta2)(meniscus)(fusionNumber));
	REGISTER_CLASS_NAME(CapillaryParameters);
	REGISTER_BASE_CLASS_NAME(FrictPhys);
};

REGISTER_SERIALIZABLE(CapillaryParameters);


