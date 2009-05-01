/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include "EngineUnit.hpp"

#include<yade/lib-multimethods/FunctorWrapper.hpp>

#define FUNCTOR2D(type1,type2) public: std::string get2DFunctorType1(void){return string(#type1);}; std::string get2DFunctorType2(void){return string(#type2);};


template
<
	class ReturnType,
	class AttributesType
>
class EngineUnit2D :	public EngineUnit,
			public FunctorWrapper<ReturnType, AttributesType>
{
	public:
		virtual std::string get2DFunctorType1(void){throw runtime_error("Class "+this->getClassName()+" did not use FUNCTOR2D to declare its argument types?");}
		virtual std::string get2DFunctorType2(void){throw runtime_error("Class "+this->getClassName()+" did not use FUNCTOR2D to declare its argument types?");}
		virtual vector<string> getFunctorTypes(){vector<string> ret; ret.push_back(get2DFunctorType1()); ret.push_back(get2DFunctorType2()); return ret;};
	REGISTER_CLASS_AND_BASE(EngineUnit2D,EngineUnit FunctorWrapper);
	/* do not REGISTER_ATTRIBUTES here, since we are template; derived classes should call REGISTER_ATTRIBUTES(EngineUnit,(their)(own)(attributes)), bypassing EngineUnit2D */
};


