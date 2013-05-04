#include<woo/core/LabelMapper.hpp>
#include<woo/lib/pyutil/except.hpp>
#include<woo/core/Master.hpp>
#include<boost/regex.hpp>


WOO_PLUGIN(core,(LabelMapper));
CREATE_LOGGER(LabelMapper);

int LabelMapper::findWhere(const string& label) const {
	if(pyMap.find(label)!=pyMap.end()) return IN_PY;
	if(wooMap.find(label)!=wooMap.end()) return IN_WOO;
	if(wooSeqMap.find(label)!=wooSeqMap.end()) return IN_WOO_SEQ;
	return NOWHERE;
}

bool LabelMapper::__contains__(const string& label) const {
	return findWhere(label)!=NOWHERE;
}

py::list LabelMapper::pyKeys() const{
	py::list ret;
	for(auto& kv: wooMap) ret.append(kv.first);
	for(auto& kv: wooSeqMap) ret.append(kv.first);
	for(auto& kv: pyMap) ret.append(kv.first);
	return ret;
}

int LabelMapper::__len__() const {
	return wooMap.size()+wooSeqMap.size()+pyMap.size();
}

py::list LabelMapper::pyItems() const{
	py::list ret;
	for(auto& kv: wooMap) ret.append(py::make_tuple(kv.first,kv.second));
	for(auto& kv: wooSeqMap) ret.append(py::make_tuple(kv.first,kv.second));
	for(auto& kv: pyMap) ret.append(py::make_tuple(kv.first,kv.second));
	return ret;
}


int LabelMapper::labelType(const string& label, string& lab0, int& index) const {
	boost::smatch match;
	if(boost::regex_match(label,match,boost::regex("([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\[([0-9]+)\\]"))){
		lab0=match[1];
		index=lexical_cast<long>(match[2]);
		if(index<0) woo::ValueError("LabelMapper: label '"+lab0+"' specifies non-positive index "+to_string(index));
		return LABEL_SEQ;
	}
	if(boost::regex_match(label,match,boost::regex("[a-zA-Z_][a-zA-Z0-9_]*"))){
		return LABEL_PLAIN;
	}
	woo::NameError("LabelMapper: label '"+label+" is not a valid python identifier name.");
	return -1; // unreachable
}

string LabelMapper::pyAsStr(const py::object& o){
	std::ostringstream oss;
	if(PyObject_HasAttrString(o.ptr(),"__repr__")){
		py::extract<string> ex(o.attr("__repr__")()); // call __repr__
		if(ex.check()) return ex();  // see if the result is a string (should be)
		// if not, proceed to the dummy version below
	}
	oss<<"<"<<py::extract<string>(o.attr("__class__").attr("__name__"))()<<" at "<<o.ptr()<<">";
	return oss.str();
}

template<typename listTuple>
bool LabelMapper::sequence_check_setitem(const string& label, py::object o){
	py::extract<listTuple> exSeq(o);
	if(!exSeq.check()) return false;
	listTuple seq=exSeq();
	int isWoo=-1; // -1 is indeterminate, 0 is pure-python objects, 1 is woo objects
	if(py::len(seq)==0) woo::ValueError("Sequence given to LabelMapper is empty.");
	for(int i=0; i<py::len(seq); i++){
		// None can be both empty shared_ptr<Object> or None in python, so that one is not decisive
		if(py::object(seq[i]).is_none()) continue; 
		py::extract<shared_ptr<Object>> exi(seq[i]);
		if(exi.check()){
			if(isWoo==0) woo::ValueError("Sequence given to LabelMapper mixes python objects and woo.Object's.");
			isWoo=1;
		} else {
			if(isWoo==1) woo::ValueError("Sequence given to LabelMapper mixes python objects and woo.Object's.");
			isWoo=0;
		}
	}
	switch(isWoo){
		case -1: woo::ValueError("Unable to decide whether the sequence contains python objects or woo.Object's (containing only None)."); /*compiler happy*/ return false; 
		case 0: __setitem__py(label,o); return true;
		case 1: {
			vector<shared_ptr<Object>> vec; vec.reserve(py::len(seq));
			for(int i=0; i<py::len(seq); i++) vec.push_back(py::extract<shared_ptr<Object>>(seq[i])());
			__setitem__wooSeq(label,vec);
			return true;
		}
		// unreachable
		default: throw std::logic_error("LabelMapper::sequence_check_setitem: isWoo=="+to_string(isWoo)+"??");
	}
}


