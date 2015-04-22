#pragma once

#include<woo/lib/base/Types.hpp>

#include<boost/any.hpp>
#include<boost/version.hpp>
#include<boost/python.hpp>
#include<boost/type_traits.hpp>
#include<boost/preprocessor.hpp>
#include<boost/type_traits/integral_constant.hpp>
#include<boost/enable_shared_from_this.hpp>
#include<woo/lib/pyutil/raw_constructor.hpp>
#include<woo/lib/pyutil/doc_opts.hpp>
#include<woo/lib/pyutil/except.hpp>
#include<woo/lib/pyutil/pickle.hpp>

#include<boost/preprocessor.hpp>
#include<boost/version.hpp>
#include<boost/archive/binary_oarchive.hpp>
#include<boost/archive/binary_iarchive.hpp>
#ifndef WOO_NOXML
	#include<boost/archive/xml_oarchive.hpp>
	#include<boost/archive/xml_iarchive.hpp>
	// declare supported archive types so that we can declare the templates explcitily in headers
	// and specialize them explicitly in implementation files
	#define WOO_BOOST_ARCHIVES (boost::archive::binary_iarchive)(boost::archive::binary_oarchive)(boost::archive::xml_iarchive)(boost::archive::xml_oarchive)
#else
	#define WOO_BOOST_ARCHIVES (boost::archive::binary_iarchive)(boost::archive::binary_oarchive)
#endif

#include<boost/serialization/export.hpp> // must come after all supported archive types

#include<boost/serialization/base_object.hpp>
#include<boost/serialization/shared_ptr.hpp>
#include<boost/serialization/weak_ptr.hpp>
#include<boost/serialization/list.hpp>
#include<boost/serialization/vector.hpp>
#include<boost/serialization/map.hpp>
#include<boost/serialization/set.hpp>
#include<boost/serialization/nvp.hpp>


#include<woo/lib/object/ObjectIO.hpp>
#include<woo/lib/base/Math.hpp>
#include<woo/lib/base/Logging.hpp>
#include<woo/lib/object/AttrTrait.hpp>


/*! Macro defining what classes can be found in this plugin -- must always be used in the respective .cpp file.
 * A function registerThisPluginClasses_FirstPluginName is generated at every occurence. The identifier should
 * be unique and avoids use of __COUNTER__ which didn't appear in gcc until 4.3.
 */
#if BOOST_VERSION<104200
 	#error Boost >= 1.42 is required
#endif

#define _WOO_PLUGIN_BOOST_REGISTER(x,y,z) BOOST_CLASS_EXPORT_IMPLEMENT(z); BOOST_SERIALIZATION_FACTORY_0(z);
#define WOO_REGISTER_OBJECT(name) BOOST_CLASS_EXPORT_KEY(name);

// the __attribute__((constructor(priority))) construct not supported before gcc 4.3
// it will only produce warning from log4cxx if not used
#if __GNUC__ == 4 && __GNUC_MINOR__ >=3
	#define WOO_CTOR_PRIORITY(p) (p)
#else
	#define WOO_CTOR_PRIORITY(p)
#endif
#define _PLUGIN_CHECK_REPEAT(x,y,z) void z::must_use_both_WOO_CLASS_BASE_DOC_ATTRS_and_WOO_PLUGIN(){}
#define _WOO_PLUGIN_REPEAT(x,y,z) BOOST_PP_STRINGIZE(z),
#define _WOO_FACTORY_REPEAT(x,y,z) __attribute__((unused)) bool BOOST_PP_CAT(_registered,z)=Master::instance().registerClassFactory(BOOST_PP_STRINGIZE(z),(Master::FactoryFunc)([](void)->shared_ptr<woo::Object>{ return make_shared<z>(); }));
// priority 500 is greater than priority for log4cxx initialization (in core/main/pyboot.cpp); therefore lo5cxx will be initialized before plugins are registered
#define WOO__ATTRIBUTE__CONSTRUCTOR __attribute__((constructor))

#define WOO_PLUGIN(module,plugins) BOOST_PP_SEQ_FOR_EACH(_WOO_FACTORY_REPEAT,~,plugins); namespace{ WOO__ATTRIBUTE__CONSTRUCTOR void BOOST_PP_CAT(registerThisPluginClasses_,BOOST_PP_SEQ_HEAD(plugins)) (void){ LOG_DEBUG_EARLY("Registering classes in "<<__FILE__); const char* info[]={__FILE__ , BOOST_PP_SEQ_FOR_EACH(_WOO_PLUGIN_REPEAT,~,plugins) NULL}; Master::instance().registerPluginClasses(BOOST_PP_STRINGIZE(module),info);} } BOOST_PP_SEQ_FOR_EACH(_WOO_PLUGIN_BOOST_REGISTER,~,plugins) BOOST_PP_SEQ_FOR_EACH(_PLUGIN_CHECK_REPEAT,~,plugins)




// empty functions for ADL
//namespace{
	template<typename T>	void preLoad(T&){}; template<typename T> void postLoad(T& obj, void* addr){ /* cerr<<"Generic no-op postLoad("<<typeid(T).name()<<"&) called for "<<obj.getClassName()<<std::endl; */ }
	template<typename T>	void preSave(T&){}; template<typename T> void postSave(T&){}
//};

using namespace woo;

