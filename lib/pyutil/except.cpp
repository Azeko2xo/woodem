#include<yade/lib/pyutil/except.hpp>
#include<boost/python.hpp>

namespace yade{
	void StopIteration(){ PyErr_SetNone(PyExc_StopIteration); boost::python::throw_error_already_set(); }

	// void ArithmeticError(const std::string& what){ PyErr_SetString(PyExc_ArithmeticError,what.c_str()); boost::python::throw_error_already_set(); }
	#define _DEFINE_YADE_PY_ERROR(x,y,AnyError) void AnyError(const std::string& what){ PyErr_SetString(BOOST_PP_CAT(PyExc_,AnyError),what.c_str()); boost::python::throw_error_already_set(); }
	BOOST_PP_SEQ_FOR_EACH(_DEFINE_YADE_PY_ERROR,~,YADE_PYUTIL_ERRORS)
	#undef _DEFINE_YADE_PY_ERROR
};
