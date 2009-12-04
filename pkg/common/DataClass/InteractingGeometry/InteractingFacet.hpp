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

// define this to have topology information about facets enabled;
// it is necessary for FacetTopologyAnalyzer
#define FACET_TOPO

class InteractingFacet : public Shape {
    public:
	
	// Registered attributes
		
	/// Vertex positions in the local reference frame
	vector<Vector3r> vertices;

	InteractingFacet();
	virtual ~InteractingFacet();
	
	// Postprocessed attributes 

	/// Facet's normal
	Vector3r nf; 
	/// Normals of edges 
	Vector3r ne[3];
	/// Inscribing cirle radius
	Real icr;
	/// Length of the vertice vectors 
	Real vl[3];
	/// Unit vertice vectors
	Vector3r vu[3];
	#ifdef FACET_TOPO
		//! facet id's that are adjacent to respective edges
		vector<body_id_t> edgeAdjIds;
		//! half angle between normals of this facet and the adjacent facet
		vector<Real> edgeAdjHalfAngle;
	#endif

	DECLARE_LOGGER;

	REGISTER_ATTRIBUTES(Shape,(vertices)
		#ifdef FACET_TOPO
			(edgeAdjIds)(edgeAdjHalfAngle)
		#endif
	);
	protected: void postProcessAttributes(bool deserializing);
	REGISTER_CLASS_NAME(InteractingFacet);
	REGISTER_BASE_CLASS_NAME(Shape);

	REGISTER_CLASS_INDEX(InteractingFacet,Shape);
};

REGISTER_SERIALIZABLE(InteractingFacet);