// see:
//		https://bugs.launchpad.net/woo/+bug/539562
// 	http://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/faq.html#topythonconversionfailed
// for reason why the original def_readwrite will not work:
// #define _PYATTR_DEF(x,thisClass,z) .def_readwrite(BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2,0,z)),&thisClass::BOOST_PP_TUPLE_ELEM(2,0,z),BOOST_PP_TUPLE_ELEM(2,1,z))
#define _PYATTR_DEF(x,thisClass,z) _DEF_READWRITE_CUSTOM(thisClass,z)
//
// return reference for vector and matrix types to allow things like
// O.bodies.pos[1].state.vel[2]=0 
// returning value would only change copy of velocity, without propagating back to the original
//
// see http://www.mail-archive.com/woo-dev@lists.launchpad.net/msg03406.html
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
namespace woo{
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
// this duplicate is for classes that are not in woo:: namespace (yet)
template<class C, typename T, T C::*A>
void make_setter_postLoad(C& instance, const T& val){ instance.*A=val; /* cerr<<"make_setter_postLoad called"<<endl; */ instance.callPostLoad((void*)&(instance.*A)); /* postLoad(instance,(void*)&(instance.*A)); */ }

template<bool integral> struct _register_bit_accessors_if_integral;
// do-nothing variant
template<> struct _register_bit_accessors_if_integral<false> {
	template<typename classObjT, typename classT, typename attrT, attrT classT::*A>
	static void call(classObjT& _classObj, const vector<string>& bits, bool ro){ };
};
// register bits variant
template<> struct _register_bit_accessors_if_integral<true> {
	template<typename classObjT, typename classT, typename attrT, attrT classT::*A>
	static void call(classObjT& _classObj, const vector<string>& bits, bool ro){
		for(size_t i=0; i<bits.size(); i++){
			auto getter=py::detail::make_function_aux([i](const classT& obj){ return bool(obj.*A & (1<<i)); },py::default_call_policies(),boost::mpl::vector<bool,classT>());
			auto setter=py::detail::make_function_aux([i](classT& obj, bool val){ if(val) obj.*A|=(1<<i); else obj.*A&=~(1<<i); },py::default_call_policies(),boost::mpl::vector<void,classT,bool>());
			if(ro) _classObj.add_property(bits[i].c_str(),getter);
			else   _classObj.add_property(bits[i].c_str(),getter,setter);
		}
	}
};

template<bool namedEnum> struct  _def_woo_attr__namedEnum{};
/* instantiation for attribute which IS NOT not a named enumeration */
template<> struct _def_woo_attr__namedEnum<false>{
	template<typename classObjT, typename traitT, typename classT, typename attrT, attrT classT::*A>
	void wooDef(classObjT& _classObj, traitT& trait, const char* className, const char *attrName){
		bool _ro=trait.isReadonly(), _post=trait.isTriggerPostLoad(), _ref(!_ro && (woo::py_wrap_ref<attrT>::value || trait.isPyByRef()));
		const char* docStr(trait._doc.c_str());
		if      ( _ref && !_ro && !_post) _classObj.def_readwrite(attrName,A,docStr);
		else if ( _ref && !_ro &&  _post) _classObj.add_property(attrName,py::make_getter(A),make_setter_postLoad<classT,attrT,A>,docStr);
		else if ( _ref &&  _ro)           _classObj.def_readonly(attrName,A,docStr);
		else if (!_ref && !_ro && !_post) _classObj.add_property(attrName,py::make_getter(A,py::return_value_policy<py::return_by_value>()),py::make_setter(A,py::return_value_policy<py::return_by_value>()),docStr);
		else if (!_ref && !_ro &&  _post) _classObj.add_property(attrName,py::make_getter(A,py::return_value_policy<py::return_by_value>()),make_setter_postLoad<classT,attrT,A>,docStr);
		else if (!_ref &&  _ro)           _classObj.add_property(attrName,py::make_getter(A,py::return_value_policy<py::return_by_value>()),docStr);
		if(_ro && _post) cerr<<"WARN: "<<className<<"::"<<attrName<<" with the woo::Attr::readonly flag also uselessly sets woo::Attr::triggerPostLoad."<<endl;
		if(!trait._bits.empty()) _register_bit_accessors_if_integral<std::is_integral<attrT>::value>::template call<classObjT,classT,attrT,A>(_classObj,trait._bits,_ro && (!trait._bitsRw));
	}
};

/* instantiation for attribute which IS a named enumeration */
// FIXME: this works but does not look pretty
// see the comment of http://stackoverflow.com/a/25281985/761090
// https://mail.python.org/pipermail/cplusplus-sig/2009-February/014263.html documents the current solution
template<> struct _def_woo_attr__namedEnum<true>{
	template<typename classObjT, typename traitT, typename classT, typename attrT, attrT classT::*A>
	void wooDef(classObjT& _classObj, traitT& trait, const char* className, const char *attrName){
		bool _ro=trait.isReadonly(), _post=trait.isTriggerPostLoad();
		const char* docStr(trait._doc.c_str());
		#if 0
			if (_ro) _classObj.add_property(attrName,std::function<string(const classT&)>([=](const classT& obj)->string{ return trait.namedEnum_num2name(obj.*A); }),docStr);
			else if(!_ro && !_post) _classObj.add_property(attrName,std::function<string(const classT&)>([=](const classT& obj)->string{ return trait.namedEnum_num2name(obj.*A); }),std::function<void(classT&, py::object)>([=](classT& obj, py::object val){ obj.*A=trait.namedEnum_name2num(val); }),docStr);
		#endif
		auto getter=py::detail::make_function_aux([trait](const classT& obj){ return trait.namedEnum_num2name(obj.*A); },py::default_call_policies(),boost::mpl::vector<string,classT>());
		auto setter=py::detail::make_function_aux([trait](classT& obj, py::object val){ obj.*A=trait.namedEnum_name2num(val);},py::default_call_policies(),boost::mpl::vector<void,classT,py::object>());
		auto setterPostLoad=py::detail::make_function_aux([trait](classT& obj, py::object val){ obj.*A=trait.namedEnum_name2num(val); obj.callPostLoad((void*)&(obj.*A)); },py::default_call_policies(),boost::mpl::vector<void,classT,py::object>());
		if (_ro)                _classObj.add_property(attrName,getter,docStr);
		else if(!_ro && !_post) _classObj.add_property(attrName,getter,setter,docStr);
		else if(!_ro &&  _post) _classObj.add_property(attrName,getter,setterPostLoad,docStr);
	}
};

#define _DEF_READWRITE_CUSTOM(thisClass,attr) if(!(_ATTR_TRAIT(thisClass,attr).isHidden())){ auto _trait(_ATTR_TRAIT(thisClass,attr)); constexpr bool isNamedEnum(!!(_ATTR_TRAIT_TYPE(thisClass,attr)::compileFlags & woo::Attr::namedEnum)); _def_woo_attr__namedEnum<isNamedEnum>().wooDef<decltype(_classObj),_ATTR_TRAIT_TYPE(thisClass,attr),thisClass,decltype(thisClass::_ATTR_NAM(attr)),&thisClass::_ATTR_NAM(attr)>(_classObj, _trait, BOOST_PP_STRINGIZE(thisClass), _ATTR_NAM_STR(attr)); }

// for static postLoad, use static if via templates
// so that the function does not have to be declared everywhere
template<bool noSave> struct _setter_postLoadStaticMaybe{};
template<> struct _setter_postLoadStaticMaybe<true >{ template<class C, typename T, T* A> static void setter(const T& val){ *A=val; C::postLoadStatic((void*)&(*A)); } };
template<> struct _setter_postLoadStaticMaybe<false>{ template<class C, typename T, T* A> static void setter(const T& val){ *A=val; } };

// static if only for calling postLoad (not for setting an attribute)
template<bool noSave> struct _postLoadStaticMaybe{};
template<> struct _postLoadStaticMaybe<true >{ template<class C, typename T, T* A> static void call(){ C::postLoadStatic((void*)&(*A)); } };
template<> struct _postLoadStaticMaybe<false>{ template<class C, typename T, T* A> static void call(){ } };

// template for static attributes
template<bool namedEnum> struct  _def_woo_attr_static__namedEnum{};

/* instantiation for attribute which IS NOT not a named enumeration */
template<> struct _def_woo_attr_static__namedEnum<false>{
	template<typename classObjT, typename traitT, traitT& (*traitGetter)(), typename classT, typename attrT, attrT *A>
	void wooDef(classObjT& _classObj, traitT& trait, const char* className, const char *attrName){
		bool _ro=trait.isReadonly(), _ref(!_ro && (woo::py_wrap_ref<attrT>::value || trait.isPyByRef()));
		constexpr bool _post=!!(traitT::compileFlags & woo::Attr::triggerPostLoad);
		if(_ref){
			if(_ro) _classObj.add_static_property(attrName,py::make_getter(A));
			else    _classObj.add_static_property(attrName,py::make_getter(A),/*setter*/_setter_postLoadStaticMaybe<_post>::template setter<classT,attrT,A>);
		} else {
			if(_ro) _classObj.add_static_property(attrName,py::make_getter(A,py::return_value_policy<py::return_by_value>()));
			else _classObj.add_static_property(attrName,py::make_getter(A,py::return_value_policy<py::return_by_value>()),/*setter*/_setter_postLoadStaticMaybe<_post>::template setter<classT,attrT,A>);
		}
	}
};
// helper template for static named enumerations
// we can't keep the trait somewhere (needs explicit instantiation for storage)
// so we keep traitGetter pointer around and get the trait reference every time needed
// the trick with lambdas (and make_function_aux) as above does not work for static attributes
// so we do it differently here (reason unknown, template hell)
template<typename classT, typename traitT, traitT& (*traitGetter)(), typename attrT, attrT *A>
struct _namedEnum_accessors{
	// these functions are passed as function pointers to add_static_property
	static string getter(){ return traitGetter().namedEnum_num2name(*A); }
	static void setter(py::object val){
		constexpr bool _post=!!(traitT::compileFlags & woo::Attr::triggerPostLoad);
		*A=traitGetter().namedEnum_name2num(val);
		_postLoadStaticMaybe<_post>::template call<classT,attrT,A>();
	}
};

template<> struct _def_woo_attr_static__namedEnum<true>{
	template<typename classObjT, typename traitT, traitT& (*traitGetter)(), typename classT, typename attrT, attrT *A>
	void wooDef(classObjT& _classObj, traitT& trait, const char* className, const char *attrName){
		bool _ro=trait.isReadonly();
		typedef _namedEnum_accessors<classT,traitT,traitGetter,attrT,A> access;
		if (_ro) _classObj.add_static_property(attrName,access::getter);
		else _classObj.add_static_property(attrName,access::getter,access::setter);
	}
};


#if 1
	// new version, not yet functional
	#define _DEF_READWRITE_CUSTOM_STATIC(thisClass,attr) if(!(_ATTR_TRAIT(thisClass,attr).isHidden())){ auto _trait(_ATTR_TRAIT(thisClass,attr)); constexpr bool isNamedEnum(!!(_ATTR_TRAIT_TYPE(thisClass,attr)::compileFlags & woo::Attr::namedEnum)); _def_woo_attr_static__namedEnum<isNamedEnum>().wooDef<decltype(_classObj),_ATTR_TRAIT_TYPE(thisClass,attr),&_ATTR_TRAIT_GET(thisClass,attr),thisClass,decltype(thisClass::_ATTR_NAM(attr)),&thisClass::_ATTR_NAM(attr)>(_classObj, _trait, BOOST_PP_STRINGIZE(thisClass), _ATTR_NAM_STR(attr)); }
#else
	#define _DEF_READWRITE_CUSTOM_STATIC(thisClass,attr) if(!(_ATTR_TRAIT(thisClass,attr).isHidden())){ \
		bool _ro(_ATTR_TRAIT(thisClass,attr).isReadonly()), _ref(woo::py_wrap_ref<decltype(thisClass::_ATTR_NAM(attr))>::value || (_ATTR_TRAIT(thisClass,attr).isPyByRef())); \
		constexpr bool _post=!!(_ATTR_TRAIT_TYPE(thisClass,attr)::compileFlags & woo::Attr::triggerPostLoad); \
		if      ( _ref &&  _ro) _classObj.add_static_property(_ATTR_NAM_STR(attr),py::make_getter(thisClass::_ATTR_NAM(attr))); \
		else if ( _ref && !_ro) _classObj.add_static_property(_ATTR_NAM_STR(attr),py::make_getter(thisClass::_ATTR_NAM(attr)),/*setter*/_setter_postLoadStaticMaybe<_post>::setter<thisClass,decltype(thisClass::_ATTR_NAM(attr)),&thisClass::_ATTR_NAM(attr)>); \
		else if (!_ref && _ro) _classObj.add_static_property(_ATTR_NAM_STR(attr),py::make_getter(thisClass::_ATTR_NAM(attr),py::return_value_policy<py::return_by_value>())); \
		else if (!_ref &&!_ro) _classObj.add_static_property(_ATTR_NAM_STR(attr),py::make_getter(thisClass::_ATTR_NAM(attr),py::return_value_policy<py::return_by_value>()),/*setter*/_setter_postLoadStaticMaybe<_post>::setter<thisClass,decltype(thisClass::_ATTR_NAM(attr)),&thisClass::_ATTR_NAM(attr)>); \
	}