void LabelMapper::__setitem__(const string& label, py::object o){
	py::extract<shared_ptr<Object>> exObj(o);
	// woo objects
	if(exObj.check()){ __setitem__woo(label,exObj()); return; }
	// list of woo objects
	if(sequence_check_setitem<py::list>(label,o)) return;
	// tuple of woo objects
	if(sequence_check_setitem<py::tuple>(label,o)) return;
	// python object
	__setitem__py(label,o);
}

void LabelMapper::__setitem__woo(const string& label, const shared_ptr<Object>& o){
	string lab0=label; int index; 
	int type=labelType(label,lab0,index);
	int where=findWhere(label);
	if(type==LABEL_PLAIN){
		switch(where){
			case IN_WOO:
				assert(wooMap.find(label)!=wooMap.end());
				assert(type==LABEL_PLAIN); // such a label should not have been created at all
				if(wooMap[label].get()!=o.get()){
					LOG_WARN("Label '"<<label<<"' overwrites "<<wooMap[label]->pyStr()<<" with "<<o->pyStr());
				}
				break;
			case IN_PY:
				assert(pyMap.find(label)!=pyMap.end());
				assert(type==LABEL_PLAIN);
				LOG_WARN("Label '"<<label<<"' changes type from pure python object to woo.Object, overwriting "<<pyAsStr(pyMap[label])<<" with "<<o->pyStr());
				pyMap.erase(label);
				break;
			case IN_WOO_SEQ:
				assert(wooSeqMap.find(label)!=wooSeqMap.end());
				LOG_WARN("Label '"<<label<<"' changes types from sequence of woo.Object to woo.Object, deleting "<<wooSeqMap.find(label)->second.size()<<" items in the sequence.");
				wooSeqMap.erase(label);
				break;
			/* NOWEHERE is OK */
		};
		wooMap[label]=o;
	} else {
		assert(type==LABEL_SEQ);
		where=findWhere(lab0); // find base label again
		switch(where){
			case IN_WOO:
				LOG_WARN("Sequence label '"<<lab0<<"["<<index<<"]' creates a new sequence, deleting '"<<lab0<<"' with "<<wooMap[lab0]->pyStr());
				wooMap.erase(lab0);
				break;
			case IN_PY:
				LOG_WARN("Sequence label '"<<lab0<<"["<<index<<"]' creates a new sequence, deleting '"<<lab0<<"' with "<<pyAsStr(pyMap[lab0]));
				pyMap.erase(lab0);
				break;
			case NOWHERE: ;
			case IN_WOO_SEQ: ;// ok
		};
		auto& vec=wooSeqMap[lab0];
		if(vec.size()<=(size_t)index) vec.resize(index+1);
		vec[index]=o;
	}
}

void LabelMapper::__setitem__wooSeq(const string& label, const vector<shared_ptr<Object>>& oo){
	string lab0=label; int index; 
	int type=labelType(label,lab0,index);
	if(type==LABEL_SEQ) woo::NameError("Subscripted label '"+label+"' may not be assigned a sequence of woo.Objects!");
	int where=findWhere(label);
	string old;
	switch(where){
		case IN_WOO:
			old="an existing woo.Object "+wooMap[label]->pyStr();
			wooMap.erase(label);
			break;
		case IN_PY:
			old="an existing pure-python object "+pyAsStr(pyMap[label]);
			pyMap.erase(label);
			break;
		case IN_WOO_SEQ:
			old="an existing sequence with "+to_string(wooSeqMap[label].size())+"items";
			wooSeqMap.erase(label);
			break;
	};
	if(where!=NOWHERE) LOG_WARN("Label '"<<label<<"' overwrites "<<old<<" with a new sequence of "<<oo.size()<<" woo.Object's.");
	wooSeqMap[label]=oo;
};

