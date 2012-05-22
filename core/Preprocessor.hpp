#pragma once
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
