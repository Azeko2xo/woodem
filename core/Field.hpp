#pragma once
#include<woo/lib/multimethods/Indexable.hpp>
#include<woo/lib/object/Object.hpp>
#include<woo/core/Master.hpp>

namespace py=boost::python;

#include<boost/thread/thread.hpp>

struct Scene;

struct Constraint: public Object{	
	virtual void applyConstraint(Vector3r& newPos, Vector3r& newOri){};
	WOO_CLASS_BASE_DOC_ATTRS(Constraint,Object,"Object defining constraint on motion of a node; this is an abstract class which is not to be used directly.",
	);
};
WOO_REGISTER_OBJECT(Constraint);

struct NodeData: public Object{
	boost::mutex lock; // used by applyForceTorque

	// template to be specialized by derived classes
	template<typename Derived> struct Index; // { BOOST_STATIC_ASSERT(false); /* template must be specialized for derived NodeData types */ };

	WOO_CLASS_BASE_DOC(NodeData,Object,"Data associated with some node.");
};
WOO_REGISTER_OBJECT(NodeData);

#ifdef WOO_OPENGL
struct GLViewInfo;
struct Node;

// object representing what should be rendered at associated node
struct NodeGlRep: public Object{
	//boost::mutex lock; // for rendering
	//void safeRender(const shared_ptr<Node>& n, GLViewInfo* glvi){ boost::mutex::scoped_lock l(lock); render(n,glvi); }
	virtual void render(const shared_ptr<Node>&, const GLViewInfo*){};
	WOO_CLASS_BASE_DOC(NodeGlRep,Object,"Object representing what should be rendered at associated node (abstract base class).");
};
WOO_REGISTER_OBJECT(NodeGlRep);

struct QglMovableObject;
struct ScalarRange: public Object{
	shared_ptr<QglMovableObject> movablePtr;
	void reset();
	Vector3r color(Real v);
	Real maxAbs(){ if(!isOk()) abort(); throw std::runtime_error("ScalarRange::maxAbs(): uninitialized object, call with value to adjust range.");  return max(abs(mnmx[0]),abs(mnmx[1])); }
	Real maxAbs(Real v){ adjust(v); return max(abs(mnmx[0]),abs(mnmx[1])); }
	bool isOk(){ return(mnmx[0]<mnmx[1]); }
	// return value on the range, given normalized value
	Real normInv(Real norm);
	Real norm(Real v);
	void adjust(const Real& v);
	// called only when mnmx is manipulated
	enum{RANGE_LOG=1,RANGE_REVERSED=2,RANGE_SYMMETRIC=4,RANGE_AUTO_ADJUST=8,RANGE_HIDDEN=16};
	void postLoad(const ScalarRange&,void*);