void LabelMapper::__setitem__py(const string& label, py::object o){
	string lab0=label; int index; 
	int type=labelType(label,lab0,index);
	int where=findWhere(label);
	assert(!py::extract<woo::Object>(o).check());
	if(type==LABEL_PLAIN){
		switch(where){
			case IN_WOO:
				LOG_WARN("Label '"<<label<<" changes type from woo.Object to pure-python object, overwriting "<<wooMap[label]->pyStr()<<" with "<<pyAsStr(o));
				wooMap.erase(label);
				break;
			case IN_PY:
				if(o.ptr()!=pyMap[label].ptr()) LOG_WARN("Label '"<<label<<" overwrites "<<pyAsStr(pyMap[label])<<" with "<<pyAsStr(o));
				break;
			case IN_WOO_SEQ:
				LOG_WARN("Label '"<<label<<"' changes type from sequence of woo.Object to pure-python object, deleting "<<wooSeqMap.find(label)->second.size()<<" items in the sequence.");
				wooSeqMap.erase(label);
			// NOWHERE is OK
		};
		pyMap[label]=o;
	} else {
		assert(type==LABEL_SEQ);
		where=findWhere(lab0); // find base label again
		switch(where){
			case IN_WOO:
				LOG_WARN("Sequence label '"<<lab0<<"["<<index<<"]' overwrites '"<<lab0<<"' containing "<<wooMap[lab0]->pyStr()<<" with a new python list");
				wooMap.erase(lab0);
				break;
			case IN_WOO_SEQ:
				LOG_WARN("Sequence label '"<<lab0<<"["<<index<<"]' overwrites '"<<lab0<<"' containing "<<wooMap[lab0]->pyStr()<<" with a new python list");
				break;
			case IN_PY:
				if(!py::extract<py::list>(pyMap[lab0]).check()){
					LOG_WARN("Sequence label '"<<lab0<<"["<<index<<"]' overwrites non-list python object "<<pyAsStr(pyMap[lab0]));
					pyMap[lab0]=py::list();
				} else {
				}
				break;
			case NOWHERE:
				pyMap[lab0]=py::list();
		}
		assert(py::extract<py::list>(pyMap[lab0]).check());
		py::list lst=py::extract<py::list>(pyMap[lab0])();
		// extend the list so that it is long enough
		while(py::len(lst)<index+1) lst.append(py::object());
		if(!py::object(lst[index]).is_none()) LOG_WARN("Label '"<<lab0<<"["<<index<<"]' overwrites old item "<<pyAsStr(lst[index])<<" with "<<pyAsStr(o));
		lst[index]=o;
	}
}

py::object LabelMapper::__getitem__(const string& label){
	int where=findWhere(label);
	switch(where){
		case NOWHERE: woo::NameError("No such label: '"+label+"'"); break;
		case IN_WOO: return py::object(wooMap[label]); break;
		case IN_PY: return pyMap[label]; break;
		case IN_WOO_SEQ: return py::object(wooSeqMap[label]); break; // should convert to list automatically
	};
	abort(); // unreachable
}

void LabelMapper::__delitem__(const string& label){
	int where=findWhere(label);
	switch(where){
		case NOWHERE: woo::NameError("No such label: '"+label+"'"); break;
		case IN_WOO: wooMap.erase(label); break;
		case IN_PY: pyMap.erase(label); break;
		case IN_WOO_SEQ: wooSeqMap.erase(label); break;
	};
}
