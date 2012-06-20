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

#include<yade/lib/base/Singleton.hpp>
#include<yade/lib/base/Logging.hpp>
#include<yade/lib/base/Types.hpp>
namespace yade { class Object; };
using namespace yade;


class ClassFactory: public Singleton<ClassFactory>{
	private:
		typedef shared_ptr<Object> (*CreateSharedFnPtr)();
		// pointer to factory func
	 	/// map class name to the pointer of argumentless function returning shared_ptr<Factorable> to that class
		typedef std::map<std::string, CreateSharedFnPtr> factorableCreatorsMap;
		factorableCreatorsMap map;
	public:
		ClassFactory() { if(getenv("YADE_DEBUG")) fprintf(stderr,"Constructing ClassFactory.\n"); }
		DECLARE_LOGGER;
		// register class in the class factory
		bool registerFactorable(const std::string& name, CreateSharedFnPtr createShared);
		/// Create a shared pointer of an object with given name
		shared_ptr<Object> createShared(const std::string& name);
		void load(const std::string& fullFileName);
		void registerPluginClasses(const char* module, const char* fileAndClasses[]);
		std::list<std::pair<std::string,std::string> > modulePluginClasses;
	FRIEND_SINGLETON(ClassFactory);
};


