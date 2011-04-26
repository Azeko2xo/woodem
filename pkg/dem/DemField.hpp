#pragma once
#include<yade/core/Field.hpp>
#include<yade/core/Engine.hpp>

class DemNodeData: public NodeData{
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(DemNodeData,NodeData,"Nodal data for DEM.",
		((Vector3r,vel,Vector3r::Zero(),,"Linear velocity"))
		((Vector3r,angVel,Vector3r::Zero(),,"Angular velocity"))
		((Real,mass,0,,"Nodal mass"))
		((Vector3r,inertia,0,,"Rotational inertia (in local axes)"))
		, /* ctor */ createIndex();
	);
	REGISTER_CLASS_INDEX(DemNodeData,NodeData);
};
REGISTER_SERIALIZABLE(DemNodeData);

class DemField: public Field{
	ForceContainer forces; // not serializable, neither directly accessible from python
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(DemField,Serializable,"Field describing a discrete element assembly. Each body references (possibly many) nodes by their index in :yref:`Field.nodes` and :yref:`Field.nodalData`. ",
		((shared_ptr<BodyContainer>,bodies,new BodyContainer,Attr::noGui,"Particles (each particle holds its contacts, and references associated nodes)"))
		, /* ctor */ createIndex();
	);
	REGISTER_CLASS_INDEX(DemField,Field);
};
REGISTER_SERIALIZABLE(DemField);

struct StateNodeXCompat: public GlobalEngine{
	void action();
	YADE_CLASS_BASE_DOC(StateNodeXCompat,GlobalEngine,"Cross-update Nodes from State; used at the beginning of the loop, to make sure old engines work properly.");
};
REGISTER_SERIALIZABLE(StateNodeXCompat);

#if 0
struct BodyForceToNodes: public GlobalEngine{
	void action();
	YADE_CLASS_BASE_DOC(BodyForceToNodes,GlobalEngine,"Apply forces on multi-nodal particles to the nodes

};
#endif
#if 0
struct UpdateNodesFromState: public GlobalEngine{
	void action();
	YADE_CLASS_BASE_DOC(UpdateNodesFromState,GlobalEngine,"Update Nodes from State; used at the end of the loop, so that results of motion integration are picked up by the next run of UpdateStateFromNodes.");
};
REGISTER_SERIALIZABLE(UpdateStateFromNodes);
#endif

#ifdef YADE_OPENGL
#include<yade/pkg/common/GLDrawFunctors.hpp>
struct Gl1_Node: public GlNodeFunctor{
	virtual void go(const shared_ptr<Node>&, const GLViewInfo&);
	RENDERS(Node);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_Node,GlNodeFunctor,"Render generic :yref:`Node`.",
		((int,wd,1,,"Local axes line width in pixels"))
		((Vector2i,wd_range,Vector2i(0,5),Attr::noGui,"Range for width"))
		((Real,len,.05,,"Relative local axes line length in pixels, relative to scene radius; if non-positive, only points are drawn"))
		((Vector2r,len_range,Vector2r(0.,.1),Attr::noGui,"Range for len"))
	);
};
#endif
