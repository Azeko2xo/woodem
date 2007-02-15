/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef SPHERE2SPHERE4MACROMICROCONTACTGEOMETRY_HPP
#define SPHERE2SPHERE4MACROMICROCONTACTGEOMETRY_HPP

#include <yade/yade-package-common/InteractionGeometryEngineUnit.hpp>

class InteractingSphere2InteractingSphere4SpheresContactGeometry : public InteractionGeometryEngineUnit
{
	public :
		virtual bool go(	const shared_ptr<InteractingGeometry>& cm1,
					const shared_ptr<InteractingGeometry>& cm2,
					const Se3r& se31,
					const Se3r& se32,
					const shared_ptr<Interaction>& c);
		virtual bool goReverse(	const shared_ptr<InteractingGeometry>& cm1,
					const shared_ptr<InteractingGeometry>& cm2,
					const Se3r& se31,
					const Se3r& se32,
					const shared_ptr<Interaction>& c);
					
		InteractingSphere2InteractingSphere4SpheresContactGeometry();		
					
		double InteractionDetectionFactor;// InteractionGeometry will be computed when InteractionDetectionFactor*(rad1+rad2) > distance 		
	

	REGISTER_CLASS_NAME(InteractingSphere2InteractingSphere4SpheresContactGeometry);
	REGISTER_BASE_CLASS_NAME(InteractionGeometryEngineUnit);

	DEFINE_FUNCTOR_ORDER_2D(InteractingSphere,InteractingSphere);
	
	protected :
		virtual void registerAttributes();
};

REGISTER_SERIALIZABLE(InteractingSphere2InteractingSphere4SpheresContactGeometry,false);

#endif // SPHERE2SPHERE4MACROMICROCONTACTGEOMETRY_HPP

