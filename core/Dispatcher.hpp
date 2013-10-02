/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<boost/lexical_cast.hpp>

#include<woo/core/Master.hpp>
#include<woo/core/Engine.hpp>
#include<woo/core/Functor.hpp>
#include<woo/lib/multimethods/DynLibDispatcher.hpp>

#include<boost/preprocessor/cat.hpp>

// real base class for all dispatchers (the other one are templates)
class Dispatcher: public Engine{
	public:
	// these functions look to be completely unused...?
	virtual string getFunctorType() { throw; };
	virtual int getDimension() { throw; };
	virtual string getBaseClassType(unsigned int ) { throw; };
	//
	virtual ~Dispatcher();
	WOO_CLASS_BASE_DOC(Dispatcher,Engine,"Engine dispatching control to its associated functors, based on types of argument it receives. This abstract base class provides no functionality in itself.")
};
WOO_REGISTER_OBJECT(Dispatcher);

/* Each real dispatcher derives from Dispatcher1D or Dispatcher2D (both templates), which in turn derive from Dispatcher (an Engine) and DynLibDispatcher (the dispatch logic).
Because we need literal functor and class names for registration in python, we provide macro that creates the real dispatcher class with everything needed.
*/

#define _YADE_DISPATCHER1D_FUNCTOR_ADD(FunctorT,f) virtual void addFunctor(shared_ptr<FunctorT> f){ add1DEntry(f); }
#define _YADE_DISPATCHER2D_FUNCTOR_ADD(FunctorT,f) virtual void addFunctor(shared_ptr<FunctorT> f){ add2DEntry(f); }

#define _YADE_DIM_DISPATCHER_FUNCTOR_DOC_ATTRS_CTOR_PY(Dim,DispatcherT,FunctorT,doc,attrs,ctor,ppy) \
	typedef FunctorT FunctorType; \
	void updateScenePtr(){ for(const shared_ptr<FunctorT>& f: functors){ f->scene=scene; f->field=field.get(); }} \
	void postLoad(DispatcherT&, void* addr){ clearMatrix(); for(shared_ptr<FunctorT> f: functors) add(static_pointer_cast<FunctorT>(f)); } \
	virtual void add(FunctorT* f){ add(shared_ptr<FunctorT>(f)); } \
	virtual void add(shared_ptr<FunctorT> f){ bool dupe=false; string fn=f->getClassName(); for(const shared_ptr<FunctorT>& f: functors) { if(fn==f->getClassName()) dupe=true; } if(!dupe) functors.push_back(f); addFunctor(f); } \
	BOOST_PP_CAT(_YADE_DISPATCHER,BOOST_PP_CAT(Dim,D_FUNCTOR_ADD))(FunctorT,f) \
	py::list functors_get(void) const { py::list ret; for(const shared_ptr<FunctorT>& f: functors){ ret.append(f); } return ret; } \
	virtual void getLabeledObjects(const shared_ptr<LabelMapper>& labelMapper){ for(const shared_ptr<FunctorT>& f: functors){ Engine::handlePossiblyLabeledObject(f,labelMapper); } } \
	void functors_set(const vector<shared_ptr<FunctorT> >& ff){ functors.clear(); for(const shared_ptr<FunctorT>& f: ff) add(f); postLoad(*this,NULL); } \
	void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){ if(py::len(t)==0)return; if(py::len(t)!=1) throw invalid_argument("Exactly one list of " BOOST_PP_STRINGIZE(FunctorT) " must be given."); typedef std::vector<shared_ptr<FunctorT> > vecF; vecF vf=py::extract<vecF>(t[0])(); functors_set(vf); t=py::tuple(); } \
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(DispatcherT,Dispatcher,"Dispatcher calling :obj:`functors<" BOOST_PP_STRINGIZE(FunctorT) ">` based on received argument type(s).\n\n" doc, \
		((vector<shared_ptr<FunctorT> >,functors,,,"Functors active in the dispatch mechanism [overridden below].")) /*additional attrs*/ attrs, \
		/*ctor*/ ctor, /*py*/ ppy .add_property("functors",&DispatcherT::functors_get,&DispatcherT::functors_set,"Functors associated with this dispatcher (list of :obj:`" BOOST_PP_STRINGIZE(FunctorT) "`)") \
		.def("dispMatrix",&DispatcherT::dump,py::arg("names")=true,"Return dictionary with contents of the dispatch matrix.").def("dispFunctor",&DispatcherT::getFunctor,"Return functor that would be dispatched for given argument(s); None if no dispatch; ambiguous dispatch throws."); \
	)

#define WOO_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(DispatcherT,FunctorT,doc,attrs,ctor,py) _YADE_DIM_DISPATCHER_FUNCTOR_DOC_ATTRS_CTOR_PY(1,DispatcherT,FunctorT,doc,attrs,ctor,py)
#define WOO_DISPATCHER2D_FUNCTOR_DOC_ATTRS_CTOR_PY(DispatcherT,FunctorT,doc,attrs,ctor,py) _YADE_DIM_DISPATCHER_FUNCTOR_DOC_ATTRS_CTOR_PY(2,DispatcherT,FunctorT,doc,attrs,ctor,py)

// HELPER FUNCTIONS

