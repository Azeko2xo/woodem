/*************************************************************************
*  Copyright (C) 2007 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef GLDRAW_LATTICE_INTERACTING_GEOMETRY_HPP
#define GLDRAW_LATTICE_INTERACTING_GEOMETRY_HPP

#include<yade/pkg-common/GLDrawFunctors.hpp>
#include <vector>

class GLDrawLatticeInteractingGeometry : public GLDrawInteractingGeometryFunctor
{
	private :
		Real maxLen;
		//std::vector<std::vector<std::vector<bool> > > done;
		//std::vector<std::vector<std::vector<unsigned char> > > done;
		std::set<std::set<unsigned int> > done;
		bool drawn(unsigned int A,unsigned int B,unsigned int C);
		void zeroDrawn();
		
		std::vector<unsigned char> damaged;
		void damagedNeighbor(unsigned int a);

	public :
		GLDrawLatticeInteractingGeometry();
		virtual void go(const shared_ptr<InteractingGeometry>&, const shared_ptr<PhysicalParameters>&,bool);

	RENDERS(LatticeInteractingGeometry);
	REGISTER_CLASS_NAME(GLDrawLatticeInteractingGeometry);
	REGISTER_BASE_CLASS_NAME(GLDrawInteractingGeometryFunctor);

};

REGISTER_SERIALIZABLE(GLDrawLatticeInteractingGeometry);

#endif

