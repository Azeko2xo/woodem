#include<yade/lib/serialization/Serializable.hpp>

struct Scene; 
struct Preprocessor: public Serializable{
	virtual shared_ptr<Scene> operator()();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Preprocessor,Serializable,"Subclasses of this class generate a Scene object when called, based on their attributes."
		, /* attrs */
		, /* ctor */
		, /* py */
		.def("__call__",&Preprocessor::operator())
	);
};
REGISTER_SERIALIZABLE(Preprocessor);

#if 0
struct PyPreprocessor: public Preprocessor, py::wrapper<Preprocessor>{
	// dispatchers for virtual functions
	shared_ptr<Scene> operator()(){ return this->get_override("__call__")(); }
	/* this exposes some internals of Serializable, ideally it would be a mixin class */
	virtual void must_use_both_YADE_CLASS_BASE_DOC_ATTRS_and_YADE_PLUGIN();
	// yattrs must be overridden in python
	py::list pyYAttrs() const;
	py::dict pyDict() const;
	void pyRegisterClass(py::object _scope){
		py::scope thisScope(_scope);
		YADE_SET_DOCSTRING_OPTS;
		py::class_<PyPreprocessor,shared_ptr<PyPreprocessor>,boost::noncopyable> classObj("PyPreprocessor","Parent class for preprocessors implemented in python",py::no_init);
		classObj.def("__init__",py::raw_constructor(Serializable_ctor_kwAttrs<PyPreprocessor>));
	}
	template<class ArchiveT> void serialize(ArchiveT &ar, unsigned int version){
		throw std::runtime_error("PyPreprocessor class does not support boost::serialization.");
	};
};
REGISTER_SERIALIZABLE(PyPreprocessor);
#endif
