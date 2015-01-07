#pragma once
#include<boost/version.hpp>

// macro to set the same docstring generation options in all modules
#define WOO_SET_DOCSTRING_OPTS boost::python::docstring_options docopt; docopt.enable_all(); docopt.disable_cpp_signatures();

