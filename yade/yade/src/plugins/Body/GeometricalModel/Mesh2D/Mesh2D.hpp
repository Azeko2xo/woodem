/***************************************************************************
 *   Copyright (C) 2004 by Olivier Galizzi                                 *
 *   olivier.galizzi@imag.fr                                               *
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

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MESH2D_H__
#define __MESH2D_H__

#include "GeometricalModel.hpp"

#define offset(i,j) ((i)*height+(j))

class Edge : public Serializable
{
	public : int first;
	public : int second;
	public : Edge() {};
	public : Edge(int f,int s) :first(f), second(s) {};
	protected : virtual void postProcessAttributes(bool) {};
	public : void registerAttributes()
	{
		REGISTER_ATTRIBUTE(first);
		REGISTER_ATTRIBUTE(second);
	};
	REGISTER_CLASS_NAME(Edge);

};
REGISTER_SERIALIZABLE(Edge,true);

class Mesh2D : public GeometricalModel
{
	public : vector<Vector3r> vertices;
	public : vector<Edge> edges;
	public : int width,height;
	public : vector<vector<int> > faces;
	public : vector<Vector3r> fNormals;
	public : vector<Vector3r> vNormals;
 	public : vector<vector<int> > triPerVertices;
	// construction
	public : Mesh2D ();
	public : virtual ~Mesh2D ();

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Serialization										///
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	REGISTER_CLASS_NAME(Mesh2D);
	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Indexable											///
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	REGISTER_CLASS_INDEX(Mesh2D,GeometricalModel);

};


REGISTER_SERIALIZABLE(Mesh2D,false);

#endif // __MESH2D_H__
