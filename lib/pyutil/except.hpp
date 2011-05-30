
#include<string>
#include<boost/preprocessor.hpp>

namespace yade{
	void StopIteration();
	#define YADE_PYUTIL_ERRORS (ArithmeticError)(AttributeError)(IndexError)(KeyError)(NameError)(NotImplementedError)(RuntimeError)(TypeError)(ValueError)
	#define _DECLARE_YADE_PY_ERROR(x,y,err) void err(const std::string&);
	BOOST_PP_SEQ_FOR_EACH(_DECLARE_YADE_PY_ERROR,~,YADE_PYUTIL_ERRORS)
	#undef _DECLARE_YADE_PY_ERROR
};
