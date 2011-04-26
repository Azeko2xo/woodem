#pragma once
#include<yade/lib/multimethods/Indexable.hpp>
#include<yade/lib/serialization/Serializable.hpp>
#include<yade/core/Dispatcher.hpp>

class Constraint: public Serializable{	
	YADE_CLASS_BASE_DOC_ATTRS(Constraint,Serializable,"Object defining constraint on motion of a node; this is an abstract class which is not to be used directly.",
	);
};
REGISTER_SERIALIZABLE(Constraint);

class Node: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Node,Serializable,"A point in space, referenced by other objects.",
		((Vector3r,pos,Vector3r::Zero(),,"Position in space (cartesian coordinates)."))
		((Quaternionr,ori,Quaternionr::Identity(),,"Orientation of this node; unused in some fields."))
		, /* ctor */
		, /* py */ YADE_PY_TOPINDEXABLE(Node); createIndex();
	);
	REGISTER_INDEX_COUNTER(Node);
};
REGISTER_SERIALIZABLE(Node);

class NodeData: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(NodeData,Serializable,"Field data associated with a node.",
		((shared_ptr<Constraint>,constraint,,,"Constraint associated with this node."))
		, /* ctor */
		, /* py */ YADE_PY_TOPINDEXABLE(NodeData);
	);
	REGISTER_INDEX_COUNTER(NodeData);
};
REGISTER_SERIALIZABLE(NodeData);

class Field: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Field,Serializable,"Spatial field described by nodes, their topology and associated values.",
		// ((shared_ptr<Cell>,cell,,,"Periodicity of space, if any"))
		((vector<shared_ptr<Node> >,nodes,,,"Nodes referenced from this field."))
		((vector<shared_ptr<NodeData> >,nodeData,,,"Nodal data, associated to nodes with the same index."))
		//((shared_ptr<Topology>,topology,,,"How nodes build up cells, neighborhood and coonectivity information."))
		//((vector<shared_ptr<CellData> >,cells,,,""))
		, /* ctor */
		, /* py */ YADE_PY_TOPINDEXABLE(Field);
	);
	REGISTER_INDEX_COUNTER(Field);
};
REGISTER_SERIALIZABLE(Field);
