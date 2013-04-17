#pragma once
#include<woo/lib/base/Types.hpp>

struct LabelMapper: public Object{
	typedef std::map<string,py::object> StrPyMap;
	typedef std::map<string,shared_ptr<Object>> StrWooMap;
	typedef std::map<string,vector<shared_ptr<Object>>> StrWooSeqMap;

	void __setitem__woo(const string& label, const shared_ptr<Object>& o);
	void __setitem__wooSeq(const string& label, const vector<shared_ptr<Object>>& o);
	void __setitem__py(const string& label, py::object o);
	void __setitem__(const string& label, py::object o);
	bool __contains__(const string& label) const;
	enum{NOWHERE=0,IN_PY,IN_WOO,IN_WOO_SEQ};
	int findWhere(const string& label) const; // return in which container an object is to be found
	enum{LABEL_PLAIN=0,LABEL_SEQ};
	int labelType(const string& label, string& l0, int& index) const;
	py::object __getitem__(const string& label);
	// shared_ptr<Object>& __getitem__woo(const string& label);
	void __delitem__(const string& label);

	py::list pyKeys() const;
	py::list pyItems() const;
	int __len__() const;

	DECLARE_LOGGER;

	static string pyAsStr(const py::object& o);

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(LabelMapper,Object,"Object mapping labels to :obj:`woo.Object`, lists of :obj:`woo.Object` or Python's objects.",
		((StrPyMap,pyMap,,AttrTrait<>().hidden(),"Map names to python objects"))
		((StrWooMap,wooMap,,AttrTrait<>().hidden(),"Map names to woo objects"))
		((StrWooSeqMap,wooSeqMap,,AttrTrait<>().hidden(),"Map names to sequences of woo objects"))
		, /*ctor*/
		, /*py*/
			.def("__setitem__",&LabelMapper::__setitem__)
			.def("__getitem__",&LabelMapper::__getitem__)
			.def("__delitem__",&LabelMapper::__delitem__)
			.def("__contains__",&LabelMapper::__contains__)
			.def("items",&LabelMapper::pyItems)
			.def("keys",&LabelMapper::pyKeys)
			.def("__len__",&LabelMapper::__len__)
	);
};
REGISTER_SERIALIZABLE(LabelMapper);