	bool isAutoAdjust() const { return flags&RANGE_AUTO_ADJUST; }
	void setAutoAdjust(bool aa) { if(!aa) flags&=~RANGE_AUTO_ADJUST; else flags|=RANGE_AUTO_ADJUST; }
	bool isSymmetric() const { return flags&RANGE_SYMMETRIC; }
	void setSymmetric(bool s) { if(!s) flags&=~RANGE_SYMMETRIC; else flags|=RANGE_SYMMETRIC; }
	bool isReversed() const { return flags&RANGE_REVERSED; }
	void setReversed(bool s) { if(!s) flags&=~RANGE_REVERSED; else flags|=RANGE_REVERSED; }
	bool isHidden() const { return flags&RANGE_HIDDEN; }
	void setHidden(bool h) { if(!h) flags&=~RANGE_HIDDEN; else flags|=RANGE_HIDDEN; }
	bool isLog() const { return flags&RANGE_LOG; }
	void setLog(bool l) { if(!l) flags&=~RANGE_LOG; else flags|=RANGE_LOG; }
	void cacheLogs(); 

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ScalarRange,Object,"Store and share range of scalar values",
		((Vector2r,mnmx,Vector2r(std::numeric_limits<Real>::infinity(),-std::numeric_limits<Real>::infinity()),AttrTrait<Attr::triggerPostLoad>().buttons({"Reset","self.reset()","Re-initialize range"}),"Packed minimum and maximum values; adjusting from python sets :obj:`autoAdjust` to false automatically."))
		((Vector2r,logMnmx,,AttrTrait<Attr::noSave>().hidden(),"Logs of mnmx values, to avoid computing logarithms all the time; computed via cacheLogs."))
		((int,flags,(RANGE_AUTO_ADJUST),AttrTrait<>().bits({"log","reversed","symmetric","autoAdjust","hidden"}),"Flags for this range: autoAdjust, symmetric, reversed, hidden, log."))
		//((bool,autoAdjust,true,,"Automatically adjust range using given values."))
		//((bool,sym,false,,"Force maximum to be negative of minimum and vice versa (only with autoadjust)"))
		((Vector2i,dispPos,Vector2i(-1000,-1000),AttrTrait<>().noGui(),"Where is this range displayed on the OpenGL canvas; initially out of range, will be reset automatically."))
		((Real,length,200,AttrTrait<>().noGui(),"Length on the display; if negative, it is fractional relative to view width/height"))
		((bool,landscape,false,AttrTrait<>().noGui(),"Make the range display with landscape orientation"))
		((std::string,label,,,"Short name of this range."))
		((int,cmap,-1,AttrTrait<>().colormapChoice(),"Colormap index to be used; -1 means to use the default colormap (see *woo.master.cmaps*, *woo.master.cmap*)"))
		, /* ctor */
		, /* py */
			.def("norm",&ScalarRange::norm,"Return value of the argument normalized to 0..1 range; the value is not clamped to 0..1 however: if autoAdjust is false, it can fall outside.")
			.def("reset",&ScalarRange::reset)
			.add_property("autoAdjust",&ScalarRange::isAutoAdjust,&ScalarRange::setAutoAdjust)
			.add_property("symmetric",&ScalarRange::isSymmetric,&ScalarRange::setSymmetric)
			.add_property("reversed",&ScalarRange::isReversed,&ScalarRange::setReversed)
			.add_property("hidden",&ScalarRange::isHidden,&ScalarRange::setHidden)
			.add_property("log",&ScalarRange::isLog,&ScalarRange::setLog)
	);
};
WOO_REGISTER_OBJECT(ScalarRange);
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

	// transform point p from global to local coordinates
	Vector3r glob2loc(const Vector3r& p){ return ori.conjugate()*(p-pos); }
	// 
	Vector3r loc2glob(const Vector3r& p){ return ori*p+pos; }

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Node,Object,"A point in space (defining local coordinate system), referenced by other objects.",
		((Vector3r,pos,Vector3r::Zero(),AttrTrait<>().lenUnit(),"Position in space (cartesian coordinates); origin :math:`O` of the local coordinate system."))
		((Quaternionr,ori,Quaternionr::Identity(),,"Orientation :math:`q` of this node."))
		((vector<shared_ptr<NodeData> >,data,,,"Array of data, ordered in globally consistent manner."))
		#ifdef WOO_OPENGL
			((shared_ptr<NodeGlRep>,rep,,,"What should be shown at this node when rendered via OpenGL."))
		#endif
		, /* ctor */ createIndex();
		, /* py */ WOO_PY_TOPINDEXABLE(Node)
			.def("glob2loc",&Node::glob2loc,(py::arg("p")),"Transform point :math:`p` from global to node-local coordinates as :math:`q^*(p-O)`.")
			.def("loc2glob",&Node::glob2loc,(py::arg("p")),"Transform point :math:`p_l` from node-local to global coordinates as :math:`q\\cdot p_l+O`.")
	);
	REGISTER_INDEX_COUNTER(Node);
};
WOO_REGISTER_OBJECT(Node);


struct Field: public Object, public Indexable{
	Scene* scene; // backptr to scene; must be set by Scene!
	py::object py_getScene();
	virtual void selfTest(){};
	virtual Real critDt() { return Inf; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(Field,Object,"Spatial field described by nodes, their topology and associated values.",
		((vector<shared_ptr<Node> >,nodes,,AttrTrait<Attr::pyByRef>(),"Nodes referenced from this field."))
		//((shared_ptr<Topology>,topology,,,"How nodes build up cells, neighborhood and coonectivity information."))
		//((vector<shared_ptr<CellData> >,cells,,,""))
		, /* ctor */ scene=NULL;
		, /* py */
			.add_property("scene",&Field::py_getScene,"Get associated scene object, if any (this function is dangerous in some corner cases, as it has to use raw pointer).")
			.def("critDt",&Field::critDt,"Return critical (maximum numerically stable) timestep for this field. By default returns infinity (no critical timestep) but derived fields may override this function.")
			WOO_PY_TOPINDEXABLE(Field)
	);
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
