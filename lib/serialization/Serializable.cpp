#include "Serializable.hpp"


void Serializable::pyRegisterClass(boost::python::object _scope) {
	checkPyClassRegistersItself("Serializable");
	boost::python::scope thisScope(_scope); 
	python::class_<Serializable, shared_ptr<Serializable>, noncopyable >("Serializable")
		.def("__str__",&Serializable::pyStr).def("__repr__",&Serializable::pyStr)
		.def("dict",&Serializable::pyDict,"Return dictionary of attributes.")
		.def("updateAttrs",&Serializable::pyUpdateAttrs,"Update object attributes from given dictionary")
		#if 1
			/* boost::python pickling support, as per http://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/pickle.html */ 
			.def("__getstate__",&Serializable::pyDict).def("__setstate__",&Serializable::pyUpdateAttrs)
			.add_property("__safe_for_unpickling__",&Serializable::getClassName,"just define the attr, return some bogus data")
			.add_property("__getstate_manages_dict__",&Serializable::getClassName,"just define the attr, return some bogus data")
		#endif
		// constructor with dictionary of attributes
		.def("__init__",python::raw_constructor(Serializable_ctor_kwAttrs<Serializable>))
		// comparison operators
		.def(boost::python::self == boost::python::self)
		.def(boost::python::self != boost::python::self)
		;
}

void Serializable::checkPyClassRegistersItself(const std::string& thisClassName) const {
	if(getClassName()!=thisClassName) throw std::logic_error(("Class "+getClassName()+" does not register with YADE_CLASS_BASE_DOC_ATTR*, would not be accessible from python.").c_str());
}

#if 1
	void Serializable::pyUpdateAttrs(const python::dict& d){	
		python::list l=d.items(); size_t ll=python::len(l); if(ll==0) return;
		for(size_t i=0; i<ll; i++){
			python::tuple t=python::extract<python::tuple>(l[i]);
			string key=python::extract<string>(t[0]);
			pySetAttr(key,t[1]);
		}
		callPostLoad();
	}
#else
	void Serializable::pyUpdateAttrs(const shared_ptr<Serializable>& instance, const python::dict& d){	
		python::list l=d.items(); size_t ll=python::len(l); if(ll==0) return;
		python::object self(instance);
		for(size_t i=0; i<ll; i++){
			python::tuple t=python::extract<python::tuple>(l[i]);
			string key=python::extract<string>(t[0]);
			self.attr(key.c_str())=t[1];
		}
	}
#endif


