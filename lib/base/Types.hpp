#pragma once

#include<boost/foreach.hpp>
#ifndef FOREACH
	#define FOREACH BOOST_FOREACH
#endif

#include<boost/lexical_cast.hpp>
using boost::lexical_cast;

// enables beautiful constructs like: for(int x: {0,1,2})...
#include<initializer_list>

/* Avoid using std::shared_ptr, because boost::serialization does not support it.
   For boost::python, declaring get_pointer template would be enough.
	See http://stackoverflow.com/questions/6568952/c0x-stdshared-ptr-vs-boostshared-ptr
*/
#include<boost/shared_ptr.hpp>
#include<boost/make_shared.hpp>
using boost::shared_ptr;
using boost::static_pointer_cast;
using boost::dynamic_pointer_cast;
using boost::make_shared;

#if 0
	#include<memory>
	using std::shared_ptr;
	namespace boost { namespace python { template<class T> T* get_pointer(std::shared_ptr<T> const& p){ return p.get(); } } }
#endif

#include<vector>
#include<map>
#include<set>
#include<list>
#include<string>
#include<iostream>
#include<stdexcept>
#include<tuple>
#include<cmath>
using std::vector;
using std::map;
using std::set;
using std::list;
using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::pair;
using std::runtime_error;
using std::invalid_argument;
using std::logic_error;
using std::max;
using std::min;
using std::abs;
//using std::isnan;
//using std::isinf;

// FIXME: this is not a very reliable way to detect presence of to_string
// gcc built without --enable-c99 does not support to_string (checked with 4.7.1)
// so we just emulate that thing with lexical_cast
#if defined(__GNUC__) && !defined(_GLIBCXX_USE_C99)
	template<typename T> string to_string(const T& t){ return lexical_cast<string>(t); }
#else
	using std::to_string;
#endif

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;

#include<boost/python.hpp>
namespace py=boost::python;

