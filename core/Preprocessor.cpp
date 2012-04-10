#include<yade/core/Preprocessor.hpp>
YADE_PLUGIN(core,(Preprocessor)
	#if 0
		(PyPreprocessor)
	#endif
);

shared_ptr<Scene> Preprocessor::operator()(){ throw std::logic_error("Preprocessor() called on the base class."); }

#if 0
py::list PyPreprocessor::pyYAttrs() const {
	auto method=this->get_override("yattrs");
	if(!method) throw std::runtime_error("PyPreprocessor.yattrs not overridden!");
	py::extract<py::list> lex=method();
	if(!lex.check()) throw std::runtime_error("PyPreprocessor.yattrs did not return py::list object!");
	py::list l(Preprocessor::pyYAttrs());
	l.extend(lex());
	return l;
}

py::dict PyPreprocessor::pyDict() const {
	auto method=this->get_override("dict");
	if(!method) throw std::runtime_error("PyPreprocessor.dict not overridden!");
	py::extract<py::dict> dex=method();
	if(!dex.check()) throw std::runtime_error("PyPreprocessor.dict did not return py::dict object!");
	py::dict d(Preprocessor::pyDict());
	d.update(dex());
	return d;
}
#endif