#endif

// macros for deprecated attribute access
// gcc<=4.3 is not able to compile this code; we will just not generate any code for deprecated attributes in such case
#if (defined(__clang__) || !defined(__GNUG__)) || ((defined(__GNUG__) && (__GNUC__ > 4 || (__GNUC__==4 && __GNUC_MINOR__ > 3))))
	// gcc > 4.3 && non-gcc compilers
	#define _PYSET_ATTR_DEPREC(x,thisClass,z) if(key==BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(z))){ _DEPREC_WARN(thisClass,z); _DEPREC_NEWNAME(z)=py::extract<decltype(_DEPREC_NEWNAME(z))>(value); return; }
	#define _PYATTR_DEPREC_DEF(x,thisClass,z) .add_property(BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(z)),&thisClass::BOOST_PP_CAT(_getDeprec_,_DEPREC_OLDNAME(z)),&thisClass::BOOST_PP_CAT(_setDeprec_,_DEPREC_OLDNAME(z)),"[deprecated] alias for :obj:`" BOOST_PP_STRINGIZE(_DEPREC_NEWNAME(z)) " <" BOOST_PP_STRINGIZE(thisClass) "." BOOST_PP_STRINGIZE(_DEPREC_NEWNAME(z)) ">` (" _DEPREC_COMMENT(z) ")")
	#define _PYHASKEY_ATTR_DEPREC(x,thisClass,z) if(key==BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(z))) return true;
	/* accessors functions ussing warning */
	#define _ACCESS_DEPREC(x,thisClass,z) /*getter*/ decltype(_DEPREC_NEWNAME(z)) BOOST_PP_CAT(_getDeprec_,_DEPREC_OLDNAME(z))(){_DEPREC_WARN(thisClass,z); return _DEPREC_NEWNAME(z); } /*setter*/ void BOOST_PP_CAT(_setDeprec_,_DEPREC_OLDNAME(z))(const decltype(_DEPREC_NEWNAME(z))& val){_DEPREC_WARN(thisClass,z); _DEPREC_NEWNAME(z)=val; }
