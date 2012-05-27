#pragma once

#include<yade/lib/base/Types.hpp>

#include<boost/any.hpp>
#include<boost/version.hpp>
#include<boost/python.hpp>
#include<boost/type_traits.hpp>
#include<boost/preprocessor.hpp>
#include<boost/type_traits/integral_constant.hpp>
#include<yade/lib/factory/Factorable.hpp>
#include<yade/lib/pyutil/raw_constructor.hpp>
#include<yade/lib/pyutil/doc_opts.hpp>
#include<yade/lib/pyutil/except.hpp>

#include<yade/lib/serialization/ObjectIO.hpp>

#include<yade/lib/base/Math.hpp>

#include<yade/lib/serialization/AttrTrait.hpp>

// empty functions for ADL
//namespace{
	template<typename T>	void preLoad(T&){}; template<typename T> void postLoad(T& obj){ /* cerr<<"Generic no-op postLoad("<<typeid(T).name()<<"&) called for "<<obj.getClassName()<<std::endl; */ }
	template<typename T>	void preSave(T&){}; template<typename T> void postSave(T&){}
//};

using namespace yade;

// see:
//		https://bugs.launchpad.net/yade/+bug/539562
// 	http://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/faq.html#topythonconversionfailed
// for reason why the original def_readwrite will not work:
// #define _PYATTR_DEF(x,thisClass,z) .def_readwrite(BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2,0,z)),&thisClass::BOOST_PP_TUPLE_ELEM(2,0,z),BOOST_PP_TUPLE_ELEM(2,1,z))
#define _PYATTR_DEF(x,thisClass,z) _DEF_READWRITE_CUSTOM(thisClass,z)
//
// return reference for vector and matrix types to allow things like
// O.bodies.pos[1].state.vel[2]=0 
// returning value would only change copy of velocity, without propagating back to the original
//
// see http://www.mail-archive.com/yade-dev@lists.launchpad.net/msg03406.html
//
// note that for sequences (like vector<> etc), values are returned; but in case of 
// vector of shared_ptr's, things inside are still shared, so
// O.engines[2].gravity=(0,0,9.33) will work
//
// OTOH got sequences of non-shared types, it sill (silently) fail:
// f=Facet(); f.vertices[1][0]=4 
//
// see http://www.boost.org/doc/libs/1_42_0/libs/type_traits/doc/html/boost_typetraits/background.html
// about how this works
namespace yade{
	// by default, do not return reference; return value instead
	template<typename T> struct py_wrap_ref: public boost::false_type{};
	// specialize for types that should be returned as references
	template<> struct py_wrap_ref<Vector3r>: public boost::true_type{};
	template<> struct py_wrap_ref<Vector3i>: public boost::true_type{};
	template<> struct py_wrap_ref<Vector2r>: public boost::true_type{};
	template<> struct py_wrap_ref<Vector2i>: public boost::true_type{};
	template<> struct py_wrap_ref<Quaternionr>: public boost::true_type{};
	template<> struct py_wrap_ref<Matrix3r>: public boost::true_type{};
	template<> struct py_wrap_ref<MatrixXr>: public boost::true_type{};
	template<> struct py_wrap_ref<Matrix6r>: public boost::true_type{};
	template<> struct py_wrap_ref<Vector6r>: public boost::true_type{};
	template<> struct py_wrap_ref<Vector6i>: public boost::true_type{};
	template<> struct py_wrap_ref<VectorXr>: public boost::true_type{};

	//template<class C, typename T, T C::*A>
	//void make_setter_postLoad(C& instance, const T& val){ instance.*A=val; cerr<<"make_setter_postLoad called"<<endl; postLoad(instance); }
};

