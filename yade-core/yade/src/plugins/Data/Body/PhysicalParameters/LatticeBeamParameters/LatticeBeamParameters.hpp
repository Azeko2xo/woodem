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

#ifndef LATTICE_BEAM_PARAMETERS_HPP
#define LATTICE_BEAM_PARAMETERS_HPP 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade-common/RigidBodyParameters.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class LatticeBeamParameters : public RigidBodyParameters
{
	public :
		unsigned int 	id1,id2;
		Real  		initialLength,length;
		Vector3r 	direction;
	
	public : LatticeBeamParameters();
	public : virtual ~LatticeBeamParameters();

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Serialization										///
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	REGISTER_CLASS_NAME(LatticeBeamParameters);
	public : void registerAttributes();
	
///////////////////////////////////////////////////////////////////////////////////////////////////
/// Indexable											///
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	REGISTER_CLASS_INDEX(LatticeBeamParameters,RigidBodyParameters);

};

REGISTER_SERIALIZABLE(LatticeBeamParameters,false);

#endif // LATTICE_BEAM_PARAMETERS_HPP 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

