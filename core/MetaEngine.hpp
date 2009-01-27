/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include "Engine.hpp"
#include "EngineUnit.hpp"

class MetaEngine : public Engine
{
	public:
		vector<vector<string> >		functorNames; // public for python interface; since there is getFunctorArguments returning RW(!) reference to this, why have it private anyway?!
		list<shared_ptr<EngineUnit> >	functorArguments;
	protected:
		void storeFunctorArguments(shared_ptr<EngineUnit> eu);

	public :
		vector<vector<string> >& getFunctorNames();

		typedef list<shared_ptr<EngineUnit> >::iterator EngineUnitListIterator;
		
		//virtual void add( string , string , shared_ptr<EngineUnit> eu = shared_ptr<EngineUnit>()) {throw;}
		//virtual void add( string , string , string , shared_ptr<EngineUnit> eu = shared_ptr<EngineUnit>()) {throw;}
		virtual void add(shared_ptr<EngineUnit> eu=shared_ptr<EngineUnit>()) {throw;}
		virtual void add(EngineUnit*) {throw;}
		virtual void add(string) {throw;}

		virtual list<string> getNeededBex();
		
		void storeFunctorName(const string& baseClassName1, const string& libName, shared_ptr<EngineUnit> eu);
		void storeFunctorName(const string& baseClassName1, const string& baseClassName2, const string& libName, shared_ptr<EngineUnit> eu);
		void storeFunctorName(const string& baseClassName1, const string& baseClassName2, const string& baseClassName3, const string& libName, shared_ptr<EngineUnit> eu);
		shared_ptr<EngineUnit> findFunctorArguments(const string& libName);
		void clear();
		
		MetaEngine();
		virtual ~MetaEngine();
		
		virtual string getEngineUnitType() { throw; };
		virtual int getDimension() { throw; };
		virtual string getBaseClassType(unsigned int ) { throw; };

	protected:
		virtual void postProcessAttributes(bool deserializing);
	REGISTER_ATTRIBUTES(Engine,(functorNames)(functorArguments));
	REGISTER_CLASS_AND_BASE(MetaEngine,Engine);
};

REGISTER_SERIALIZABLE(MetaEngine);


