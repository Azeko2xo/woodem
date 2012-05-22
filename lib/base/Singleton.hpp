// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

/* keep the old singleton implementation here for some time, in case of troubles;
   otherwise, use the implementation from boost, with thin compatibility layer here
*/
#if 1
	#include<boost/serialization/singleton.hpp>
	#define FRIEND_SINGLETON(Class) friend class Singleton<Class>;
	#define SINGLETON_SELF(Class)
	template<class T>
	struct Singleton: public boost::serialization::singleton<T>{
		static T& instance(){ return boost::serialization::singleton<T>::get_mutable_instance(); }
	};
#else
	#include <boost/thread/mutex.hpp>
	#define FRIEND_SINGLETON(Class) friend class Singleton<Class>;					
	// use to instantiate the self static member.
	#define SINGLETON_SELF(Class) template<> Class* Singleton<Class>::self=NULL;
	namespace { boost::mutex singleton_constructor_mutex; }
	template <class T>
	class Singleton{
		protected:
			static T* self; // must not be method-local static variable, since it gets created in different translation units multiple times.
		public:
			static T& instance(){
				if(!self) {
					boost::mutex::scoped_lock lock(singleton_constructor_mutex);
					if(!self) self=new T;
				}
				return *self;
			}
	};
#endif

