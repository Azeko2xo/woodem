#pragma once
#include<woo/lib/base/Types.hpp>
#include<woo/lib/object/Object.hpp>
#

struct LabelMapper: public Object{
	typedef std::map<string,py::object> StrPyMap;
	typedef std::map<string,shared_ptr<Object>> StrWooMap;
	typedef std::map<string,vector<shared_ptr<Object>>> StrWooSeqMap;

	void __setitem__woo(const string& label, const shared_ptr<Object>& o);
	void __setitem__wooSeq(const string& label, const vector<shared_ptr<Object>>& o);
	void __setitem__py(const string& label, py::object o);
	void __setitem__(const string& label, py::object o);
	bool __contains__(const string& label) const;
	enum{NOWHERE=0,IN_PY,IN_WOO,IN_WOO_SEQ,IN_MOD};
	int whereIs(const string& label) const; // return in which container an object is to be found
	enum{LABEL_PLAIN=0,LABEL_SEQ};
	int labelType(const string& label, string& l0, int& index) const;
	py::object __getitem__(const string& label);
	// shared_ptr<Object>& __getitem__woo(const string& label);
	void __delitem__(const string& label);

	template<typename listTuple>
	bool sequence_check_setitem(const string& label, py::object o);

	py::list pyKeys() const;
	py::list pyItems() const;
	int __len__() const;
	py::list __dir__(const string& prefix="") const;

	WOO_DECL_LOGGER;

	static string pyAsStr(const py::object& o);

	void setWritable(const string& label, bool writable);
	bool isWritable(const string& label) const;
	void newModule(const string& label);
	void ensureUsedModsOk(const string& label);

	#define woo_core_LabelMapper__CLASS_BASE_DOC_ATTRS_PY \
		LabelMapper,Object,"Map labels to :obj:`woo.Object`, lists of :obj:`woo.Object` or Python's objects, while preserving reference-counting of c++ objects during save/loads. This object is exposed as :obj:`Scene.labels` (with dictionary-like access) and :obj:`Scene.lab` (with attribute access) and presents a simulation-bound persistent namespace for arbitrary objects.", \
		((StrPyMap,pyMap,,AttrTrait<Attr::hidden>(),"Map names to python objects")) \
		((StrWooMap,wooMap,,AttrTrait<Attr::hidden>(),"Map names to woo objects")) \
		((StrWooSeqMap,wooSeqMap,,AttrTrait<Attr::hidden>(),"Map names to sequences of woo objects")) \
		((std::set<string>,modSet,,AttrTrait<Attr::hidden>(),"Set of pseudo-modules names (fully qualified)")) \
		((std::set<string>,writables,,AttrTrait<Attr::hidden>(),"Set of writable names (without warning)")) \
		, /*py*/ \
			.def("__setitem__",&LabelMapper::__setitem__) \
			.def("__getitem__",&LabelMapper::__getitem__) \
			.def("__delitem__",&LabelMapper::__delitem__) \
			.def("__contains__",&LabelMapper::__contains__) \
			.def("items",&LabelMapper::pyItems) \
			.def("keys",&LabelMapper::pyKeys) \
			.def("__len__",&LabelMapper::__len__) \
			.def("__dir__",&LabelMapper::__dir__,(py::arg("prefix")="")) \
			.def("_whereIs",&LabelMapper::whereIs,"Return symbolic constant specifying where is the object with the label given stored (for testing only).") \
			.def("_setWritable",&LabelMapper::setWritable,(py::arg("label"),py::arg("writable")=true),"Make given label writable (without warning), or remove the writable flag (with *writable*=False).\n\n.. note:: Pseudomodules cannot be made writable.") \
			.def("_isWritable",&LabelMapper::isWritable,(py::arg("label")),"Say whether given label is writable. Raise exception when the label does not exist.") \
			.def("_newModule",&LabelMapper::newModule) \
			; \
			_classObj.attr("nowhere")=(int)LabelMapper::NOWHERE; \
			_classObj.attr("inWoo")=(int)LabelMapper::IN_WOO; \
			_classObj.attr("inWooSeq")=(int)LabelMapper::IN_WOO_SEQ; \
			_classObj.attr("inPy")=(int)LabelMapper::IN_PY; \
			_classObj.attr("inMod")=(int)LabelMapper::IN_MOD; 
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_core_LabelMapper__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(LabelMapper);
