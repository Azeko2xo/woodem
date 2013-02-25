#include<woo/lib/object/Object.hpp>

#if 0
static void Object_setAttr(py::object self, py::str name, py::object value){
#if 0
	py::dict d=py::extract<py::dict>(py::getattr(self,"__dict__"));
	if(d.has_key(name))
#endif
	{ py::setattr(self,name,value); return; }
#if 0
	woo::AttributeError(("Class "+py::extract<std::string>(py::getattr(py::getattr(self,"__class__"),"__name__"))()+" does not have attribute "+py::extract<std::string>(name)()+".").c_str());
#endif
}
#endif
namespace woo {

vector<py::object> Object::derivedCxxClasses;
py::list Object::getDerivedCxxClasses(){ py::list ret; for(py::object c: derivedCxxClasses) ret.append(c); return ret; }

void Object::pyRegisterClass() {
	checkPyClassRegistersItself("Object");
	py::class_<Object, shared_ptr<Object>, boost::noncopyable > classObj("Object");
	classObj
		.def("__str__",&Object::pyStr).def("__repr__",&Object::pyStr)
		.def("dict",&Object::pyDict,"Return dictionary of attributes.")
		//.def("_attrTraits",&Object::pyAttrTraits,(py::arg("parents")=true),"Return list of attribute traits.")
		.def("updateAttrs",&Object::pyUpdateAttrs,"Update object attributes from given dictionary")
		#if 1
			/* boost::python pickling support, as per http://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/pickle.html */ 
			.def("__getstate__",&Object::pyDict).def("__setstate__",&Object::pyUpdateAttrs)
			.add_property("__safe_for_unpickling__",&Object::getClassName,"just define the attr, return some bogus data")
			.add_property("__getstate_manages_dict__",&Object::getClassName,"just define the attr, return some bogus data")
		#endif
		.def("save",&Object::boostSave,py::arg("filename"))
		.def("_boostLoad",&Object::boostLoad,py::arg("filename")).staticmethod("_boostLoad")
		//.def_readonly("_derivedCxxClasses",&Object::derivedCxxClasses)
		.add_static_property("_derivedCxxClasses",&Object::getDerivedCxxClasses)
		.add_property("_cxxAddr",&Object::pyCxxAddr)
		// setting attributes with protection of creating class instance mistakenly
		#if 0
			.def("__setattr__",&Object_setAttr)
		#endif
		// constructor with dictionary of attributes
		//.def("__init__",py::raw_constructor(Object_ctor_kwAttrs<Object>))
		// comparison operators
		.def(py::self == py::self)
		.def(py::self != py::self)
	;
	classObj.attr("_attrTraits")=py::list();
	//classObj.attr("_derivedCxxClasses")=Object::derivedCxxClasses;
}

void Object::checkPyClassRegistersItself(const std::string& thisClassName) const {
	if(getClassName()!=thisClassName) throw std::logic_error(("Class "+getClassName()+" does not register with WOO_CLASS_BASE_DOC_ATTR*, would not be accessible from python.").c_str());
}

void Object::pyUpdateAttrs(const py::dict& d){	
	py::list l=d.items(); size_t ll=py::len(l); if(ll==0) return;
	for(size_t i=0; i<ll; i++){
		py::tuple t=py::extract<py::tuple>(l[i]);
		string key=py::extract<string>(t[0]);
		pySetAttr(key,t[1]);
	}
	callPostLoad(NULL);
}

};
