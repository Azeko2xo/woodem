/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "Tetrahedron2InteractingMyTetrahedron.hpp"
#include<yade/pkg-dem/InteractingMyTetrahedron.hpp>
#include<yade/pkg-common/Tetrahedron.hpp>


void Tetrahedron2InteractingMyTetrahedron::go(	const shared_ptr<GeometricalModel>& gm,
				shared_ptr<InteractingGeometry>& ig,
				const Se3r& se3,
				const Body*	)
{
	Tetrahedron* tet = static_cast<Tetrahedron*>(gm.get());
	if(ig == 0)
		ig = boost::shared_ptr<InteractingGeometry>(new InteractingMyTetrahedron);

	InteractingMyTetrahedron* it = YADE_CAST<InteractingMyTetrahedron*>(ig.get());
	
	it->c1 = tet->v[0]*0.5;
	it->c2 = tet->v[1]*0.5;
	it->c3 = tet->v[2]*0.5;
	it->c4 = tet->v[3]*0.5;

	it->r1 = it->c1.Length();
	it->r2 = it->c2.Length();
	it->r3 = it->c3.Length();
	it->r4 = it->c4.Length();
}
	
YADE_PLUGIN("Tetrahedron2InteractingMyTetrahedron");