// ADL only works within the same namespace
// this duplicate is for classes that are not in yade:: namespace (yet)
template<class C, typename T, T C::*A>
void make_setter_postLoad(C& instance, const T& val){ instance.*A=val; /* cerr<<"make_setter_postLoad called"<<endl; */ instance.callPostLoad(); /* postLoad(instance); */ }
#define _DEF_READWRITE_BY_VALUE(thisClass,attr,doc) add_property(/*attr name*/BOOST_PP_STRINGIZE(attr),/*read access*/py::make_getter(&thisClass::attr,py::return_value_policy<py::return_by_value>()),/*write access*/py::make_setter(&thisClass::attr,py::return_value_policy<py::return_by_value>()),/*docstring*/doc)
// not sure if this is correct: the getter works by value, the setter by reference (the default)...?
#define _DEF_READWRITE_BY_VALUE_POSTLOAD(thisClass,attr,doc) add_property(/*attr name*/BOOST_PP_STRINGIZE(attr),/*read access*/py::make_getter(&thisClass::attr,py::return_value_policy<py::return_by_value>()),/*write access*/ make_setter_postLoad<thisClass,decltype(thisClass::attr),&thisClass::attr>,/*docstring*/doc)
#define _DEF_READONLY_BY_VALUE(thisClass,attr,doc) add_property(/*attr name*/BOOST_PP_STRINGIZE(attr),/*read access*/py::make_getter(&thisClass::attr,py::return_value_policy<py::return_by_value>()),/*docstring*/doc)
/* Huh, add_static_property does not support doc argument (add_property does); if so, use add_property for now at least... */
#define _DEF_READWRITE_BY_VALUE_STATIC(thisClass,attr,doc)  _DEF_READWRITE_BY_VALUE(thisClass,attr,doc)
// the conditional yade::py_wrap_ref should be eliminated by compiler at compile-time, as it depends only on types, not their values
// most of this could be written with templates, including flags (ints can be template args)
#define _DEF_READWRITE_CUSTOM(thisClass,attr) if(!(_ATTR_FLG(attr).isHidden())){ \
	bool _ro(_ATTR_FLG(attr).isReadonly()), _post(_ATTR_FLG(attr).isTriggerPostLoad()), _ref(yade::py_wrap_ref<decltype(thisClass::_ATTR_NAM(attr))>::value || (_ATTR_FLG(attr).isPyByRef())); \
	std::string docStr(_ATTR_DOC(attr)); /*docStr+=" :yattrflags:`"+boost::lexical_cast<string>(_ATTR_FLG(attr))+"` ";*/ \
	if      ( _ref && !_ro && !_post) _classObj.def_readwrite(_ATTR_NAM_STR(attr),&thisClass::_ATTR_NAM(attr),docStr.c_str()); \
	else if ( _ref && !_ro &&  _post) _classObj.add_property(_ATTR_NAM_STR(attr),py::make_getter(&thisClass::_ATTR_NAM(attr)),make_setter_postLoad<thisClass,decltype(thisClass::_ATTR_NAM(attr)),&thisClass::_ATTR_NAM(attr)>,docStr.c_str()); \
	else if ( _ref &&  _ro)           _classObj.def_readonly(_ATTR_NAM_STR(attr),&thisClass::_ATTR_NAM(attr),docStr.c_str()); \
	else if (!_ref && !_ro && !_post) _classObj._DEF_READWRITE_BY_VALUE(thisClass,_ATTR_NAM(attr),docStr.c_str()); \
	else if (!_ref && !_ro &&  _post) _classObj._DEF_READWRITE_BY_VALUE_POSTLOAD(thisClass,_ATTR_NAM(attr),docStr.c_str()); \
	else if (!_ref &&  _ro)           _classObj._DEF_READONLY_BY_VALUE(thisClass,_ATTR_NAM(attr),docStr.c_str()); \
	if(_ro && _post) std::cerr<<"WARN: " BOOST_PP_STRINGIZE(thisClass) "::" _ATTR_NAM_STR(attr) " with the yade::Attr::readonly flag also uselessly sets yade::Attr::triggerPostLoad."<<std::endl; \
}

#define _DEF_READWRITE_CUSTOM_STATIC(thisClass,attr,doc) { /* if(yade::py_wrap_ref<decltype(thisClass::attr)>::value)*/ _classObj.def_readwrite(BOOST_PP_STRINGIZE(attr),&thisClass::attr,doc); /* else _classObj._DEF_READWRITE_BY_VALUE_STATIC(thisClass,attr,doc);*/ }



