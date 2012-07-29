#pragma once
#include<woo/lib/multimethods/Indexable.hpp>
#include<woo/lib/object/Object.hpp>
#include<woo/core/Master.hpp>

namespace py=boost::python;

#include<boost/thread/thread.hpp>

class Scene;

struct Constraint: public Object{	
	virtual void applyConstraint(Vector3r& newPos, Vector3r& newOri){};
	WOO_CLASS_BASE_DOC_ATTRS(Constraint,Object,"Object defining constraint on motion of a node; this is an abstract class which is not to be used directly.",
	);
};
REGISTER_SERIALIZABLE(Constraint);

struct NodeData: public Object{
	boost::mutex lock; // used by applyForceTorque

	// template to be specialized by derived classes
	template<typename Derived> struct Index; // { BOOST_STATIC_ASSERT(false); /* template must be specialized for derived NodeData types */ };

	WOO_CLASS_BASE_DOC(NodeData,Object,"Data associated with some node.");
};
REGISTER_SERIALIZABLE(NodeData);

#ifdef WOO_OPENGL
struct GLViewInfo;
struct Node;

// object representing what should be rendered at associated node
struct NodeGlRep: public Object{
	//boost::mutex lock; // for rendering
	//void safeRender(const shared_ptr<Node>& n, GLViewInfo* glvi){ boost::mutex::scoped_lock l(lock); render(n,glvi); }
	virtual void render(const shared_ptr<Node>&, GLViewInfo*){};
	WOO_CLASS_BASE_DOC(NodeGlRep,Object,"Object representing what should be rendered at associated node (abstract base class).");
};
REGISTER_SERIALIZABLE(NodeGlRep);

class QglMovableObject;
struct ScalarRange: public Object{
	shared_ptr<QglMovableObject> movablePtr;
	void reset();
	Vector3r color(Real v);
	Real maxAbs(){ if(!isOk()) abort(); throw std::runtime_error("ScalarRange::maxAbs(): uninitialized object, call with value to adjust range.");  return max(abs(mnmx[0]),abs(mnmx[1])); }
	Real maxAbs(Real v){ adjust(v); return max(abs(mnmx[0]),abs(mnmx[1])); }
	bool isOk(){ return(mnmx[0]<mnmx[1]); }
	// return value on the range, given normalized value
	Real normInv(Real norm){ return mnmx[0]+norm*(mnmx[1]-mnmx[0]); } 
	Real norm(Real v);
	void adjust(const Real& v);
	// called only when mnmx is manipulated
	void postLoad(const ScalarRange&){ if(isOk()) autoAdjust=false; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ScalarRange,Object,"Store and share range of scalar values",
		((Vector2r,mnmx,Vector2r(std::numeric_limits<Real>::infinity(),-std::numeric_limits<Real>::infinity()),AttrTrait<Attr::triggerPostLoad>(),"Packed minimum and maximum values; adjusting from python sets :ref:`autoAdjust` to false automatically."))
		((bool,autoAdjust,true,,"Automatically adjust range using given values."))
		((bool,sym,false,,"Force maximum to be negative of minimum and vice versa (only with autoadjust)"))
		((Vector2i,dispPos,Vector2i(-1000,-1000),AttrTrait<>().noGui(),"Where is this range displayed on the OpenGL canvas; initially out of range, will be reset automatically."))
		((Real,length,-.5,AttrTrait<>().noGui(),"Length on the display; if negative, it is fractional relative to view width/height"))
		((bool,landscape,false,AttrTrait<>().noGui(),"Make the range display with landscape orientation"))
		((std::string,label,,,"Short name of this range."))
		((int,cmap,-1,AttrTrait<>().colormapChoice(),"Colormap index to be used; -1 means to use the default colormap (see *O.lsCmap*, *O.setCmap*)"))
		, /* ctor */
		, /* py */
			.def("norm",&ScalarRange::norm,"Return value of the argument normalized to 0..1 range; the value is not clamped to 0..1 however: if autoAdjust is false, it can fall outside.")
			.def("reset",&ScalarRange::reset)
	);
};
REGISTER_SERIALIZABLE(ScalarRange);
#endif



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

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Node,Object,"A point in space, referenced by other objects.",
		((Vector3r,pos,Vector3r::Zero(),AttrTrait<>().lenUnit(),"Position in space (cartesian coordinates)."))
		((Quaternionr,ori,Quaternionr::Identity(),,"Orientation of this node."))
		((vector<shared_ptr<NodeData> >,data,,,"Array of data, ordered in globally consistent manner."))
		#ifdef WOO_OPENGL
			((shared_ptr<NodeGlRep>,rep,,,"What should be shown at this node when rendered via OpenGL."))
		#endif
		, /* ctor */ createIndex();
		, /* py */ WOO_PY_TOPINDEXABLE(Node)
	);
	REGISTER_INDEX_COUNTER(Node);
};
REGISTER_SERIALIZABLE(Node);


struct Field: public Object, public Indexable{
	Scene* scene; // backptr to scene; must be set by Scene!
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Field,Object,"Spatial field described by nodes, their topology and associated values.",
		// ((Scene*,scene,NULL,AttrTrait<Attr::hidden>(),"Backptr to scene")) // must be set by Scene!
		((vector<shared_ptr<Node> >,nodes,,AttrTrait<Attr::pyByRef>(),"Nodes referenced from this field."))
		//((vector<shared_ptr<NodeData> >,nodeData,,,"Nodal data, associated to nodes with the same index."))
		//((shared_ptr<Topology>,topology,,,"How nodes build up cells, neighborhood and coonectivity information."))
		//((vector<shared_ptr<CellData> >,cells,,,""))
		, /* ctor */ scene=NULL;
		, /* py */
			WOO_PY_TOPINDEXABLE(Field)
	);
	REGISTER_INDEX_COUNTER(Field);

	// return bounding box for rendering
	// by defalt, returns bbox of nodes, but derived fields may override this
	virtual bool renderingBbox(Vector3r& mn, Vector3r& mx);

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
REGISTER_SERIALIZABLE(Field);
