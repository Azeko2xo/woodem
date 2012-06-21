#pragma once

#include<string>
#include<boost/preprocessor.hpp>
#include<boost/format.hpp>

namespace yade{
	void StopIteration();
	#define WOO_PYUTIL_ERRORS (ArithmeticError)(AttributeError)(IndexError)(KeyError)(NameError)(NotImplementedError)(RuntimeError)(TypeError)(ValueError)
	#define _DECLARE_YADE_PY_ERROR(x,y,err) void err(const std::string&); void err(const boost::format& f);
	BOOST_PP_SEQ_FOR_EACH(_DECLARE_YADE_PY_ERROR,~,WOO_PYUTIL_ERRORS)
	#undef _DECLARE_YADE_PY_ERROR

	// return string representation of current python exception
	std::string parsePythonException();
	// use this if the GIL is already locked
	std::string parsePythonException_gilLocked();
};
