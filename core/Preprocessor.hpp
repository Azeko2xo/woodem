#pragma once
#include<woo/lib/object/Object.hpp>

struct Scene; 
struct Preprocessor: public Object{
	virtual shared_ptr<Scene> operator()();
	// convert /tmp to c:/temp on windows
	// do nothing on Linux
	void fixWindowsPath(string& s);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Preprocessor,Object,"Subclasses of this class generate a Scene object when called, based on their attributes."
		, /* attrs */
		, /* ctor */
		, /* py */
		.def("__call__",&Preprocessor::operator())
	);
};
REGISTER_SERIALIZABLE(Preprocessor);