#else
	#define _PYSET_ATTR_DEPREC(x,y,z)
	#define _PYATTR_DEPREC_DEF(x,y,z)
	#define _PYHASKEY_ATTR_DEPREC(x,y,z)
	#define _ACCESS_DEPREC(x,y,z)
#endif

// static switch to make hidden attributes not settable via ctor args in python
// this avoids compile-time error with boost::multi_array which with py::extract
template<bool hidden, bool namedEnum> struct _setAttrMaybe{};
template<bool namedEnum> struct _setAttrMaybe</*hidden*/true,namedEnum>{
	template<typename traitT, typename Tsrc, typename Tdst>
	static void set(traitT& trait, const string& name, const Tsrc& src, Tdst& dst){ woo::AttributeError(name+" is not settable from python (marked as hidden)."); }
};
template<> struct _setAttrMaybe</*hidden*/false,/*namedEnum*/false>{
	template<typename traitT, typename Tsrc, typename Tdst>
	static void set(traitT& trait, const string& name, const Tsrc& src, Tdst& dst){ dst=py::extract<Tdst>(src); }
};
template<> struct _setAttrMaybe</*hidden*/false,/*namedEnum*/true>{
	template<typename traitT, typename Tsrc, typename Tdst>
	static void set(traitT& trait, const string& name, const Tsrc& src, Tdst& dst){ dst=trait.namedEnum_name2num(src); }
};

// loop bodies for attribute access
#define _PYGET_ATTR(x,y,z) if(key==_ATTR_NAM_STR(z)) return py::object(_ATTR_NAM(z));
#define _PYSET_ATTR(x,klass,z) if(key==_ATTR_NAM_STR(z)) { typedef _ATTR_TRAIT_TYPE(klass,z) traitT; _setAttrMaybe<!!(traitT::compileFlags & woo::Attr::hidden),!!(traitT::compileFlags & woo::Attr::namedEnum)>::set(_ATTR_TRAIT_GET(klass,z)(),key,value,_ATTR_NAM(z)); return; }
#define _PYATTR_TRAIT(x,klass,z)        traitList.append(py::ptr(static_cast<AttrTraitBase*>(&_ATTR_TRAIT_GET(klass,z)())));
#define _PYATTR_TRAIT_STATIC(x,klass,z) traitList.append(py::ptr(static_cast<AttrTraitBase*>(&_ATTR_TRAIT_GET(klass,z)()))); // static_() already set in trait definition
#define _PYHASKEY_ATTR(x,y,z) if(key==_ATTR_NAM_STR(z)) return true;
#define _PYDICT_ATTR(x,y,z) if(!(_ATTR_TRAIT(klass,z).isHidden()) && (all || !(_ATTR_TRAIT(klass,z).isNoSave())) && (all || !(_ATTR_TRAIT(klass,z).isNoDump()))){ /*if(_ATTR_TRAIT(klass,z) & woo::Attr::pyByRef) ret[_ATTR_NAM_STR(z)]=py::object(boost::ref(_ATTR_NAM(z))); else */  ret[_ATTR_NAM_STR(z)]=py::object(_ATTR_NAM(z)); }

// static switch for noSave attributes via templates
template<bool noSave> struct _SerializeMaybe{};
template<> struct _SerializeMaybe<true>{
	template<class ArchiveT, typename T>
	static void serialize(ArchiveT& ar, T& obj, const char* name){ /*std::cerr<<"["<<name<<"]";*/ ar & boost::serialization::make_nvp(name,obj); }
};
template<> struct _SerializeMaybe<false>{
	template<class ArchiveT, typename T>
	static void serialize(ArchiveT& ar, T&, const char* name){ /* do nothing */ }
};


// serialization of a single attribute
#define _WOO_BOOST_SERIALIZE_REPEAT(x,klass,z) _SerializeMaybe<!(_ATTR_TRAIT_TYPE(klass,z)::compileFlags & woo::Attr::noSave)>::serialize(ar,_ATTR_NAM(z), BOOST_PP_STRINGIZE(_ATTR_NAM(z)));

// the body of the serialization function
#define _WOO_BOOST_SERIALIZE_BODY(thisClass,baseClass,attrs) \
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(baseClass);  \
	/* with ADL, either the generic (empty) version above or baseClass::preLoad etc will be called (compile-time resolution) */ \
	if(ArchiveT::is_loading::value) preLoad(*this); else preSave(*this); \
	BOOST_PP_SEQ_FOR_EACH(_WOO_BOOST_SERIALIZE_REPEAT,thisClass,attrs) \
	if(ArchiveT::is_loading::value) postLoad(*this,NULL); else postSave(*this);

// declaration/implementation version of the whole serialization function
// declaration first:
#define _WOO_BOOST_SERIALIZE_DECL(thisClass,baseClass,attrs)\
	friend class boost::serialization::access;\
	private: template<class ArchiveT> void serialize(ArchiveT & ar, unsigned int version);

// implementation: must provide explicit instantiation
#define _WOO_BOOST_SERIALIZE_IMPL_INSTANTIATE(x,thisClass,archiveType) template void thisClass::serialize<archiveType>(archiveType & ar, unsigned int version);
#define _WOO_BOOST_SERIALIZE_IMPL(thisClass,baseClass,attrs)\
	template<class ArchiveT> void thisClass::serialize(ArchiveT & ar, unsigned int version){ \
		_WOO_BOOST_SERIALIZE_BODY(thisClass,baseClass,attrs) \
	} \
	/* explicit instantiation for all available archive types -- see http://www.boost.org/doc/libs/1_55_0/libs/serialization/doc/pimpl.html */ \
	BOOST_PP_SEQ_FOR_EACH(_WOO_BOOST_SERIALIZE_IMPL_INSTANTIATE,thisClass,WOO_BOOST_ARCHIVES)

// inline version of the serialization function
// no need for explcit instantiation, as the code is in headers
#define _WOO_BOOST_SERIALIZE_INLINE(thisClass,baseClass,attrs) \
	friend class boost::serialization::access; \
	private: template<class ArchiveT> void serialize(ArchiveT & ar, unsigned int version){ \
		_WOO_BOOST_SERIALIZE_BODY(thisClass,baseClass,attrs) \
	}



#define _REGISTER_ATTRIBUTES_DEPREC(thisClass,baseClass,attrs,deprec)  _WOO_BOOST_SERIALIZE_INLINE(thisClass,baseClass,attrs) public: \
	void pySetAttr(const std::string& key, const py::object& value) WOO_CXX11_OVERRIDE {BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR,thisClass,attrs); BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR_DEPREC,thisClass,deprec); baseClass::pySetAttr(key,value); } \
	/* return dictionary of all acttributes and values; deprecated attributes omitted */ py::dict pyDict(bool all=true) const WOO_CXX11_OVERRIDE { py::dict ret; BOOST_PP_SEQ_FOR_EACH(_PYDICT_ATTR,thisClass,attrs); ret.update(baseClass::pyDict(all)); return ret; } \
	void callPostLoad(void* addr) WOO_CXX11_OVERRIDE { baseClass::callPostLoad(addr); postLoad(*this,addr); }


