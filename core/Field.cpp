#include<woo/core/Field.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/core/Scene.hpp>

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_core_Node__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_core_Field__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_core_NodeData__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC(woo_core_NodeVisRep__CLASS_BASE_DOC);

WOO_PLUGIN(core,(Node)(Field)(NodeData)(NodeVisRep));

AlignedBox3r Field::renderingBbox() const {
	AlignedBox3r b;
	for(const shared_ptr<Node>& n: nodes){
		if(!n) continue;
		b.extend(n->pos);
	}
	return b;
}

const char* NodeData::getterName() const { throw std::runtime_error(pyStr()+" does not reimplement NodeData::getterName."); }
void NodeData::setDataOnNode(Node&){ throw std::runtime_error(pyStr()+" does not reimplement NodeData::setDataOnNode."); }

string Node::pyStr() const {
	std::ostringstream o;
	o<<"<"+getClassName()<<" @ "<<this<<", at ("<<pos.transpose()<<")>";
	return o.str();
}

void Node::pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){
	for(const char* name:{"dem","gl","sparc","clDem"}){
		if(!kw.has_key(name)) continue;
		auto& d=py::extract<shared_ptr<NodeData>>(kw[name])();
		if(d->getterName()!=name) throw std::runtime_error("Node: mismatch passing "+string(name)+"="+d->pyStr()+": the shorthand for this type should be "+d->getterName());
		d->setDataOnNode(*this);
		py::api::delitem(kw,name);
	}
}


py::object Field::py_getScene(){
	if(!scene) return py::object();
	else return py::object(static_pointer_cast<Scene>(scene->shared_from_this()));
}


// Node::dataIndexMax=Node::ST_LAST;

