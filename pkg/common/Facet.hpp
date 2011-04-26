/*************************************************************************
*  Copyright (C) 2008 by Sergei Dorofeenko				 *
*  sega@users.berlios.de                                                 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#pragma once


#include<yade/core/Shape.hpp>
#include<yade/core/Body.hpp>
#include<yade/pkg/common/Dispatching.hpp>


// define this to have topology information about facets enabled;
// it is necessary for FacetTopologyAnalyzer
// #define FACET_TOPO

class Facet: public Shape {
   public:
	virtual ~Facet();
	/// Normals of edges 
	Vector3r ne[3];
	/// Inscribing cirle radius
	Real icr;
	/// Length of the vertice vectors 
	Real vl[3];
	/// Unit vertice vectors
	Vector3r vu[3];
	// set vertices such that they are in v0, v1, v2 in global space; returns position and orientation of the particle, calls postLoad automatically
	boost::tuple<Vector3r,Quaternionr> updateGlobalVertices(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2);
	void postLoad(Facet&);
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Facet,Shape,"Facet (triangular particle) geometry.",
		((vector<Vector3r>,vertices,vector<Vector3r>(3,Vector3r(NaN,NaN,NaN)),(Attr::triggerPostLoad | Attr::noResize),"Vertex positions in local coordinates."))
		((Vector3r,normal,Vector3r(NaN,NaN,NaN),(Attr::readonly | Attr::noSave),"Facet's normal"))
		#ifdef FACET_TOPO
		((vector<Body::id_t>,edgeAdjIds,vector<Body::id_t>(3,Body::ID_NONE),,"Facet id's that are adjacent to respective edges [experimental]"))
		((vector<Real>,edgeAdjHalfAngle,vector<Real>(3,0),,"half angle between normals of this facet and the adjacent facet [experimental]"))
		#endif
		,
		/* ctor */ createIndex();,
	);
	DECLARE_LOGGER;
	REGISTER_CLASS_INDEX(Facet,Shape);
};
REGISTER_SERIALIZABLE(Facet);

class Bo1_Facet_Aabb : public BoundFunctor{
	public:
		void go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r& se3, const Body*);
	FUNCTOR1D(Facet);
	YADE_CLASS_BASE_DOC(Bo1_Facet_Aabb,BoundFunctor,"Creates/updates an :yref:`Aabb` of a :yref:`Facet`.");
};
REGISTER_SERIALIZABLE(Bo1_Facet_Aabb);


#ifdef YADE_OPENGL
#include<yade/pkg/common/GLDrawFunctors.hpp>
class Gl1_Facet: public GlShapeFunctor{	
	public:
		virtual void go(const shared_ptr<Shape>&, const shared_ptr<State>&,bool,const GLViewInfo&);
	RENDERS(Facet);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_Facet,GlShapeFunctor,"Renders :yref:`Facet` object",
		((bool,normals,false,,"In wire mode, render normals of facets and edges; facet's :yref:`colors<Shape::color>` are disregarded in that case."))
	);
};
REGISTER_SERIALIZABLE(Gl1_Facet);
#endif