// macros for deprecated attribute access
// gcc<=4.3 is not able to compile this code; we will just not generate any code for deprecated attributes in such case
#if !defined(__GNUG__) || (defined(__GNUG__) && (__GNUC__ > 4 || (__GNUC__==4 && __GNUC_MINOR__ > 3)))
	// gcc > 4.3 && non-gcc compilers
	#define _PYSET_ATTR_DEPREC(x,thisClass,z) if(key==BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(z))){ _DEPREC_WARN(thisClass,z); _DEPREC_NEWNAME(z)=py::extract<decltype(_DEPREC_NEWNAME(z))>(value); return; }
	#define _PYATTR_DEPREC_DEF(x,thisClass,z) .add_property(BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(z)),&thisClass::BOOST_PP_CAT(_getDeprec_,_DEPREC_OLDNAME(z)),&thisClass::BOOST_PP_CAT(_setDeprec_,_DEPREC_OLDNAME(z)),"|ydeprecated| alias for :yref:`" BOOST_PP_STRINGIZE(_DEPREC_NEWNAME(z)) "<" BOOST_PP_STRINGIZE(thisClass) "." BOOST_PP_STRINGIZE(_DEPREC_NEWNAME(z)) ">` (" _DEPREC_COMMENT(z) ")")
	#define _PYHASKEY_ATTR_DEPREC(x,thisClass,z) if(key==BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(z))) return true;
	/* accessors functions ussing warning */
	#define _ACCESS_DEPREC(x,thisClass,z) /*getter*/ decltype(_DEPREC_NEWNAME(z)) BOOST_PP_CAT(_getDeprec_,_DEPREC_OLDNAME(z))(){_DEPREC_WARN(thisClass,z); return _DEPREC_NEWNAME(z); } /*setter*/ void BOOST_PP_CAT(_setDeprec_,_DEPREC_OLDNAME(z))(const decltype(_DEPREC_NEWNAME(z))& val){_DEPREC_WARN(thisClass,z); _DEPREC_NEWNAME(z)=val; }
#else
	#define _PYSET_ATTR_DEPREC(x,y,z)
	#define _PYATTR_DEPREC_DEF(x,y,z)
	#define _PYHASKEY_ATTR_DEPREC(x,y,z)
	#define _ACCESS_DEPREC(x,y,z)
#endif


// loop bodies for attribute access
#define _PYGET_ATTR(x,y,z) if(key==_ATTR_NAM_STR(z)) return py::object(_ATTR_NAM(z));
//#define _PYSET_ATTR(x,y,z) if(key==_ATTR_NAM_STR(z)) { _ATTR_NAM(z)=py::extract<decltype(_ATTR_NAM(z))>(t[1]); py::delitem(d,py::object(_ATTR_NAM(z))); continue; }
#define _PYSET_ATTR(x,y,z) if(key==_ATTR_NAM_STR(z)) { _ATTR_NAM(z)=py::extract<decltype(_ATTR_NAM(z))>(value); return; }
#define _PYYATTR_ATTR(x,y,z) if(!(_ATTR_FLG(z).isHidden())) ret.append(_ATTR_NAM_STR(z));
#define _PYATTR_TRAIT(x,y,z)        traitList.append(py::ptr(static_cast<AttrTraitBase*>(&_ATTR_TRAIT_GET(z)())));
#define _PYATTR_TRAIT_STATIC(x,y,z) traitList.append(py::ptr(static_cast<AttrTraitBase*>(&_ATTR_TRAIT_GET(z)()))); // static_() already set in trait definition
#define _PYHASKEY_ATTR(x,y,z) if(key==_ATTR_NAM_STR(z)) return true;
#define _PYDICT_ATTR(x,y,z) if(!(_ATTR_FLG(z).isHidden()) && !(_ATTR_FLG(z).isNoSave()) && !(_ATTR_FLG(z).isNoDump())){ /*if(_ATTR_FLG(z) & yade::Attr::pyByRef) ret[_ATTR_NAM_STR(z)]=py::object(boost::ref(_ATTR_NAM(z))); else */  ret[_ATTR_NAM_STR(z)]=py::object(_ATTR_NAM(z)); }
// use the old version, the new one does not work (yet?)
#if 0
	/* template version; generates no code for non-serializable types at all */
	template<bool noSave, class ArchiveT, typename T> struct _SerializeUnlessNoSave{ static void serialize(ArchiveT& ar, const T&, const char* name); };
	template<class ArchiveT, typename T> struct _SerializeUnlessNoSave<true,ArchiveT,T>{ static void serialize(ArchiveT& ar, const T& t, const char* name){}; };
	template<class ArchiveT, typename T> struct _SerializeUnlessNoSave<false,ArchiveT,T>{ static void serialize(ArchiveT& ar, const T& obj, const char* name){ ar & boost::serialization::make_nvp(name,obj); } };
	#define _REGISTER_BOOST_SERIALIZATION_ATTRIBUTES_REPEAT(x,y,z) _SerializeUnlessNoSave<(_ATTR_FLG(z) & yade::Attr::noSave),ArchiveT,decltype(_ATTR_NAM(z))>::serialize(ar,_ATTR_NAM(z), BOOST_PP_STRINGIZE(_ATTR_NAM(z)));
#else
	// generates code for noSave attributes as well, which must therefore be theoretically serializable
	#define _REGISTER_BOOST_SERIALIZATION_ATTRIBUTES_REPEAT(x,y,z) if((_ATTR_FLG(z).isNoSave())==0) { ar & BOOST_SERIALIZATION_NVP(_ATTR_NAM(z)); }
