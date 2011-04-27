#pragma once
#include<yade/lib/multimethods/Indexable.hpp>
#include<yade/lib/serialization/Serializable.hpp>
#include<yade/core/Dispatcher.hpp>

#include<boost/thread/thread.hpp>

class Scene;

class Constraint: public Serializable{	
	virtual void applyConstraint(Vector3r& newPos, Vector3r& newOri){};
	YADE_CLASS_BASE_DOC_ATTRS(Constraint,Serializable,"Object defining constraint on motion of a node; this is an abstract class which is not to be used directly.",
	);
};
REGISTER_SERIALIZABLE(Constraint);

class DynState: public Serializable{
	boost::mutex lock; // used by applyForceTorque
public:
	void applyForceTorque(const Vector3r& f, const Vector3r& t){ boost::mutex::scoped_lock l(lock); force+=f; torque+=t; }
	YADE_CLASS_BASE_DOC_ATTRS(DynState,Serializable,"Dynamic state of node.",
		((Vector3r,vel,Vector3r::Zero(),,"Linear velocity."))
		((Vector3r,angVel,Vector3r::Zero(),,"Angular velocity."))
		((Real,mass,0,,"Associated mass."))
		((Vector3r,inertia,Vector3r::Zero(),,"Inertia along local (principal) axes"))
		((Vector3r,force,Vector3r::Zero(),,"Applied force"))
		((Vector3r,torque,Vector3r::Zero(),,"Applied torque"))
	);
};
REGISTER_SERIALIZABLE(DynState);

class Node: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Node,Serializable,"A point in space, referenced by other objects.",
		((Vector3r,pos,Vector3r::Zero(),,"Position in space (cartesian coordinates)."))
		((Quaternionr,ori,Quaternionr::Identity(),,"Orientation of this node."))
		((shared_ptr<DynState>,dyn,,,"Dynamic state of the node (not defined by default)"))
		, /* ctor */
		, /* py */ YADE_PY_TOPINDEXABLE(Node); createIndex();
	);
	REGISTER_INDEX_COUNTER(Node);
};
REGISTER_SERIALIZABLE(Node);

#if 0
class NodeData: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(NodeData,Serializable,"Field data associated with a node.",
		((shared_ptr<Constraint>,constraint,,,"Constraint associated with this node."))
		, /* ctor */
		, /* py */ YADE_PY_TOPINDEXABLE(NodeData);
	);
	REGISTER_INDEX_COUNTER(NodeData);
};
REGISTER_SERIALIZABLE(NodeData);
#endif

class Field: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Field,Serializable,"Spatial field described by nodes, their topology and associated values.",
		((Scene*,scene,NULL,Attr::hidden,"Backptr to scene")) // must be set by Scene!
		((vector<shared_ptr<Node> >,nodes,,,"Nodes referenced from this field."))
		//((vector<shared_ptr<NodeData> >,nodeData,,,"Nodal data, associated to nodes with the same index."))
		//((shared_ptr<Topology>,topology,,,"How nodes build up cells, neighborhood and coonectivity information."))
		//((vector<shared_ptr<CellData> >,cells,,,""))
		, /* ctor */
		, /* py */ YADE_PY_TOPINDEXABLE(Field);
	);
	REGISTER_INDEX_COUNTER(Field);
};
REGISTER_SERIALIZABLE(Field);
