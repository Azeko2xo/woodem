#include "Serializable.hpp"

#if 0
static void Serializable_setAttr(py::object self, py::str name, py::object value){
#if 0
	py::dict d=py::extract<py::dict>(py::getattr(self,"__dict__"));
	if(d.has_key(name))
#endif
	{ py::setattr(self,name,value); return; }
#if 0
	yade::AttributeError(("Class "+py::extract<std::string>(py::getattr(py::getattr(self,"__class__"),"__name__"))()+" does not have attribute "+py::extract<std::string>(name)()+".").c_str());
#endif
}
#endif

void Serializable::pyRegisterClass(py::object _scope) {
	checkPyClassRegistersItself("Serializable");
	py::scope thisScope(_scope); 
	py::class_<Serializable, shared_ptr<Serializable>, boost::noncopyable >("Serializable")
		.def("__str__",&Serializable::pyStr).def("__repr__",&Serializable::pyStr)
		.def("dict",&Serializable::pyDict,"Return dictionary of attributes.")
		.def("yattrs",&Serializable::pyYAttrs,"Return names of registered attributes.")
		.def("updateAttrs",&Serializable::pyUpdateAttrs,"Update object attributes from given dictionary")
		#if 1
			/* boost::python pickling support, as per http://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/pickle.html */ 
			.def("__getstate__",&Serializable::pyDict).def("__setstate__",&Serializable::pyUpdateAttrs)
			.add_property("__safe_for_unpickling__",&Serializable::getClassName,"just define the attr, return some bogus data")
			.add_property("__getstate_manages_dict__",&Serializable::getClassName,"just define the attr, return some bogus data")
		#endif
		// setting attributes with protection of creating class instance mistakenly
		#if 0
			.def("__setattr__",&Serializable_setAttr)
		#endif
		// constructor with dictionary of attributes
		.def("__init__",py::raw_constructor(Serializable_ctor_kwAttrs<Serializable>))
		// comparison operators
		.def(py::self == py::self)
		.def(py::self != py::self)
		;
}

void Serializable::checkPyClassRegistersItself(const std::string& thisClassName) const {
	if(getClassName()!=thisClassName) throw std::logic_error(("Class "+getClassName()+" does not register with YADE_CLASS_BASE_DOC_ATTR*, would not be accessible from python.").c_str());
}

void Serializable::pyUpdateAttrs(const py::dict& d){	
	py::list l=d.items(); size_t ll=py::len(l); if(ll==0) return;
	for(size_t i=0; i<ll; i++){
		py::tuple t=py::extract<py::tuple>(l[i]);
		string key=py::extract<string>(t[0]);
		pySetAttr(key,t[1]);
	}
	callPostLoad();
}