#endif
#define _REGISTER_BOOST_SERIALIZATION_ATTRIBUTES(baseClass,attrs) \
	friend class boost::serialization::access; \
	private: template<class ArchiveT> void serialize(ArchiveT & ar, unsigned int version){ \
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(baseClass);  \
		/* with ADL, either the generic (empty) version above or baseClass::preLoad etc will be called (compile-time resolution) */ \
		if(ArchiveT::is_loading::value) preLoad(*this); else preSave(*this); \
		BOOST_PP_SEQ_FOR_EACH(_REGISTER_BOOST_SERIALIZATION_ATTRIBUTES_REPEAT,~,attrs) \
		if(ArchiveT::is_loading::value) postLoad(*this); else postSave(*this); \
	}

#define _REGISTER_ATTRIBUTES_DEPREC(thisClass,baseClass,attrs,deprec)  _REGISTER_BOOST_SERIALIZATION_ATTRIBUTES(baseClass,attrs) public: \
	void pySetAttr(const std::string& key, const py::object& value){BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR,~,attrs); BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR_DEPREC,thisClass,deprec); baseClass::pySetAttr(key,value); } \
	/* return dictionary of all acttributes and values; deprecated attributes omitted */ py::dict pyDict() const { py::dict ret; BOOST_PP_SEQ_FOR_EACH(_PYDICT_ATTR,~,attrs); ret.update(baseClass::pyDict()); return ret; } \
	virtual void callPostLoad(void){ baseClass::callPostLoad(); postLoad(*this); }

#define _DEF_TRAIT_GETTER(x,thisClass,z) template<class Trait, > 
#define _DEFINE_TRAIT_GETTERS(thisClass,attrs) BOOST_PP_SEQ_FOR_EACH(_DEF_TRAIT_GETTER,thisClass,attrs)



// print warning about deprecated attribute; thisClass is type name, not string
#define _DEPREC_WARN(thisClass,deprec)  std::cerr<<"WARN: "<<getClassName()<<"."<<BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(deprec))<<" is deprecated, use "<<BOOST_PP_STRINGIZE(thisClass)<<"."<<BOOST_PP_STRINGIZE(_DEPREC_NEWNAME(deprec))<<" instead. "; if(_DEPREC_COMMENT(deprec)){ if(std::string(_DEPREC_COMMENT(deprec))[0]=='!'){ std::cerr<<endl; throw std::invalid_argument(BOOST_PP_STRINGIZE(thisClass) "." BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(deprec)) " is deprecated; throwing exception requested. Reason: " _DEPREC_COMMENT(deprec));} else std::cerr<<"("<<_DEPREC_COMMENT(deprec)<<")"; } std::cerr<<std::endl;

// getters for individual fields
#define _ATTR_TYP(s) BOOST_PP_TUPLE_ELEM(5,0,s)
#define _ATTR_NAM(s) BOOST_PP_TUPLE_ELEM(5,1,s)
#define _ATTR_INI(s) BOOST_PP_TUPLE_ELEM(5,2,s)
#define _ATTR_FLG_RAW(s) BOOST_PP_TUPLE_ELEM(5,3,s)
#define _ATTR_TRAIT(s) makeAttrTrait(BOOST_PP_TUPLE_ELEM(5,3,s)).doc(_ATTR_DOC(s)).name(_ATTR_NAM_STR(s)).cxxType(_ATTR_TYP_STR(s)).ini(_ATTR_TYP(s)(_ATTR_INI(s)))
#define _ATTR_TRAIT_TYPE(s) BOOST_PP_CAT(TraitType_,_ATTR_NAM(s))
#define _ATTR_TRAIT_GET(s) BOOST_PP_CAT(getTrait_,_ATTR_NAM(s))
// get flags through AttrTrait now, to test
#define _ATTR_FLG(s) _ATTR_TRAIT(s)
#define _ATTR_DOC(s) BOOST_PP_TUPLE_ELEM(5,4,s)
// stringized getters
#define _ATTR_TYP_STR(s) BOOST_PP_STRINGIZE(_ATTR_TYP(s))
#define _ATTR_NAM_STR(s) BOOST_PP_STRINGIZE(_ATTR_NAM(s))
#define _ATTR_INI_STR(s) BOOST_PP_STRINGIZE(_ATTR_INI(s))
// embed default and type values in the docstring (must keep the same size and ordering)
#define _ATTRS_EMBED_INI_TYP_IN_DOC(x,y,z) (( _ATTR_TYP(z) , _ATTR_NAM(z) , _ATTR_INI(z), _ATTR_FLG(z), _ATTR_DOC(z) " :ydefault:`" _ATTR_INI_STR(z) "`" " :yattrtype:`" _ATTR_TYP_STR(z) "`" ))