//! Return functors of this dispatcher, as list of functors of appropriate type
template<typename DispatcherT>
std::vector<shared_ptr<typename DispatcherT::functorType> > Dispatcher_functors_get(shared_ptr<DispatcherT> self){
	std::vector<shared_ptr<typename DispatcherT::functorType> > ret;
	for(const shared_ptr<Functor>& functor: self->functors){ shared_ptr<typename DispatcherT::functorType> functorRightType(dynamic_pointer_cast<typename DispatcherT::functorType>(functor)); if(!functorRightType) throw logic_error("Internal error: Dispatcher of type "+self->getClassName()+" did not contain Functor of the required type "+typeid(typename DispatcherT::functorType).name()+"?"); ret.push_back(functorRightType); }
	return ret;
}

template<typename DispatcherT>
void Dispatcher_functors_set(shared_ptr<DispatcherT> self, std::vector<shared_ptr<typename DispatcherT::functorType> > functors){
	self->clear();
	for(const shared_ptr<typename DispatcherT::functorType>& item: functors) self->add(item);
}

// Dispatcher is not a template, hence converting this into a real constructor would be complicated; keep it separated, at least for now...
//! Create dispatcher of given type, with functors given as list in argument
template<typename DispatcherT>
shared_ptr<DispatcherT> Dispatcher_ctor_list(const std::vector<shared_ptr<typename DispatcherT::functorType> >& functors){
	shared_ptr<DispatcherT> instance(new DispatcherT);
	Dispatcher_functors_set<DispatcherT>(instance,functors);
	return instance;
}



template
<
	class FunctorType,
	bool autoSymmetry=true
>
class Dispatcher1D : public Dispatcher,
				public DynLibDispatcher
				<	  TYPELIST_1(typename FunctorType::DispatchType1)		// base classes for dispatch
					, FunctorType		// class that provides multivirtual call
					, typename FunctorType::ReturnType		// return type
					, typename FunctorType::ArgumentTypes
					, autoSymmetry
				>
{

	public :
		typedef typename FunctorType::DispatchType1 baseClass;
		typedef baseClass argType1;
		typedef FunctorType functorType;
		typedef DynLibDispatcher<TYPELIST_1(baseClass),FunctorType,typename FunctorType::ReturnType,typename FunctorType::ArgumentTypes,autoSymmetry> dispatcherBase;

		shared_ptr<FunctorType> getFunctor(shared_ptr<baseClass> arg){ return dispatcherBase::getExecutor(arg); }
		py::dict dump(bool convertIndicesToNames){
			py::dict ret;
			for(const DynLibDispatcher_Item1D& item: dispatcherBase::dataDispatchMatrix1D()){
				if(convertIndicesToNames){
					string arg1=Dispatcher_indexToClassName<argType1>(item.ix1);
					ret[py::make_tuple(arg1)]=item.functorName;
				} else ret[py::make_tuple(item.ix1)]=item.functorName;
			}
			return ret;
		}

		int getDimension() { return 1; }
	
		virtual string getFunctorType(){
			shared_ptr<FunctorType> eu(new FunctorType);
			return eu->getClassName();
		}

		virtual string getBaseClassType(unsigned int i){
			if (i==0) { shared_ptr<baseClass> bc(new baseClass); return bc->getClassName(); }
			else return "";
		}


	public:
	REGISTER_ATTRIBUTES(Dispatcher,);
	REGISTER_CLASS_AND_BASE(Dispatcher1D,Dispatcher DynLibDispatcher);
};


template
<
	class FunctorType,
	bool autoSymmetry=true
>
class Dispatcher2D : public Dispatcher,
				public DynLibDispatcher
				<	  TYPELIST_2(typename FunctorType::DispatchType1,typename FunctorType::DispatchType2) // base classes for dispatch
					, FunctorType			// class that provides multivirtual call
					, typename FunctorType::ReturnType    // return type
					, typename FunctorType::ArgumentTypes // argument of engine unit
					, autoSymmetry
				>
{
	public :
		typedef typename FunctorType::DispatchType1 baseClass1; typedef typename FunctorType::DispatchType2 baseClass2;
		typedef baseClass1 argType1;
		typedef baseClass2 argType2;
		typedef FunctorType functorType;
		typedef DynLibDispatcher<TYPELIST_2(baseClass1,baseClass2),FunctorType,typename FunctorType::ReturnType,typename FunctorType::ArgumentTypes,autoSymmetry> dispatcherBase;
		shared_ptr<FunctorType> getFunctor(shared_ptr<baseClass1> arg1, shared_ptr<baseClass2> arg2){ return dispatcherBase::getExecutor(arg1,arg2); }
		py::dict dump(bool convertIndicesToNames){
			py::dict ret;
			for(const DynLibDispatcher_Item2D& item: dispatcherBase::dataDispatchMatrix2D()){
				if(convertIndicesToNames){
					string arg1=Dispatcher_indexToClassName<argType1>(item.ix1), arg2=Dispatcher_indexToClassName<argType2>(item.ix2);
					ret[py::make_tuple(arg1,arg2)]=item.functorName;
				} else ret[py::make_tuple(item.ix1,item.ix2)]=item.functorName;
			}
			return ret;
		}

		virtual int getDimension() { return 2; }

		virtual string getFunctorType(){
			shared_ptr<FunctorType> eu(new FunctorType);
			return eu->getClassName();
		}
		virtual string getBaseClassType(unsigned int i){
			if (i==0){ shared_ptr<baseClass1> bc(new baseClass1); return bc->getClassName(); }
			else if (i==1){ shared_ptr<baseClass2> bc(new baseClass2); return bc->getClassName();}
			else return "";
		}
	public:
	REGISTER_ATTRIBUTES(Dispatcher,);
	REGISTER_CLASS_AND_BASE(Dispatcher2D,Dispatcher DynLibDispatcher);
};

