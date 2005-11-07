/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef FUNNEL_HPP
#define FUNNEL_HPP

#include <yade/yade-core/FileGenerator.hpp>

class Funnel : public FileGenerator
{
	private :
		Vector3r	 nbSpheres
				,groundSize
				,gravity;
		
		Real		 minRadius
				,density
				,maxRadius
				,dampingForce
				,disorder
				,dampingMomentum
				,sphereYoungModulus
				,spherePoissonRatio
				,sphereFrictionDeg;
		
		int		timeStepUpdateInterval;
		bool		rotationBlocked;
	
		void createBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents);
		void createSphere(shared_ptr<Body>& body, int i, int j, int k);
		void createActors(shared_ptr<MetaBody>& rootBody);
		void positionRootBody(shared_ptr<MetaBody>& rootBody);

	public :
		Funnel ();
		~Funnel ();
		string generate();

	protected :
		void registerAttributes();
		virtual void postProcessAttributes(bool deserializing);
	REGISTER_CLASS_NAME(Funnel);
	REGISTER_BASE_CLASS_NAME(FileGenerator);

};

REGISTER_SERIALIZABLE(Funnel,false);

#endif //  FUNNEL_HPP