// deprecated specification getters
#define _DEPREC_OLDNAME(x) BOOST_PP_TUPLE_ELEM(3,0,x)
#define _DEPREC_NEWNAME(x) BOOST_PP_TUPLE_ELEM(3,1,x)
#define _DEPREC_COMMENT(x) BOOST_PP_TUPLE_ELEM(3,2,x) "" // if the argument is omited, return empty string instead of nothing

#define _PY_REGISTER_CLASS_BODY(thisClass,baseClass,docString,attrs,deprec,extras) \
	checkPyClassRegistersItself(#thisClass); \
	YADE_SET_DOCSTRING_OPTS; \
	py::class_<thisClass,shared_ptr<thisClass>,py::bases<baseClass>,boost::noncopyable> _classObj(#thisClass,docString,/*call raw ctor even for parameterless construction*/py::no_init); \
	_classObj.def("__init__",py::raw_constructor(Serializable_ctor_kwAttrs<thisClass>)); \
	BOOST_PP_SEQ_FOR_EACH(_PYATTR_DEF,thisClass,attrs); \
	(void) _classObj BOOST_PP_SEQ_FOR_EACH(_PYATTR_DEPREC_DEF,thisClass,deprec); \
	(void) _classObj extras ; \
	py::list traitList; BOOST_PP_SEQ_FOR_EACH(_PYATTR_TRAIT,thisClass,attrs); _classObj.attr("_attrTraits")=traitList;\
	Serializable::derivedCxxClasses.push_back(py::object(_classObj));

#define _YADE_CLASS_BASE_DOC_ATTRS_DEPREC_PY(thisClass,baseClass,docString,attrs,deprec,extras) \
	_REGISTER_ATTRIBUTES_DEPREC(thisClass,baseClass,attrs,deprec) \
	REGISTER_CLASS_AND_BASE(thisClass,baseClass) \
	/* accessors for deprecated attributes, with warnings */ BOOST_PP_SEQ_FOR_EACH(_ACCESS_DEPREC,thisClass,deprec) \
	/* python class registration */ virtual void pyRegisterClass() { _PY_REGISTER_CLASS_BODY(thisClass,baseClass,docString,attrs,deprec,extras); } \
	virtual void must_use_both_YADE_CLASS_BASE_DOC_ATTRS_and_YADE_PLUGIN(); // virtual ensures v-table for all classes 

// #define YADE_CLASS_BASE_DOC_ATTRS_PY(thisClass,baseClass,docString,attrs,extras) YADE_CLASS_BASE_DOC_ATTRS_DEPREC_PY(thisClass,baseClass,docString,attrs,,extras)

// return "type name;", define trait type and getter
#define _ATTR_DECL_AND_TRAIT(x,y,z) \
	_ATTR_TYP(z) _ATTR_NAM(z); \
	typedef std::remove_reference<decltype(_ATTR_TRAIT(z))>::type _ATTR_TRAIT_TYPE(z); \
	static _ATTR_TRAIT_TYPE(z)& _ATTR_TRAIT_GET(z)(){ static _ATTR_TRAIT_TYPE(z) _tmp=_ATTR_TRAIT(z); return _tmp; }

// return name(default), (for initializers list); TRICKY: last one must not have the comma
#define _ATTR_MAKE_INITIALIZER(x,maxIndex,i,z) BOOST_PP_TUPLE_ELEM(2,0,z)(BOOST_PP_TUPLE_ELEM(2,1,z)) BOOST_PP_COMMA_IF(BOOST_PP_NOT_EQUAL(maxIndex,i))
#define _ATTR_NAME_ADD_DUMMY_FIELDS(x,y,z) ((/*type*/,z,/*default*/,/*flags*/,/*doc*/))
#define _ATTR_MAKE_INIT_TUPLE(x,y,z) (( _ATTR_NAM(z),_ATTR_INI(z) ))



#define _STAT_NONSTAT_ATTR_PY(thisClass,attr,doc) _DEF_READWRITE_CUSTOM_STATIC(thisClass,attr,doc) /* _DEF_READWRITE_CUSTOM(thisClass,attr,doc) */ /* duplicate static and non-static attributes do not work (they apparently trigger to-python converter being added; for now, make then non-static, that's it. */
#define _STATATTR_PY(x,thisClass,z) _STAT_NONSTAT_ATTR_PY(thisClass,_ATTR_NAM(z),/*docstring*/ "|ystatic| :ydefault:`" _ATTR_INI_STR(z) "` :yattrtype:`" _ATTR_TYP_STR(z) "` " _ATTR_DOC(z))
#define _STATATTR_DECL_AND_TRAIT(x,y,z) \
	static _ATTR_TYP(z) _ATTR_NAM(z); \
	typedef std::remove_reference<decltype(_ATTR_TRAIT(z))>::type _ATTR_TRAIT_TYPE(z); \
	static _ATTR_TRAIT_TYPE(z)& _ATTR_TRAIT_GET(z)(){ static _ATTR_TRAIT_TYPE(z) _tmp=_ATTR_TRAIT(z).static_(); return _tmp; }
#define _STATATTR_INITIALIZE(x,thisClass,z) thisClass::_ATTR_NAM(z)=_ATTR_INI(z);
#define _STATATTR_MAKE_DOC(x,thisClass,z) + ".. ystaticattr:: " BOOST_PP_STRINGIZE(thisClass) "." _ATTR_NAM_STR(z) "(=" _ATTR_INI_STR(z) ")" "\n\n|ystatic| :ydefault:`" _ATTR_INI_STR(z) "` :yattrtype:`" _ATTR_TYP_STR(z) /*"` :yattrflags:`" + boost::lexical_cast<string>(_ATTR_FLG(z)) +*/ "`\n\n\t" _ATTR_DOC(z) "\n\n"


#define _STATCLASS_PY_REGISTER_CLASS(thisClass,baseClass,docString,attrs)\
	virtual void pyRegisterClass() { checkPyClassRegistersItself(#thisClass); initSetStaticAttributesValue(); YADE_SET_DOCSTRING_OPTS; \
		py::class_<thisClass,shared_ptr<thisClass>,py::bases<baseClass>,boost::noncopyable> _classObj(#thisClass,(docString + std::string("\n\n") BOOST_PP_SEQ_FOR_EACH(_STATATTR_MAKE_DOC,thisClass,attrs)).c_str(),/*call raw ctor even for parameterless construction*/py::no_init); _classObj.def("__init__",py::raw_constructor(Serializable_ctor_kwAttrs<thisClass>)); \
		BOOST_PP_SEQ_FOR_EACH(_STATATTR_PY,thisClass,attrs); \
		py::list traitList; BOOST_PP_SEQ_FOR_EACH(_PYATTR_TRAIT_STATIC,thisClass,attrs); _classObj.attr("_attrTraits")=traitList; \
		Serializable::derivedCxxClasses.push_back(py::object(_classObj)); \
	}


/********************** USER MACROS START HERE ********************/

// attrs is (type,name,init-value,docstring)
#define YADE_CLASS_BASE_DOC(klass,base,doc)                             YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,,,,)
#define YADE_CLASS_BASE_DOC_ATTRS(klass,base,doc,attrs)                 YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,,)
#define YADE_CLASS_BASE_DOC_ATTRS_CTOR(klass,base,doc,attrs,ctor)       YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,ctor,)
#define YADE_CLASS_BASE_DOC_ATTRS_PY(klass,base,doc,attrs,py)           YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,,py)
#define YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(klass,base,doc,attrs,ctor,py) YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,ctor,py)
#define YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,inits,ctor,py) YADE_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(klass,base,doc,attrs,,inits,ctor,py)

// the most general
#define YADE_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(thisClass,baseClass,docString,attrs,deprec,inits,ctor,extras) \
	public: BOOST_PP_SEQ_FOR_EACH(_ATTR_DECL_AND_TRAIT,~,attrs) /* attribute declarations */ \
	thisClass() BOOST_PP_IF(BOOST_PP_SEQ_SIZE(inits attrs),:,) BOOST_PP_SEQ_FOR_EACH_I(_ATTR_MAKE_INITIALIZER,BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(inits attrs)), inits BOOST_PP_SEQ_FOR_EACH(_ATTR_MAKE_INIT_TUPLE,~,attrs)) { ctor ; } /* ctor, with initialization of defaults */ \
	_YADE_CLASS_BASE_DOC_ATTRS_DEPREC_PY(thisClass,baseClass,docString,BOOST_PP_SEQ_FOR_EACH(_ATTRS_EMBED_INI_TYP_IN_DOC,~,attrs),deprec,extras)

/** new-style macros **/
// attrs is (type,name,init-value,docstring)
#define YAD3_CLASS_BASE_DOC(klass,base,doc)                             YAD3_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,,,,)
#define YAD3_CLASS_BASE_DOC_ATTRS(klass,base,doc,attrs)                 YAD3_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,,)
#define YAD3_CLASS_BASE_DOC_ATTRS_CTOR(klass,base,doc,attrs,ctor)       YAD3_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,ctor,)
#define YAD3_CLASS_BASE_DOC_ATTRS_PY(klass,base,doc,attrs,py)           YAD3_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,,py)
#define YAD3_CLASS_BASE_DOC_ATTRS_CTOR_PY(klass,base,doc,attrs,ctor,py) YAD3_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,ctor,py)
#define YAD3_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,inits,ctor,py) YAD3_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(klass,base,doc,attrs,,inits,ctor,py)
#define YAD3_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY2(thisClass,baseClass,docString,attrs,deprec,inits,ctor,extras) thisClass,baseClass,docString,attrs,deprec,initrs,ctor,extras

#define YAD3_CLASS_DECLARATION(allArgsTogether) _YAD3_CLASS_DECLARATION(allArgsTogether)

#define _YAD3_CLASS_DECLARATION(thisClass,baseClass,docString,attrs,deprec,inits,ctor,etras) \
	/*class itself*/	REGISTER_CLASS_AND_BASE(thisClass,baseClass) \
	/* attribute declarations*/ BOOST_PP_SEQ_FOR_EACH(_ATTR_DECL_AND_TRAIT,~,attrs) \
	/* boost::serialization, all in header*/ _REGISTER_BOOST_SERIALIZATION_ATTRIBUTES(baseClass,attrs) public: \
	/* later: call postLoad via ADL*/virtual void callPostLoad(void){ baseClass::callPostLoad(); postLoad(*this); } \
	/* accessors for deprecated attributes, with warnings */ BOOST_PP_SEQ_FOR_EACH(_ACCESS_DEPREC,thisClass,deprec) \
	/**follow purce declarations of which implementation is handled sparately**/ \
	/*1. ctor declaration */ thisClass();\
	/*2. set attributes from kw ctor */ void pySetAttr(const std::string& key, const py::object& value); \
	/*3. for pickling*/ py::dict pyDict() const; \
	/*6. python class registration*/ virtual void pyRegisterClass(); \
	/*7. ensures v-table; will be removed later*/virtual void must_use_both_YADE_CLASS_BASE_DOC_ATTRS_and_YADE_PLUGIN();

#define YAD3_CLASS_IMPLEMENTATION(allArgsTogether) _YAD3_CLASS_IMPLEMENTATION(allArgsTogether)

#define _YAD3_CLASS_IMPLEMENTATION(thisClass,baseClass,docString,attrs,deprec,init,ctor,extras) \
	/*1.*/ thisClass::thisClass() BOOST_PP_IF(BOOST_PP_SEQ_SIZE(init attrs),:,) BOOST_PP_SEQ_FOR_EACH_I(_ATTR_MAKE_INITIALIZER,BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(init attrs)), init BOOST_PP_SEQ_FOR_EACH(_ATTR_MAKE_INIT_TUPLE,~,attrs)) { ctor; } \
	/*2.*/ void thisClass::pySetAttr(const std::string& key, const py::object& value){ BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR,~,attrs); BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR_DEPREC,thisClass,deprec); baseClass::pySetAttr(key,value); } \
	/*3.*/ py::dict thisClass::pyDict() const { py::dict ret; BOOST_PP_SEQ_FOR_EACH(_PYDICT_ATTR,~,attrs); ret.update(baseClass::pyDict()); return ret; } \
	/*6.*/ void thisClass::pyRegisterClass() { _PY_REGISTER_CLASS_BODY(thisClass,baseClass,docString,attrs,deprec,extras); } \
	/*7.*/ /*void thisClass::must_use_both_YADE_CLASS_BASE_DOC_ATTRS_and_YADE_PLUGIN();*/

	


