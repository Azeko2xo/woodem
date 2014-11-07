#pragma once
#include<woo/lib/multimethods/Indexable.hpp>
#include<woo/lib/object/Object.hpp>
#include<woo/core/Master.hpp>
#include<woo/lib/pyutil/converters.hpp>

namespace py=boost::python;

#include<boost/thread/thread.hpp>

struct Scene;

struct Node;

struct NodeData: public Object{
	boost::mutex lock; // used by applyForceTorque

	// template to be specialized by derived classes
	template<typename Derived> struct Index; // { BOOST_STATIC_ASSERT(false); /* template must be specialized for derived NodeData types */ };

	virtual const char* getterName() const;
	virtual void setDataOnNode(Node&);

	#define woo_core_NodeData__CLASS_BASE_DOC_ATTRS_PY \
		NodeData,Object,"Data associated with some node.",/*attrs*/ \
		,/*py*/ .add_property("getterName",&NodeData::getterName); \
			woo::converters_cxxVector_pyList_2way<shared_ptr<NodeData>>();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_core_NodeData__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(NodeData);

#ifdef WOO_OPENGL
	struct GLViewInfo;
	struct Node;
#endif

// object representing what should be rendered at associated node
struct NodeVisRep: public Object{
	#ifdef WOO_OPENGL
		virtual void render(const shared_ptr<Node>&, const GLViewInfo*){};
	#endif
	#define woo_core_NodeVisRep__CLASS_BASE_DOC \
		NodeVisRep,Object,"Object representing what should be rendered at associated node (abstract base class)."
	WOO_DECL__CLASS_BASE_DOC(woo_core_NodeVisRep__CLASS_BASE_DOC);
};
WOO_REGISTER_OBJECT(NodeVisRep);


struct Node: public Object, public Indexable{

	// indexing data items
	// allows to define non-casting accessors without paying runtime penalty for index lookup
	enum {ST_DEM=0,ST_GL,ST_CLDEM,ST_SPARC,ST_ANCF,/*always keep last*/ST_LAST }; // assign constants to data values
	//const char dataNames[][]={"dem","foo"}; // not yet used
	#if 0
		// allow runtime registration of additional data fields, which can be looked up (slow) by names
		static int dataIndexMax;
		int getDataIndexByName(const std::string& name);
		std::string getDataNameByIndex(int ix);
	#endif

	// generic access functions
	bool hasData(size_t ix){ assert(/*ix>=0&&*/ix<ST_LAST); return(/*ix>=0&&*/ix<data.size()&&data[ix]); }
	void setData(const shared_ptr<NodeData>& nd, size_t ix){ assert(/*ix>=0&&*/ ix<ST_LAST); if(ix>=data.size()) data.resize(ix+1); data[ix]=nd; }
	const shared_ptr<NodeData>& getData(size_t ix){ assert(/*ix>=0&&*/data.size()>ix); return data[ix]; }

	// templates to get data cast to correct type quickly
	// classes derived from NodeData should only specialize the NodeData::Index template to make those functions work
	template<class NodeDataSubclass>
	NodeDataSubclass& getData(){ return getData(NodeData::Index<NodeDataSubclass>::value)->template cast<NodeDataSubclass>(); }
	template<class NodeDataSubclass>
	const shared_ptr<NodeDataSubclass> getDataPtr(){ return static_pointer_cast<NodeDataSubclass>(getData(NodeData::Index<NodeDataSubclass>::value)); }
	template<class NodeDataSubclass>
	void setData(const shared_ptr<NodeDataSubclass>& d){ setData(d,NodeData::Index<NodeDataSubclass>::value); }
	template<class NodeDataSubclass>
	bool hasData(){ return hasData(NodeData::Index<NodeDataSubclass>::value); }

	// template for python access of nodal data
	template<typename NodeDataSubclass>
	static shared_ptr<NodeDataSubclass> pyGetData(const shared_ptr<Node>& n){ return n->hasData<NodeDataSubclass>() ? static_pointer_cast<NodeDataSubclass>(n->getData(NodeData::Index<NodeDataSubclass>::value)) : shared_ptr<NodeDataSubclass>(); }
	template<typename NodeDataSubclass>
	static void pySetData(const shared_ptr<Node>& n, const shared_ptr<NodeDataSubclass>& d){ n->setData<NodeDataSubclass>(d); }
	string pyStr() const WOO_CXX11_OVERRIDE;

	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);

