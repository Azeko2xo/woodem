#pragma once
#include<yade/lib/multimethods/Indexable.hpp>
#include<yade/lib/serialization/Serializable.hpp>
#include<yade/core/Omega.hpp>

namespace py=boost::python;

#include<boost/thread/thread.hpp>

class Scene;

struct Constraint: public Serializable{	
	virtual void applyConstraint(Vector3r& newPos, Vector3r& newOri){};
	YADE_CLASS_BASE_DOC_ATTRS(Constraint,Serializable,"Object defining constraint on motion of a node; this is an abstract class which is not to be used directly.",
	);
};
REGISTER_SERIALIZABLE(Constraint);

struct NodeData: public Serializable{
	boost::mutex lock; // used by applyForceTorque
	YADE_CLASS_BASE_DOC(NodeData,Serializable,"Data associated with some node.");
};
REGISTER_SERIALIZABLE(NodeData);

struct Node: public Serializable, public Indexable{
	// indexing data items
	// allows to define non-casting accessors without paying runtime penalty for index lookup
	enum {ST_DEM=0,ST_SPARC,/*always keep last*/ST_LAST }; // assign constants to data values
	//const char dataNames[][]={"dem","foo"}; // not yet used
	#if 0
		// allow runtime registration of additional data fields, which can be looked up (slow) by names
		static int dataIndexMax;
		int getDataIndexByName(const std::string& name);
		std::string getDataNameByIndex(int ix);
	#endif

	// generic access functions
	bool hasData(size_t ix){ assert(ix>=0&&ix<ST_LAST); return(ix>=0&&ix<data.size()&&data[ix]); }
	void setData(const shared_ptr<NodeData>& nd, size_t ix){ assert(ix>=0 && ix<ST_LAST); if(ix>=data.size()) data.resize(ix+1); data[ix]=nd; }
	const shared_ptr<NodeData>& getData(size_t ix){ assert(ix>=0 && data.size()>ix); return data[ix]; }
	// should be specialized by relevant fields to return data cast to the right type
	// see e.g. DemField class & DemData
	template<class NodeDataSubclass> NodeDataSubclass& getData();
	template<class NodeDataSubclass> void setData(const shared_ptr<NodeDataSubclass>&);
	template<class NodeDataSubclass> bool hasData();


	#undef NODE_ACCESS_PROXY
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Node,Serializable,"A point in space, referenced by other objects.",
		((Vector3r,pos,Vector3r::Zero(),,"Position in space (cartesian coordinates)."))
		((Quaternionr,ori,Quaternionr::Identity(),,"Orientation of this node."))
		((vector<shared_ptr<NodeData> >,data,,,"Array of data, ordered in globally consistent manner."))
		, /* ctor */ createIndex();
		, /* py */ YADE_PY_TOPINDEXABLE(Node)
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

struct Field: public Serializable, public Indexable{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Field,Serializable,"Spatial field described by nodes, their topology and associated values.",
		((Scene*,scene,NULL,Attr::hidden,"Backptr to scene")) // must be set by Scene!
		((vector<shared_ptr<Node> >,nodes,,,"Nodes referenced from this field."))
		//((vector<shared_ptr<NodeData> >,nodeData,,,"Nodal data, associated to nodes with the same index."))
		//((shared_ptr<Topology>,topology,,,"How nodes build up cells, neighborhood and coonectivity information."))
		//((vector<shared_ptr<CellData> >,cells,,,""))
		, /* ctor */
		, /* py */
			YADE_PY_TOPINDEXABLE(Field)
	);
	REGISTER_INDEX_COUNTER(Field);
	// nested mixin class
	class Engine{
		// say whether a particular field is acceptable by this engine
		// each field defines class inherited from Field::Engine,
		// and it is then inherited _privately_ (or as protected,
		// to include all subclasses, as e.g. Engine itself does).
		// in this way, diamond inhertiace is avoided
		virtual bool acceptsField(Field*){ return true; }
	};
};
REGISTER_SERIALIZABLE(Field);
