/*************************************************************************
*  Copyright (C) 2008 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef STATICSPHERESATTRACTIONENGINE_HPP
#define STATICSPHERESATTRACTIONENGINE_HPP 

#include<yade/core/DeusExMachina.hpp>
#include<yade/pkg-common/StaticAttractionEngine.hpp>

class StaticSpheresAttractionEngine : public StaticAttractionEngine
{
	public:
		StaticSpheresAttractionEngine() : max_displacement(0) {};
		Real max_displacement;
	protected :
		virtual Real getMaxDisplacement(MetaBody*);
		virtual bool doesItApplyToThisBody(Body*);
	
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(StaticSpheresAttractionEngine);
	REGISTER_BASE_CLASS_NAME(StaticAttractionEngine);
};

REGISTER_SERIALIZABLE(StaticSpheresAttractionEngine);

#endif 