	// transform point p from global to local coordinates
	Vector3r glob2loc(const Vector3r& p){ return ori.conjugate()*(p-pos); }
	// 
	Vector3r loc2glob(const Vector3r& p){ return ori*p+pos; }

	#define woo_core_Node__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		Node,Object,ClassTrait().doc("A point in space (defining local coordinate system), referenced by other objects.").section("","",{"NodeData"}), \
		((Vector3r,pos,Vector3r::Zero(),AttrTrait<>().lenUnit(),"Position in space (cartesian coordinates); origin :math:`O` of the local coordinate system.")) \
		((Quaternionr,ori,Quaternionr::Identity(),,"Orientation :math:`q` of this node.")) \
		((vector<shared_ptr<NodeData> >,data,,,"Array of data, ordered in globally consistent manner.")) \
		((shared_ptr<NodeVisRep>,rep,,,"What should be shown at this node when rendered via OpenGL; this data are also used in e.g. particle tracking, hence enable even in OpenGL-less builds as well.")) /* defined above, nonempty in OpenGL-enabled builds only */ \
		, /* ctor */ createIndex(); \
		, /* py */ WOO_PY_TOPINDEXABLE(Node) \
			.def("glob2loc",&Node::glob2loc,(py::arg("p")),"Transform point :math:`p` from global to node-local coordinates as :math:`q^*(p-O)q`, in code ``q.conjugate()*(p-O)``.") \
			.def("loc2glob",&Node::loc2glob,(py::arg("p")),"Transform point :math:`p_l` from node-local to global coordinates as :math:`q\\cdot p_l\\cdot q^*+O`, in code ``q*p+O``.")
	
	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_core_Node__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	REGISTER_INDEX_COUNTER(Node);
};
WOO_REGISTER_OBJECT(Node);


struct Field: public Object, public Indexable{
	Scene* scene; // backptr to scene; must be set by Scene!
	py::object py_getScene();
	virtual void selfTest(){};
	virtual Real critDt() { return Inf; }
	#define woo_core_Field__CLASS_BASE_DOC_ATTRS_CTOR_PY  \
		Field,Object,ClassTrait().doc("Spatial field described by nodes, their topology and associated values.").section("","",{"Node"}), \
		((vector<shared_ptr<Node> >,nodes,,AttrTrait<Attr::pyByRef>(),"Nodes referenced from this field.")) \
		/* ((shared_ptr<Topology>,topology,,,"How nodes build up cells, neighborhood and coonectivity information.")) */ \
		/* ((vector<shared_ptr<CellData> >,cells,,,"")) */ \
		, /* ctor */ scene=NULL; createIndex(); \
		, /* py */ \
			.add_property("scene",&Field::py_getScene,"Get associated scene object, if any (this function is dangerous in some corner cases, as it has to use raw pointer).") \
			.def("critDt",&Field::critDt,"Return critical (maximum numerically stable) timestep for this field. By default returns infinity (no critical timestep) but derived fields may override this function.") \
			WOO_PY_TOPINDEXABLE(Field) \
			; \
			woo::converters_cxxVector_pyList_2way<shared_ptr<Field>>();

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_core_Field__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	REGISTER_INDEX_COUNTER(Field);


	// return bounding box for this field, for the purposes of rendering
	// by defalt, returns bbox of Field::nodes, but derived fields may override this
	virtual AlignedBox3r renderingBbox() const;

	// replaced by regular virtual function of Engine
	#if 0
	// nested mixin class
	struct Engine{
		// say whether a particular field is acceptable by this engine
		// each field defines class inherited from Field::Engine,
		// and it is then inherited _privately_ (or as protected,
		// to include all subclasses, as e.g. Engine itself does).
		// in this way, diamond inhertiace is avoided
		virtual bool acceptsField(Field*){ cerr<<"-- acceptsField not overridden."<<endl; return true; };
	};
	#endif
};
WOO_REGISTER_OBJECT(Field);