// print warning about deprecated attribute; thisClass is type name, not string
#define _DEPREC_WARN(thisClass,deprec)  std::cerr<<"WARN: "<<getClassName()<<"."<<BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(deprec))<<" is deprecated, use "<<BOOST_PP_STRINGIZE(thisClass)<<"."<<BOOST_PP_STRINGIZE(_DEPREC_NEWNAME(deprec))<<" instead. "; if(_DEPREC_COMMENT(deprec)){ if(std::string(_DEPREC_COMMENT(deprec))[0]=='!'){ cerr<<endl; throw std::invalid_argument(BOOST_PP_STRINGIZE(thisClass) "." BOOST_PP_STRINGIZE(_DEPREC_OLDNAME(deprec)) " is deprecated; throwing exception requested. Reason: " _DEPREC_COMMENT(deprec));} else std::cerr<<"("<<_DEPREC_COMMENT(deprec)<<")"; } std::cerr<<endl;

// getters for individual fields
#define _ATTR_TYP(s) BOOST_PP_TUPLE_ELEM(5,0,s)
#define _ATTR_NAM(s) BOOST_PP_TUPLE_ELEM(5,1,s)
#define _ATTR_INI(s) BOOST_PP_TUPLE_ELEM(5,2,s)
#define _ATTR_TRAIT(klass,s) makeAttrTrait(BOOST_PP_TUPLE_ELEM(5,3,s)).doc(_ATTR_DOC(s)).className(BOOST_PP_STRINGIZE(klass)).name(_ATTR_NAM_STR(s)).cxxType(_ATTR_TYP_STR(s)).ini(_ATTR_TYP(s)(_ATTR_INI(s)))
#define _ATTR_TRAIT_TYPE(klass,s) BOOST_PP_CAT(klass,BOOST_PP_CAT(_TraitType_,_ATTR_NAM(s)))
#define _ATTR_TRAIT_GET(klass,s) BOOST_PP_CAT(klass,BOOST_PP_CAT(_getTrait_,_ATTR_NAM(s)))
#define _ATTR_DOC(s) BOOST_PP_TUPLE_ELEM(5,4,s)
// stringized getters
#define _ATTR_TYP_STR(s) BOOST_PP_STRINGIZE(_ATTR_TYP(s))
#define _ATTR_NAM_STR(s) BOOST_PP_STRINGIZE(_ATTR_NAM(s))
#define _ATTR_INI_STR(s) BOOST_PP_STRINGIZE(_ATTR_INI(s))

// deprecated specification getters
#define _DEPREC_OLDNAME(x) BOOST_PP_TUPLE_ELEM(3,0,x)
#define _DEPREC_NEWNAME(x) BOOST_PP_TUPLE_ELEM(3,1,x)
#define _DEPREC_COMMENT(x) BOOST_PP_TUPLE_ELEM(3,2,x) "" // if the argument is omited, return empty string instead of nothing

