#include<woo/lib/pyutil/except.hpp>
#include<woo/lib/pyutil/gil.hpp>
#include<boost/python.hpp>

namespace yade{
	void StopIteration(){ PyErr_SetNone(PyExc_StopIteration); boost::python::throw_error_already_set(); }

	// void ArithmeticError(const std::string& what){ PyErr_SetString(PyExc_ArithmeticError,what.c_str()); boost::python::throw_error_already_set(); }
	#define _DEFINE_YADE_PY_ERROR(x,y,AnyError) void AnyError(const std::string& what){ PyErr_SetString(BOOST_PP_CAT(PyExc_,AnyError),what.c_str()); boost::python::throw_error_already_set(); } void AnyError(const boost::format& f){ AnyError(f.str()); }
	BOOST_PP_SEQ_FOR_EACH(_DEFINE_YADE_PY_ERROR,~,WOO_PYUTIL_ERRORS)
	#undef _DEFINE_YADE_PY_ERROR
	
	// http://thejosephturner.com/blog/2011/06/15/embedding-python-in-c-applications-with-boostpython-part-2/
	std::string parsePythonException(){
		GilLock lock;
		return parsePythonException_gilLocked();
	}
	std::string parsePythonException_gilLocked(){
		PyObject *type_ptr = NULL, *value_ptr = NULL, *traceback_ptr = NULL;
		PyErr_Fetch(&type_ptr, &value_ptr, &traceback_ptr);
		std::string ret("Unfetchable Python error");
		if(type_ptr != NULL){
			py::handle<> h_type(type_ptr);
			py::str type_pstr(h_type);
			py::extract<std::string> e_type_pstr(type_pstr);
			if(e_type_pstr.check())
				ret = e_type_pstr();
			else
				ret = "Unknown exception type";
		}
		if(value_ptr != NULL){
			py::handle<> h_val(value_ptr);
			py::str a(h_val);
			py::extract<std::string> returned(a);
			if(returned.check())
				ret +=  ": " + returned();
			else
				ret += std::string(": Unparseable Python error: ");
		}
			 if(traceback_ptr != NULL){
			py::handle<> h_tb(traceback_ptr);
			py::object tb(py::import("traceback"));
			py::object fmt_tb(tb.attr("format_tb"));
			py::object tb_list(fmt_tb(h_tb));
			py::object tb_str(py::str("\n").join(tb_list));
			py::extract<std::string> returned(tb_str);
			if(returned.check())
				ret += ": " + returned();
			else
				ret += std::string(": Unparseable Python traceback");
		}
		return ret;
	}

};