/** static attrs **/

// for static classes (Gl1 functors, for instance)
#define YADE_CLASS_BASE_DOC_STATICATTRS(thisClass,baseClass,docString,attrs)\
	public: BOOST_PP_SEQ_FOR_EACH(_STATATTR_DECL_AND_TRAIT,~,attrs) /* attribute declarations */ \
	/* no ctor */ \
	REGISTER_CLASS_AND_BASE(thisClass,baseClass); \
	_REGISTER_ATTRIBUTES_DEPREC(thisClass,baseClass,attrs,) \
	/* called only at class registration, to set initial values; storage still has to be alocated in the cpp file! */ \
	void initSetStaticAttributesValue(void){ BOOST_PP_SEQ_FOR_EACH(_STATATTR_INITIALIZE,thisClass,attrs); } \
	_STATCLASS_PY_REGISTER_CLASS(thisClass,baseClass,docString,attrs) \
	void must_use_both_YADE_CLASS_BASE_DOC_ATTRS_and_YADE_PLUGIN(); 


// used only in some exceptional cases, might disappear in the future
#define REGISTER_ATTRIBUTES(baseClass,attrs) _REGISTER_ATTRIBUTES_DEPREC(_SOME_CLASS,baseClass,BOOST_PP_SEQ_FOR_EACH(_ATTR_NAME_ADD_DUMMY_FIELDS,~,attrs),)

