// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

/* use boost's implementation of Singleton, with thin compatibility layer */
#include<boost/serialization/singleton.hpp>
#define FRIEND_SINGLETON(Class) friend struct Singleton<Class>;
template<class T>
struct Singleton: public boost::serialization::singleton<T>{
	static T& instance(){ return boost::serialization::singleton<T>::get_mutable_instance(); }
};

