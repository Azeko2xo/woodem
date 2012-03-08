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

#include<yade/lib/base/Types.hpp>

#include<iostream>
#include<cstdio>

#include<yade/lib/base/Singleton.hpp>
#include<yade/lib/base/Logging.hpp>

#include<boost/preprocessor.hpp>
#include<boost/version.hpp>
#include<boost/archive/binary_oarchive.hpp>
#include<boost/archive/binary_iarchive.hpp>

#ifdef YADE_XMLSERIALIZATION
	#include<boost/archive/xml_oarchive.hpp>
	#include<boost/archive/xml_iarchive.hpp>
#endif

#include<boost/serialization/export.hpp> // must come after all supported archive types


#include<boost/serialization/base_object.hpp>
#include<boost/serialization/shared_ptr.hpp>
#include<boost/serialization/list.hpp>
#include<boost/serialization/vector.hpp>
#include<boost/serialization/map.hpp>
#include<boost/serialization/nvp.hpp>


class Factorable;

class ClassFactory: public Singleton<ClassFactory>{
	private:
		typedef shared_ptr<Factorable> (*CreateSharedFnPtr)();
		// pointer to factory func
	 	/// map class name to the pointer of argumentless function returning shared_ptr<Factorable> to that class
		typedef std::map<std::string, CreateSharedFnPtr> factorableCreatorsMap;
		factorableCreatorsMap map;
		ClassFactory() { if(getenv("YADE_DEBUG")) fprintf(stderr,"Constructing ClassFactory.\n"); }
		DECLARE_LOGGER;
	public:
		// register class in the class factory
		bool registerFactorable(const std::string& name, CreateSharedFnPtr createShared);
		/// Create a shared pointer on a serializable class of the given name
		shared_ptr<Factorable> createShared(const std::string& name);
		void load(const std::string& fullFileName);
		void registerPluginClasses(const char* module, const char* fileAndClasses[]);
		std::list<std::pair<std::string,std::string> > modulePluginClasses;
	FRIEND_SINGLETON(ClassFactory);
};


/*! Macro defining what classes can be found in this plugin -- must always be used in the respective .cpp file.
 * A function registerThisPluginClasses_FirstPluginName is generated at every occurence. The identifier should
 * be unique and avoids use of __COUNTER__ which didn't appear in gcc until 4.3.
 */

#if BOOST_VERSION>=104200
	#define _YADE_PLUGIN_BOOST_REGISTER(x,y,z) BOOST_CLASS_EXPORT_IMPLEMENT(z); BOOST_SERIALIZATION_FACTORY_0(z);
#else
	#define _YADE_PLUGIN_BOOST_REGISTER(x,y,z) BOOST_CLASS_EXPORT(z); BOOST_SERIALIZATION_FACTORY_0(z);
#endif


// the __attribute__((constructor(priority))) construct not supported before gcc 4.3
// it will only produce warning from log4cxx if not used
#if __GNUC__ == 4 && __GNUC_MINOR__ >=3
	#define YADE_CTOR_PRIORITY(p) (p)
#else
	#define YADE_CTOR_PRIORITY(p)
#endif

#define _PLUGIN_CHECK_REPEAT(x,y,z) void z::must_use_both_YADE_CLASS_BASE_DOC_ATTRS_and_YADE_PLUGIN(){}
#define _YADE_PLUGIN_REPEAT(x,y,z) BOOST_PP_STRINGIZE(z),
#define _YADE_FACTORY_REPEAT(x,y,z) shared_ptr<Factorable> BOOST_PP_CAT(createShared,z)(){ return shared_ptr<z>(new z); } const bool BOOST_PP_CAT(registered,z) __attribute__ ((unused))=ClassFactory::instance().registerFactorable(BOOST_PP_STRINGIZE(z),BOOST_PP_CAT(createShared,z));

// priority 500 is greater than priority for log4cxx initialization (in core/main/pyboot.cpp); therefore lo5cxx will be initialized before plugins are registered
#define YADE_PLUGIN0(plugins) YADE_PLUGIN(wrapper,plugins)

#define YADE_PLUGIN(module,plugins) BOOST_PP_SEQ_FOR_EACH(_YADE_FACTORY_REPEAT,~,plugins); namespace{ __attribute__((constructor)) void BOOST_PP_CAT(registerThisPluginClasses_,BOOST_PP_SEQ_HEAD(plugins)) (void){ const char* info[]={__FILE__ , BOOST_PP_SEQ_FOR_EACH(_YADE_PLUGIN_REPEAT,~,plugins) NULL}; ClassFactory::instance().registerPluginClasses(BOOST_PP_STRINGIZE(module),info);} } BOOST_PP_SEQ_FOR_EACH(_YADE_PLUGIN_BOOST_REGISTER,~,plugins) BOOST_PP_SEQ_FOR_EACH(_PLUGIN_CHECK_REPEAT,~,plugins)

