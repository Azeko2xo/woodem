/***************************************************************************
 *   Copyright (C) 2004 by Janek Kozicki                                   *
 *   cosurgi@berlios.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LATTICE_BOX_HPP
#define LATTICE_BOX_HPP 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-core/FileGenerator.hpp>
#include <yade/yade-lib-wm3-math/Vector3.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class LatticeExample : public FileGenerator
{
	private :
		Vector3r 	nbNodes;
		Real 		disorder;
		Real 		maxLength;
		int 		nodeGroupMask,beamGroupMask;
		Real 		maxDeformationSquared;
		
		Vector3r 	regionA_min;
		Vector3r 	regionA_max;
		Vector3r 	direction_A;
		Real 		velocity_A;
		Vector3r 	regionB_min;
		Vector3r 	regionB_max;
		Vector3r 	direction_B;
		Real 		velocity_B;

	public : 
		LatticeExample();
		virtual ~LatticeExample();

		string generate();
	
		void createActors(shared_ptr<MetaBody>& rootBody);
		void positionRootBody(shared_ptr<MetaBody>& rootBody);
		void createNode(shared_ptr<Body>& body, int i, int j, int k);
		void createBeam(shared_ptr<Body>& body, unsigned int i, unsigned int j);
		void calcBeamsPositionOrientationLength(shared_ptr<MetaBody>& body);
		void imposeTranslation(shared_ptr<MetaBody>& rootBody, Vector3r min, Vector3r max, Vector3r direction, Real velocity);

		virtual void registerAttributes();
		REGISTER_CLASS_NAME(LatticeExample);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_SERIALIZABLE(LatticeExample,false);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // LATTICE_BOX_HPP 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