// see https://bugs.launchpad.net/yade/+bug/666876
// we have to change things at a few other places as well
#if BOOST_VERSION>=104200
	#define REGISTER_SERIALIZABLE(name) BOOST_CLASS_EXPORT_KEY(name);
#else
	#define REGISTER_SERIALIZABLE(name) 
#endif


struct Serializable: public Factorable, public boost::noncopyable {
	// http://www.boost.org/doc/libs/1_49_0/libs/smart_ptr/sp_techniques.html#static
	struct null_deleter{void operator()(void const *)const{}};
	static vector<py::object> derivedCxxClasses;
	static py::list getDerivedCxxClasses();
	// this is only for informative purposes, hence the typecast is OK
	long pyCxxAddr() const{ return (long)this; }

		template <class ArchiveT> void serialize(ArchiveT & ar, unsigned int version){ };
		// lovely cast members like in eigen :-)
		template <class DerivedT> const DerivedT& cast() const { return *static_cast<DerivedT*>(this); }
		template <class DerivedT> DerivedT& cast(){ return *static_cast<DerivedT*>(this); }

		static shared_ptr<Serializable> boostLoad(const string& f){ auto obj=make_shared<Serializable>(); ObjectIO::load(f,"yade__Serializable",obj); return obj; }
		virtual void boostSave(const string& f){ shared_ptr<Serializable> thisPtr(this,null_deleter()); ObjectIO::save(f,"yade__Serializable",thisPtr); }
		//template<class DerivedT> shared_ptr<DerivedT> _cxxLoadChecked(const string& f){ auto obj=_cxxLoad(f); auto obj2=dynamic_pointer_cast<DerivedT>(obj); if(!obj2) throw std::runtime_error("Loaded type "+obj->getClassName()+" could not be cast to requested type "+DerivedT::getClassNameStatic()); }

