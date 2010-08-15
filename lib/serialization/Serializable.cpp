/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "Serializable.hpp"


void Serializable::pyRegisterClass(boost::python::object _scope) {
	checkPyClassRegistersItself("Serializable");
	boost::python::scope thisScope(_scope); 
	python::class_<Serializable, shared_ptr<Serializable>, noncopyable >("Serializable")
		.add_property("name",&Serializable::getClassName,"Name of the class").def("__str__",&Serializable::pyStr).def("__repr__",&Serializable::pyStr).def("postProcessAttributes",&Serializable::postProcessAttributes,(python::arg("deserializing")=true),"Call Serializable::postProcessAttributes c++ method.")
		.def("dict",&Serializable::pyDict,"Return dictionary of attributes.")
		.def("updateAttrs",&Serializable::pyUpdateAttrs,"Update object attributes from given dictionary").def("updateExistingAttrs",&Serializable::pyUpdateExistingAttrs,"Update object attributes from given dictionary, skipping those that the instance doesn't have. Return list of attributes that did *not* exist and were not updated.")
		.def("clone",&Serializable_clone<Serializable>,python::arg("attrs")=python::dict(),"Return clone of the instance, created by copying values of all attributes.")
		/* boost::python pickling support, as per http://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/pickle.html */ 
		.def("__getstate__",&Serializable::pyDict).def("__setstate__",&Serializable::pyUpdateAttrs)
		.add_property("__safe_for_unpickling__",&Serializable::getClassName,"just define the attr, return some bogus data")
		.add_property("__getstate_manages_dict__",&Serializable::getClassName,"just define the attr, return some bogus data")
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



void Serializable::pyUpdateAttrs(const python::dict& d){
	python::list l=d.items(); size_t ll=python::len(l);
	for(size_t i=0; i<ll; i++){
		python::tuple t=python::extract<python::tuple>(l[i]);
		string key=python::extract<string>(t[0]);
		pySetAttr_nowarn(key,t[1]);
	}
	if(ll>0) postProcessAttributes(/*deserializing*/true); // to make sure
}

python::list Serializable::pyUpdateExistingAttrs(const python::dict& d){
	python::list ret; python::list l=d.items(); size_t ll=python::len(l);
	for(size_t i=0; i<ll; i++){
		python::tuple t=python::extract<python::tuple>(l[i]);
		string key=python::extract<string>(t[0]);
		if(pyHasKey(key)) pySetAttr_nowarn(key,t[1]); else ret.append(t[0]);
	}
	if(ll>0) postProcessAttributes(/*deserializing*/true); // to make sure
	return ret; 
}



