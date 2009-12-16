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

#include <boost/scoped_ptr.hpp>
#include<stdexcept>
#include<string>

/*! \brief Abstract interface for all Indexable class.
	An indexable class is a class that will be managed by a MultiMethodManager.
	The index the function getClassIndex() returns, corresponds to the index in the matrix where the class will be handled.
*/

#define _THROW_NOT_OVERRIDDEN  throw std::logic_error(std::string("Derived class did not override ")+__PRETTY_FUNCTION__+", use REGISTER_INDEX_COUNTER and REGISTER_CLASS_INDEX.")

class Indexable
{
	protected :
		void createIndex();

	public :
		Indexable ();
		virtual ~Indexable ();

		/// Returns the id of the current class. This id is set by a multimethod manager
		virtual int& getClassIndex()                             { _THROW_NOT_OVERRIDDEN;}; 
		virtual const int& getClassIndex() const                 { _THROW_NOT_OVERRIDDEN;};
		virtual int& getBaseClassIndex(int )                     { _THROW_NOT_OVERRIDDEN;};
		virtual const int& getBaseClassIndex(int ) const         { _THROW_NOT_OVERRIDDEN;};
		virtual const int& getMaxCurrentlyUsedClassIndex() const { _THROW_NOT_OVERRIDDEN;};
		virtual void incrementMaxCurrentlyUsedClassIndex()       { _THROW_NOT_OVERRIDDEN;};

};

#undef _THROW_NOT_OVERRIDDEN

// this macro is used by classes that are a dimension in multimethod matrix

#define REGISTER_CLASS_INDEX(SomeClass,BaseClass)                                      \
	public: static int& getClassIndexStatic() { static int index = -1; return index; } \
	public: virtual int& getClassIndex()       { return getClassIndexStatic(); }        \
	public: virtual const int& getClassIndex() const { return getClassIndexStatic(); }  \
	public: virtual int& getBaseClassIndex(int depth) {              \
		static boost::scoped_ptr<BaseClass> baseClass(new BaseClass); \
		if(depth == 1) return baseClass->getClassIndex();             \
		else           return baseClass->getBaseClassIndex(--depth);  \
	}                                                                \
	public: virtual const int& getBaseClassIndex(int depth) const {  \
		static boost::scoped_ptr<BaseClass> baseClass(new BaseClass); \
		if(depth == 1) return baseClass->getClassIndex();             \
		else           return baseClass->getBaseClassIndex(--depth);  \
	}

// this macro is used by base class for classes that are a dimension in multimethod matrix
// to keep track of maximum number of classes of their kin. Multimethod matrix can't
// count this number (ie. as a size of the matrix), as there are many multimethod matrices

#define REGISTER_INDEX_COUNTER(SomeClass) \
	private: static int& getClassIndexStatic()       { static int index = -1; return index; }\
	public: virtual int& getClassIndex()             { return getClassIndexStatic(); }       \
	public: virtual const int& getClassIndex() const { return getClassIndexStatic(); }       \
	public: virtual int& getBaseClassIndex(int)             { throw std::logic_error("Class " #SomeClass " should not have called createIndex() in its ctor (since it is a top-level indexable, using REGISTER_INDEX_COUNTER"); } \
	public: virtual const int& getBaseClassIndex(int) const { throw std::logic_error("Class " #SomeClass " should not have called createIndex() in its ctor (since it is a top-level indexable, using REGISTER_INDEX_COUNTER"); } \
	private: static int& getMaxCurrentlyUsedIndexStatic() { static int maxCurrentlyUsedIndex = -1; return maxCurrentlyUsedIndex; } \
	public: virtual const int& getMaxCurrentlyUsedClassIndex() const {  \
		SomeClass * Indexable##SomeClass = 0;                            \
		Indexable##SomeClass = dynamic_cast<SomeClass*>(const_cast<SomeClass*>(this)); \
		assert(Indexable##SomeClass);                                    \
		return getMaxCurrentlyUsedIndexStatic();                         \
	}                                                                   \
	public: virtual void incrementMaxCurrentlyUsedClassIndex() {        \
		SomeClass * Indexable##SomeClass = 0;                            \
		Indexable##SomeClass = dynamic_cast<SomeClass*>(this);           \
		assert(Indexable##SomeClass);                                    \
		int& max = getMaxCurrentlyUsedIndexStatic();                     \
		max++;                                                           \
	}