		Serializable() {};
		virtual ~Serializable() {};
		// comparison of strong equality of 2 objects (by their address)
		bool operator==(const Serializable& other) const { return this==&other; }
		bool operator!=(const Serializable& other) const { return this!=&other; }

		void pyUpdateAttrs(const py::dict& d);
		//static void pyUpdateAttrs(const shared_ptr<Serializable>&, const py::dict& d);

		virtual void pySetAttr(const std::string& key, const py::object& value){ yade::AttributeError("No such attribute: "+key+".");};
		virtual py::dict pyDict() const { return py::dict(); }
		virtual void callPostLoad(void){ postLoad(*this); }
		// check whether the class registers itself or whether it calls virtual function of some base class;
		// that means that the class doesn't register itself properly
		virtual void checkPyClassRegistersItself(const std::string& thisClassName) const;
		// perform class registration; overridden in all classes
		virtual void pyRegisterClass();
		// perform any manipulation of arbitrary constructor arguments coming from python, manipulating them in-place;
		// the remainder is passed to the Serializable_ctor_kwAttrs of the respective class (note: args must be empty)
		virtual void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){ return; }
		
		//! string representation of this object
		virtual std::string pyStr() const { return "<"+getClassName()+" @ "+boost::lexical_cast<string>(this)+">"; }

	REGISTER_CLASS_NAME(Serializable);
	REGISTER_BASE_CLASS_NAME(Factorable);
};

// helper functions
template <typename T>
shared_ptr<T> Serializable_ctor_kwAttrs(py::tuple& t, py::dict& d){
	shared_ptr<T> instance=make_shared<T>();
	instance->pyHandleCustomCtorArgs(t,d); // can change t and d in-place
	if(py::len(t)>0) throw std::runtime_error("Zero (not "+boost::lexical_cast<string>(py::len(t))+") non-keyword constructor arguments required [in Serializable_ctor_kwAttrs; Serializable::pyHandleCustomCtorArgs might had changed it after your call].");
	if(py::len(d)>0) instance->pyUpdateAttrs(d);
	instance->callPostLoad(); 
	return instance;
}
