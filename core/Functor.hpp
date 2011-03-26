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

#include<yade/lib/serialization/Serializable.hpp>
#include<yade/lib/multimethods/FunctorWrapper.hpp>
#include<yade/core/Omega.hpp>


class TimingDeltas;
class Scene;

class Functor: public Serializable
{
	public: virtual vector<std::string> getFunctorTypes(){throw;}
	shared_ptr<TimingDeltas> timingDeltas;
	//! updated before every dispatch loop by the dispatcher; DO NOT ABUSE access to scene, except for getting global variables like scene->dt.
	Scene* scene;
	virtual ~Functor(); // defined in Dispatcher.cpp
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Functor,Serializable,"Function-like object that is called by Dispatcher, if types of arguments match those the Functor declares to accept.",
		((string,label,,,"Textual label for this object; must be valid python identifier, you can refer to it directly fron python (must be a valid python identifier).")),
		/*ctor*/ scene=Omega::instance().getScene().get() ,
		.def_readonly("timingDeltas",&Functor::timingDeltas,"Detailed information about timing inside the Dispatcher itself. Empty unless enabled in the source code and O.timingEnabled==True.")
		.add_property("bases",&Functor::getFunctorTypes,"Ordered list of types (as strings) this functor accepts.")
	);	
};
REGISTER_SERIALIZABLE(Functor);



template
<
	class _DispatchType1,
	class _ReturnType,
	class _ArgumentTypes
>
class Functor1D: public Functor,
			public FunctorWrapper<_ReturnType, _ArgumentTypes>
{
	public:
		typedef _DispatchType1 DispatchType1; typedef _ReturnType ReturnType; typedef _ArgumentTypes ArgumentTypes; 
		#define FUNCTOR1D(type1) public: std::string get1DFunctorType1(void){return string(#type1);} int checkArgTypes(const shared_ptr<DispatchType1>& arg1){ return (bool)dynamic_pointer_cast<type1>(arg1)?1:0; }
		virtual std::string get1DFunctorType1(void){throw runtime_error("Class "+this->getClassName()+" did not use FUNCTOR1D to declare its argument type?"); }
		virtual vector<string> getFunctorTypes(void){vector<string> ret; ret.push_back(get1DFunctorType1()); return ret;};
		// check that the object can be correctly cast to the derived class handled by the functor (will be used if ever utils.createInteraction can be called with list of functors only)
		// virtual bool checkArgTypes(const shared_ptr<DispatchType1>& arg1){ throw runtime_error("Class "+this->getClassName()+" did not use FUNCTOR1D to declare its argument type?"); }
	REGISTER_CLASS_AND_BASE(Functor1D,Functor FunctorWrapper);
	/* do not REGISTER_ATTRIBUTES here, since we are template; derived classes should call REGISTER_ATTRIBUTES(Functor,(their)(own)(attributes)), bypassing Functor1D */
};


template
<
	class _DispatchType1,
	class _DispatchType2,
	class _ReturnType,
	class _ArgumentTypes
>
class Functor2D:	public Functor,
			public FunctorWrapper<_ReturnType, _ArgumentTypes>
{
	public:
		typedef _DispatchType1 DispatchType1; typedef _DispatchType2 DispatchType2; typedef _ReturnType ReturnType; typedef _ArgumentTypes ArgumentTypes; 
		#define FUNCTOR2D(type1,type2) public: std::string get2DFunctorType1(void){return string(#type1);}; std::string get2DFunctorType2(void){return string(#type2);}; int checkArgTypes(const shared_ptr<DispatchType1>& arg1, const shared_ptr<DispatchType2>& arg2){ if(dynamic_pointer_cast<type1>(arg1)&&dynamic_pointer_cast<type2>(arg2)) return 1; if(dynamic_pointer_cast<type1>(arg2)&&dynamic_pointer_cast<type2>(arg1)) return -1; return 0; }
		virtual std::string get2DFunctorType1(void){throw logic_error("Class "+this->getClassName()+" did not use FUNCTOR2D to declare its argument types?");}
		virtual std::string get2DFunctorType2(void){throw logic_error("Class "+this->getClassName()+" did not use FUNCTOR2D to declare its argument types?");}
		virtual vector<string> getFunctorTypes(){vector<string> ret; ret.push_back(get2DFunctorType1()); ret.push_back(get2DFunctorType2()); return ret;};
		// check that objects can be correctly cast to derived classes handled by the functor (see comment in Functor1D:: checkArgTypes)
		// virtual bool checkArgTypes(const shared_ptr<DispatchType1>&, const shared_ptr<DispatchType2>&){ throw logic_error("Class "+this->getClassName()+" did not use FUNCTOR2D to declare its argument types?"); }
	REGISTER_CLASS_AND_BASE(Functor2D,Functor FunctorWrapper);
	/* do not REGISTER_ATTRIBUTES here, since we are template; derived classes should call REGISTER_ATTRIBUTES(Functor,(their)(own)(attributes)), bypassing Functor2D */
};



