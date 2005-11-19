/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef GLDRAWSPHERE_HPP
#define GLDRAWSPHERE_HPP

#include "GLDrawGeometricalModelFunctor.hpp"

class GLDrawSphere : public GLDrawGeometricalModelFunctor
{
	private :
		static vector<Vector3r> vertices;
		static vector<Vector3r> faces;
		static int glWiredSphereList;
		static int glSphereList;
		void subdivideTriangle(Vector3r& v1,Vector3r& v2,Vector3r& v3, int depth);
		void drawSphere(int depth);
	
	public :
		virtual void go(const shared_ptr<GeometricalModel>&, const shared_ptr<PhysicalParameters>&,bool);
		
	RENDERS(Sphere);
	REGISTER_CLASS_NAME(GLDrawSphere);
	REGISTER_BASE_CLASS_NAME(GLDrawGeometricalModelFunctor);
};

REGISTER_SERIALIZABLE(GLDrawSphere,false);

#endif // GLDRAWSPHERE_HPP

