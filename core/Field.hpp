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
	// bits for blockedDOFs
	enum {DOF_NONE=0,DOF_X=1,DOF_Y=2,DOF_Z=4,DOF_RX=8,DOF_RY=16,DOF_RZ=32};
	//! shorthand for all DOFs blocked
	static const unsigned DOF_ALL=DOF_X|DOF_Y|DOF_Z|DOF_RX|DOF_RY|DOF_RZ;
	//! shorthand for all displacements blocked
	static const unsigned DOF_XYZ=DOF_X|DOF_Y|DOF_Z;
	//! shorthand for all rotations blocked
	static const unsigned DOF_RXRYRZ=DOF_RX|DOF_RY|DOF_RZ; 

	//! Return DOF_* constant for given axis∈{0,1,2} and rotationalDOF∈{false(default),true}; e.g. axisDOF(0,true)==DOF_RX
	static unsigned axisDOF(int axis, bool rotationalDOF=false){return 1<<(axis+(rotationalDOF?3:0));}
	//! Return DOF_* constant for given DOF number ∈{0,1,2,3,4,5}
	static unsigned linDOF(int dof){ return 1<<(dof); }
	//! Getter of blockedDOFs for list of strings (e.g. DOF_X | DOR_RX | DOF_RZ → 'xXZ')
	std::string blockedDOFs_vec_get() const;
	//! Setter of blockedDOFs from string ('xXZ' → DOF_X | DOR_RX | DOF_RZ)
	void blockedDOFs_vec_set(const std::string& dofs);

	void addForceTorque(const Vector3r& f, const Vector3r& t){ boost::mutex::scoped_lock l(lock); force+=f; torque+=t; }
	bool isAspherical() const{ return !((inertia[0]==inertia[1] && inertia[1]==inertia[2])); }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(DynState,Serializable,"Dynamic state of node.",
		((Vector3r,vel,Vector3r::Zero(),,"Linear velocity."))
		((Vector3r,angVel,Vector3r::Zero(),,"Angular velocity."))
		((Real,mass,0,,"Associated mass."))
		((Vector3r,inertia,Vector3r::Zero(),,"Inertia along local (principal) axes"))
		((Vector3r,force,Vector3r::Zero(),,"Applied force"))
		((Vector3r,torque,Vector3r::Zero(),,"Applied torque"))
		((Vector3r,angMom,Vector3r::Zero(),,"Angular momentum (used with the aspherical integrator)"))
		((unsigned,blockedDOFs,0,,"blocked degrees of freedom"))
		, /*ctor*/
		, /*py*/ .add_property("blockedDOFs",&DynState::blockedDOFs_vec_get,&DynState::blockedDOFs_vec_set,"Degress of freedom where linear/angular velocity will be always constant (equal to zero, or to an user-defined value), regardless of applied force/torque. String that may contain 'xyzXYZ' (translations and rotations).")

	);
};
REGISTER_SERIALIZABLE(DynState);

class Node: public Serializable, public Indexable{
	// Vector3r& getPos(); void setPos(const Vector3r&);
	//Vector3r&
	//Vector3r&
	//Vector3r&
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Node,Serializable,"A point in space, referenced by other objects.",
		((Vector3r,pos,Vector3r::Zero(),,"Position in space (cartesian coordinates)."))
		((Quaternionr,ori,Quaternionr::Identity(),,"Orientation of this node."))
		((shared_ptr<DynState>,dyn,,,"Dynamic state of the node (not defined by default)"))
		, /* ctor */ createIndex();
		, /* py */ YADE_PY_TOPINDEXABLE(Node)
		// .add_property("pos",py::make_function(&Node::getPos,py::return_internal_reference<>),py::make_function(&Node::setPos));
		//.add_property("ori",&Node::getOri)
		//.add_property("vel",&Node::getVel)
		//.add_property("angVel",&Node::getAngVel)
		//.add_property("bDof",getBlockedDOFs,setBlockedDOFs);
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