#define _PY_REGISTER_CLASS_BODY(thisClass,baseClass,classTrait,attrs,deprec,extras) \
	checkPyClassRegistersItself(#thisClass); \
	WOO_SET_DOCSTRING_OPTS; \
	auto traitPtr=make_shared<ClassTrait>(classTrait); traitPtr->name(#thisClass).file(__FILE__).line(__LINE__); \
	py::class_<thisClass,shared_ptr<thisClass>,py::bases<baseClass>,boost::noncopyable> _classObj(#thisClass,traitPtr->getDoc().c_str(),/*call raw ctor even for parameterless construction*/py::no_init); \
	_classObj.def("__init__",py::raw_constructor(Object_ctor_kwAttrs<thisClass>)); \
	_classObj.attr("_classTrait")=traitPtr; \
	BOOST_PP_SEQ_FOR_EACH(_PYATTR_DEF,thisClass,attrs); \
	(void) _classObj BOOST_PP_SEQ_FOR_EACH(_PYATTR_DEPREC_DEF,thisClass,deprec); \
	(void) _classObj extras ; \
	py::list traitList; BOOST_PP_SEQ_FOR_EACH(_PYATTR_TRAIT,thisClass,attrs); _classObj.attr("_attrTraits")=traitList;\
	Object::derivedCxxClasses.push_back(py::object(_classObj));

#define _WOO_CLASS_BASE_DOC_ATTRS_DEPREC_PY(thisClass,baseClass,classTrait,attrs,deprec,extras) \
	_REGISTER_ATTRIBUTES_DEPREC(thisClass,baseClass,attrs,deprec) \
	REGISTER_CLASS_AND_BASE(thisClass,baseClass) \
	/* accessors for deprecated attributes, with warnings */ BOOST_PP_SEQ_FOR_EACH(_ACCESS_DEPREC,thisClass,deprec) \
	/* python class registration */ void pyRegisterClass() WOO_CXX11_OVERRIDE { _PY_REGISTER_CLASS_BODY(thisClass,baseClass,classTrait,attrs,deprec,extras); } \
	void must_use_both_WOO_CLASS_BASE_DOC_ATTRS_and_WOO_PLUGIN(); // virtual ensures v-table for all classes 

// attribute declaration
#define _WOO_ATTR_DECL(x,thisClass,z) _ATTR_TYP(z) _ATTR_NAM(z);
// trait definition - can go both in hpp or cpp (inside or outside the class body)
#define _WOO_TRAIT_DEF(x,thisClass,z) \
	typedef std::remove_reference<decltype(_ATTR_TRAIT(thisClass,z))>::type _ATTR_TRAIT_TYPE(thisClass,z); \
	static _ATTR_TRAIT_TYPE(thisClass,z)& _ATTR_TRAIT_GET(thisClass,z)(){ static _ATTR_TRAIT_TYPE(thisClass,z) _tmp=_ATTR_TRAIT(thisClass,z); return _tmp; }

// return "type name;", define trait type and getter
#define _ATTR_DECL_AND_TRAIT(x,thisClass,z) _WOO_ATTR_DECL(x,thisClass,z) _WOO_TRAIT_DEF(x,thisClass,z)


// return name(default), (for initializers list); TRICKY: last one must not have the comma
#define _ATTR_MAKE_INITIALIZER(x,maxIndex,i,z) BOOST_PP_TUPLE_ELEM(2,0,z)(BOOST_PP_TUPLE_ELEM(2,1,z)) BOOST_PP_COMMA_IF(BOOST_PP_NOT_EQUAL(maxIndex,i))
#define _ATTR_NAME_ADD_DUMMY_FIELDS(x,y,z) ((/*type*/,z,/*default*/,/*flags*/,/*doc*/))
#define _ATTR_MAKE_INIT_TUPLE(x,y,z) (( _ATTR_NAM(z),_ATTR_INI(z) ))



/* _DEF_READWRITE_CUSTOM(thisClass,attr,doc) */ /* duplicate static and non-static attributes do not work (they apparently trigger to-python converter being added; for now, make then non-static, that's it. */
#define _STATATTR_PY(x,thisClass,z) _DEF_READWRITE_CUSTOM_STATIC(thisClass,z)
#define _STATATTR_DECL_AND_TRAIT(x,thisClass,z) \
	static _ATTR_TYP(z) _ATTR_NAM(z); \
	typedef std::remove_reference<decltype(_ATTR_TRAIT(thisClass,z))>::type _ATTR_TRAIT_TYPE(thisClass,z); \
	static _ATTR_TRAIT_TYPE(thisClass,z)& _ATTR_TRAIT_GET(thisClass,z)(){ static _ATTR_TRAIT_TYPE(thisClass,z) _tmp=_ATTR_TRAIT(thisClass,z).static_(); return _tmp; }
#define _STATATTR_INITIALIZE(x,thisClass,z) thisClass::_ATTR_NAM(z)=_ATTR_TYP(z)(_ATTR_INI(z));

#define _STATCLASS_PY_REGISTER_CLASS(thisClass,baseClass,classTrait,attrs,pyExtra)\
	void pyRegisterClass() WOO_CXX11_OVERRIDE { checkPyClassRegistersItself(#thisClass); initSetStaticAttributesValue(); WOO_SET_DOCSTRING_OPTS; \
		auto traitPtr=make_shared<ClassTrait>(classTrait); traitPtr->name(#thisClass).file(__FILE__).line(__LINE__); \
		py::class_<thisClass,shared_ptr<thisClass>,py::bases<baseClass>,boost::noncopyable> _classObj(#thisClass,traitPtr->getDoc().c_str(),/*call raw ctor even for parameterless construction*/py::no_init); _classObj.def("__init__",py::raw_constructor(Object_ctor_kwAttrs<thisClass>)); \
		_classObj.attr("_classTrait")=traitPtr; \
		BOOST_PP_SEQ_FOR_EACH(_STATATTR_PY,thisClass,attrs); \
		(void) _classObj pyExtra ;\
		py::list traitList; BOOST_PP_SEQ_FOR_EACH(_PYATTR_TRAIT_STATIC,thisClass,attrs); _classObj.attr("_attrTraits")=traitList; \
		Object::derivedCxxClasses.push_back(py::object(_classObj)); \
	}

/********************** USER MACROS START HERE ********************/

// attrs is (type,name,init-value,docstring)
#define WOO_CLASS_BASE_DOC(klass,base,doc)                             WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,,,,)
#define WOO_CLASS_BASE_DOC_ATTRS(klass,base,doc,attrs)                 WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,,)
#define WOO_CLASS_BASE_DOC_ATTRS_CTOR(klass,base,doc,attrs,ctor)       WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,ctor,)
#define WOO_CLASS_BASE_DOC_ATTRS_PY(klass,base,doc,attrs,py)           WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,,py)
#define WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(klass,base,doc,attrs,ctor,py) WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,,ctor,py)
#define WOO_CLASS_BASE_DOC_ATTRS_CTOR_DTOR_PY(klass,base,doc,attrs,ctor,dtor,py) WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_DTOR_PY(klass,base,doc,attrs,,ctor,dtor,py)
#define WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_DTOR_PY(klass,base,doc,attrs,inits,ctor,dtor,py) WOO_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_DTOR_PY(klass,base,doc,attrs,,inits,ctor,dtor,py)

// some shortcuts
#define WOO_CLASS_BASE_DOC_ATTRS_INIT_PY(klass,base,doc,attrs,inits,py) WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_DTOR_PY(klass,base,doc,attrs,inits,,,py)
#define WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(klass,base,doc,attrs,inits,ctor,py) WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_DTOR_PY(klass,base,doc,attrs,inits,ctor,,py)

// the most general
#define WOO_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_DTOR_PY(thisClass,baseClass,classTraitSpec,attrs,deprec,inits,ctor,dtor,extras) \
	public: BOOST_PP_SEQ_FOR_EACH(_ATTR_DECL_AND_TRAIT,thisClass,attrs) /* attribute declarations */ \
	thisClass() BOOST_PP_IF(BOOST_PP_SEQ_SIZE(inits attrs),:,) BOOST_PP_SEQ_FOR_EACH_I(_ATTR_MAKE_INITIALIZER,BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(inits attrs)), inits BOOST_PP_SEQ_FOR_EACH(_ATTR_MAKE_INIT_TUPLE,~,attrs)) { ctor ; } /* ctor, with initialization of defaults */ \
	virtual ~thisClass(){ dtor ; }; /* virtual dtor, since classes are polymorphic*/ \
	_WOO_CLASS_BASE_DOC_ATTRS_DEPREC_PY(thisClass,baseClass,makeClassTrait(classTraitSpec),attrs,deprec,extras)

/** static attrs **/
// for static classes (Gl1 functors, for instance)
#define WOO_CLASS_BASE_DOC_STATICATTRS_CTOR_PY(thisClass,baseClass,classTraitSpec,attrs,statCtor,pyExtra)\
	public: BOOST_PP_SEQ_FOR_EACH(_STATATTR_DECL_AND_TRAIT,thisClass,attrs) /* attribute declarations */ \
	/* no ctor */ \
	REGISTER_CLASS_AND_BASE(thisClass,baseClass); \
	_REGISTER_ATTRIBUTES_DEPREC(thisClass,baseClass,attrs,) \
	/* called only at class registration, to set initial values; storage still has to be alocated in the cpp file! */ \
	void initSetStaticAttributesValue(void){ BOOST_PP_SEQ_FOR_EACH(_STATATTR_INITIALIZE,thisClass,attrs); statCtor; } \
	_STATCLASS_PY_REGISTER_CLASS(thisClass,baseClass,makeClassTrait(classTraitSpec),attrs,pyExtra) \
	void must_use_both_WOO_CLASS_BASE_DOC_ATTRS_and_WOO_PLUGIN(); 
#define WOO_CLASS_BASE_DOC_STATICATTRS_PY(thisClass,baseClass,classTraitSpec,attrs,pyExtra)\
	WOO_CLASS_BASE_DOC_STATICATTRS_CTOR_PY(thisClass,baseClass,classTraitSpec,attrs,,pyExtra)
#define WOO_CLASS_BASE_DOC_STATICATTRS(thisClass,baseClass,classTraitSpec,attrs) \
	WOO_CLASS_BASE_DOC_STATICATTRS_PY(thisClass,baseClass,classTraitSpec,attrs,)





#define WOO_DECL__CLASS_BASE_DOC(args) _WOO_DECL__CLASS_BASE_DOC(args)
#define WOO_IMPL__CLASS_BASE_DOC(args) _WOO_IMPL__CLASS_BASE_DOC(args)
#define _WOO_DECL__CLASS_BASE_DOC(klass,base,doc) _WOO_CLASS_DECLARATION(   klass,base,doc,/*attrs*/,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,/*py*/)
#define _WOO_IMPL__CLASS_BASE_DOC(klass,base,doc) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,/*attrs*/,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,/*py*/)

#define WOO_DECL__CLASS_BASE_DOC_ATTRS(args) _WOO_DECL__CLASS_BASE_DOC_ATTRS(args)
#define WOO_IMPL__CLASS_BASE_DOC_ATTRS(args) _WOO_IMPL__CLASS_BASE_DOC_ATTRS(args)
#define _WOO_DECL__CLASS_BASE_DOC_ATTRS(klass,base,doc,attrs) _WOO_CLASS_DECLARATION(   klass,base,doc,attrs,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,/*py*/)
#define _WOO_IMPL__CLASS_BASE_DOC_ATTRS(klass,base,doc,attrs) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,attrs,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,/*py*/)

#define WOO_DECL__CLASS_BASE_DOC_PY(args) _WOO_DECL__CLASS_BASE_DOC_PY(args)
#define WOO_IMPL__CLASS_BASE_DOC_PY(args) _WOO_IMPL__CLASS_BASE_DOC_PY(args)
#define _WOO_DECL__CLASS_BASE_DOC_PY(klass,base,doc,py) _WOO_CLASS_DECLARATION(   klass,base,doc,/*attrs*/,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,py)
#define _WOO_IMPL__CLASS_BASE_DOC_PY(klass,base,doc,py) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,/*attrs*/,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,py)

#define WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(args) _WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(args)
#define WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(args) _WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(args)
#define _WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(klass,base,doc,attrs,pyExtras) _WOO_CLASS_DECLARATION(   klass,base,doc,attrs,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,pyExtras)
#define _WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(klass,base,doc,attrs,pyExtras) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,attrs,/*deprec*/,/*inits*/,/*ctor*/,/*dtor*/,pyExtras)

#define WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(args) _WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(args)
#define WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(args) _WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(args)
#define _WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(klass,base,doc,attrs,ctor) _WOO_CLASS_DECLARATION(   klass,base,doc,attrs,/*deprec*/,/*inits*/,ctor,/*dtor*/,/*pyExtras*/)
#define _WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(klass,base,doc,attrs,ctor) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,attrs,/*deprec*/,/*inits*/,ctor,/*dtor*/,/*pyExtras*/)

#define WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(args) _WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(args)
#define WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(args) _WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(args)
#define _WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(klass,base,doc,attrs,ctor,pyExtras) _WOO_CLASS_DECLARATION(   klass,base,doc,attrs,/*deprec*/,/*inits*/,ctor,/*dtor*/,pyExtras)
#define _WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(klass,base,doc,attrs,ctor,pyExtras) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,attrs,/*deprec*/,/*inits*/,ctor,/*dtor*/,pyExtras)


#define WOO_DECL__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY(args) _WOO_DECL__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY(args)
#define WOO_IMPL__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY(args) _WOO_IMPL__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY(args)
#define _WOO_DECL__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY(klass,base,doc,attrs,ini,ctor,pyExtras) _WOO_CLASS_DECLARATION(   klass,base,doc,attrs,/*deprec*/,ini,ctor,/*dtor*/,pyExtras)
#define _WOO_IMPL__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY(klass,base,doc,attrs,ini,ctor,pyExtras) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,attrs,/*deprec*/,ini,ctor,/*dtor*/,pyExtras)

#define WOO_DECL__CLASS_BASE_DOC_ATTRS_INI_CTOR_DTOR_PY(args) _WOO_DECL__CLASS_BASE_DOC_ATTRS_INI_CTOR_DTOR_PY(args)
#define WOO_IMPL__CLASS_BASE_DOC_ATTRS_INI_CTOR_DTOR_PY(args) _WOO_IMPL__CLASS_BASE_DOC_ATTRS_INI_CTOR_DTOR_PY(args)
#define _WOO_DECL__CLASS_BASE_DOC_ATTRS_INI_CTOR_DTOR_PY(klass,base,doc,attrs,ini,ctor,dtor,pyExtras) _WOO_CLASS_DECLARATION(   klass,base,doc,attrs,/*deprec*/,ini,ctor,dtor,pyExtras)
#define _WOO_IMPL__CLASS_BASE_DOC_ATTRS_INI_CTOR_DTOR_PY(klass,base,doc,attrs,ini,ctor,dtor,pyExtras) _WOO_CLASS_IMPLEMENTATION(klass,base,doc,attrs,/*deprec*/,ini,ctor,dtor,pyExtras)


#define WOO_CLASS_DECLARATION(allArgsTogether) _WOO_CLASS_DECLARATION(allArgsTogether)

#define _WOO_CLASS_DECLARATION(thisClass,baseClass,classTraitSpec,attrs,deprec,inits,ctor,dtor,pyExtras) \
	/*class itself*/	REGISTER_CLASS_AND_BASE(thisClass,baseClass) \
	/* attribute declarations*/ BOOST_PP_SEQ_FOR_EACH(_WOO_ATTR_DECL,thisClass,attrs) \
	/*trait definitions*/ BOOST_PP_SEQ_FOR_EACH(_WOO_TRAIT_DEF,thisClass,attrs) \
	/* later: call postLoad via ADL*/ public: void callPostLoad(void* addr) WOO_CXX11_OVERRIDE { baseClass::callPostLoad(addr); postLoad(*this,addr); } \
	/* accessors for deprecated attributes, with warnings */ BOOST_PP_SEQ_FOR_EACH(_ACCESS_DEPREC,thisClass,deprec) \
	/**follow pure declarations of which implementation is handled sparately**/ \
	/*1. ctor declaration */ thisClass();\
	/*2. dtor declaration */ virtual ~thisClass(); \
	/*3. boost::serialization declarations */ _WOO_BOOST_SERIALIZE_DECL(thisClass,baseClass,attrs) \
	/*4. set attributes from kw ctor */ protected: void pySetAttr(const std::string& key, const py::object& value) WOO_CXX11_OVERRIDE; \
	/*5. for pickling*/ py::dict pyDict(bool all=true) const WOO_CXX11_OVERRIDE; \
	/*6. python class registration*/ void pyRegisterClass() WOO_CXX11_OVERRIDE; \
	/*7. ensures v-table; will be removed later*/ void must_use_both_WOO_CLASS_BASE_DOC_ATTRS_and_WOO_PLUGIN(); \
	/*8.*/ void must_use_both_WOO_CLASS_DECLARATION_and_WOO_CLASS_IMPLEMENTATION(); \
	public: /* make the rest public by default again */

#define WOO_CLASS_IMPLEMENTATION(allArgsTogether) _WOO_CLASS_IMPLEMENTATION(allArgsTogether)

#define _WOO_CLASS_IMPLEMENTATION(thisClass,baseClass,classTraitSpec,attrs,deprec,init,ctor,dtor,pyExtras) \
	/*1.*/ thisClass::thisClass() BOOST_PP_IF(BOOST_PP_SEQ_SIZE(attrs init),:,) BOOST_PP_SEQ_FOR_EACH_I(_ATTR_MAKE_INITIALIZER,BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(attrs init)), BOOST_PP_SEQ_FOR_EACH(_ATTR_MAKE_INIT_TUPLE,~,attrs) init) { ctor; } \
	/*2.*/ thisClass::~thisClass(){ dtor; } \
	/*3.*/ _WOO_BOOST_SERIALIZE_IMPL(thisClass,baseClass,attrs) \
	/*4.*/ void thisClass::pySetAttr(const std::string& key, const py::object& value){ BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR,thisClass,attrs); BOOST_PP_SEQ_FOR_EACH(_PYSET_ATTR_DEPREC,thisClass,deprec); baseClass::pySetAttr(key,value); } \
	/*5.*/ py::dict thisClass::pyDict(bool all) const { py::dict ret; BOOST_PP_SEQ_FOR_EACH(_PYDICT_ATTR,~,attrs); ret.update(baseClass::pyDict(all)); return ret; } \
	/*6.*/ void thisClass::pyRegisterClass() { _PY_REGISTER_CLASS_BODY(thisClass,baseClass,makeClassTrait(classTraitSpec),attrs,deprec,pyExtras); } \
	/*7. -- handled by WOO_PLUGIN */ \
	/*8.*/ void thisClass::must_use_both_WOO_CLASS_DECLARATION_and_WOO_CLASS_IMPLEMENTATION(){};

	


// used only in some exceptional cases, might disappear in the future
#define REGISTER_ATTRIBUTES(baseClass,attrs) _REGISTER_ATTRIBUTES_DEPREC(_SOME_CLASS,baseClass,BOOST_PP_SEQ_FOR_EACH(_ATTR_NAME_ADD_DUMMY_FIELDS,~,attrs),)




/* this used to be in lib/factory/Factorable.hpp */
#define REGISTER_CLASS_AND_BASE(cn,bcn) public: virtual string getClassName() const WOO_CXX11_OVERRIDE { return #cn; }; public: virtual vector<string> getBaseClassNames() const WOO_CXX11_OVERRIDE { return {#bcn}; }

#if 0
	# REGISTER_CLASS_NAME(cn); REGISTER_BASE_CLASS_NAME({#bcn});
	#define REGISTER_CLASS_NAME(cn) 
	#define REGISTER_BASE_CLASS_NAME(bcn) 
#endif

// this is used only in Obejct declaration itself below
#define WOO_TOPLEVEL_OBJECT_REGISTER_CLASS_BASE(cn,bcn) public: virtual string getClassName() const {return #cn;}; virtual vector<string> getBaseCLassNames() const {return #bcn; }
 
namespace woo{

struct Object: public boost::noncopyable, public boost::enable_shared_from_this<Object> {
	// http://www.boost.org/doc/libs/1_49_0/libs/smart_ptr/sp_techniques.html#static
	struct null_deleter{void operator()(void const *)const{}};
	static vector<py::object> derivedCxxClasses;
	static py::list getDerivedCxxClasses();
	// this is only for informative purposes, hence the typecast is OK
	ptrdiff_t pyCxxAddr() const{ return (ptrdiff_t)this; }

		template <class ArchiveT> void serialize(ArchiveT & ar, unsigned int version){ };
		// lovely cast members like in eigen :-)
		template <class DerivedT> const DerivedT& cast() const { return *static_cast<const DerivedT*>(this); }
		template <class DerivedT> DerivedT& cast(){ return *static_cast<DerivedT*>(this); }
		// testing instance type
		template <class DerivedT> bool isA() const { return (bool)dynamic_cast<const DerivedT*>(this); }

		static shared_ptr<Object> boostLoad(const string& f){ auto obj=make_shared<Object>(); ObjectIO::load(f,"woo__Object",obj); return obj; }
		virtual void boostSave(const string& f){ auto sh(shared_from_this()); ObjectIO::save(f,"woo__Object",sh); }
		//template<class DerivedT> shared_ptr<DerivedT> _cxxLoadChecked(const string& f){ auto obj=_cxxLoad(f); auto obj2=dynamic_pointer_cast<DerivedT>(obj); if(!obj2) throw std::runtime_error("Loaded type "+obj->getClassName()+" could not be cast to requested type "+DerivedT::getClassNameStatic()); }

		Object() {};
		virtual ~Object() {};
		// comparison of strong equality of 2 objects (by their address)
		bool operator==(const Object& other) const { return this==&other; }
		bool operator!=(const Object& other) const { return this!=&other; }

		void pyUpdateAttrs(const py::dict& d);
		//static void pyUpdateAttrs(const shared_ptr<Object>&, const py::dict& d);

		virtual void pySetAttr(const std::string& key, const py::object& value){ woo::AttributeError("No such attribute: "+key+".");};
		virtual py::dict pyDict(bool all=true) const { return py::dict(); }
		virtual void callPostLoad(void* addr){ postLoad(*this,addr); }
		// check whether the class registers itself or whether it calls virtual function of some base class;
		// that means that the class doesn't register itself properly
		virtual void checkPyClassRegistersItself(const std::string& thisClassName) const;
		// perform class registration; overridden in all classes
		virtual void pyRegisterClass();
		// perform any manipulation of arbitrary constructor arguments coming from python, manipulating them in-place;
		// the remainder is passed to the Object_ctor_kwAttrs of the respective class (note: args must be empty)
		virtual void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){ return; }
		
		//! string representation of this object
		virtual std::string pyStr() const { return "<"+getClassName()+" @ "+boost::lexical_cast<string>(this)+">"; }
	
	// overridden by REGISTER_CLASS_BASE_BASE in derived classes
	virtual string getClassName() const { return "Object"; }
	virtual vector<string> getBaseClassNames() const{ return {}; }

	std::string getBaseClassName(unsigned int i=0) const { std::vector<std::string> bases(getBaseClassNames()); return (i>=bases.size()?std::string(""):bases[i]); } 
	int getBaseClassNumber(){ return getBaseClassNames().size(); }
};

// helper functions
template <typename T>
shared_ptr<T> Object_ctor_kwAttrs(py::tuple& t, py::dict& d){
	shared_ptr<T> instance=make_shared<T>();
	instance->pyHandleCustomCtorArgs(t,d); // can change t and d in-place
	if(py::len(t)>0) throw std::runtime_error("Zero (not "+boost::lexical_cast<string>(py::len(t))+") non-keyword constructor arguments required [in Object_ctor_kwAttrs; Object::pyHandleCustomCtorArgs might had changed it after your call].");
	if(py::len(d)>0) instance->pyUpdateAttrs(d);
	instance->callPostLoad(NULL); 
	return instance;
}

}; /* namespace woo */
