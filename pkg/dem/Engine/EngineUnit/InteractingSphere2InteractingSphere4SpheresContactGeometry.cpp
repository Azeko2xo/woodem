/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include"InteractingSphere2InteractingSphere4SpheresContactGeometry.hpp"
#include<yade/pkg-dem/SpheresContactGeometry.hpp>
#include<yade/pkg-dem/SDECLinkGeometry.hpp>
#include<yade/pkg-common/InteractingSphere.hpp>

#include<yade/lib-base/yadeWm3Extra.hpp>


InteractingSphere2InteractingSphere4SpheresContactGeometry::InteractingSphere2InteractingSphere4SpheresContactGeometry()
{
	interactionDetectionFactor = 1;
}

void InteractingSphere2InteractingSphere4SpheresContactGeometry::registerAttributes()
{	
	REGISTER_ATTRIBUTE(interactionDetectionFactor);
}

bool InteractingSphere2InteractingSphere4SpheresContactGeometry::go(	const shared_ptr<InteractingGeometry>& cm1,
							const shared_ptr<InteractingGeometry>& cm2,
							const Se3r& se31,
							const Se3r& se32,
							const shared_ptr<Interaction>& c)
{
	InteractingSphere* s1 = static_cast<InteractingSphere*>(cm1.get());
	InteractingSphere* s2 = static_cast<InteractingSphere*>(cm2.get());

	Vector3r normal = se32.position-se31.position;
	Real penetrationDepth = pow(interactionDetectionFactor*(s1->radius+s2->radius), 2) - normal.SquaredLength();// Compute a wrong value just to check sign - faster than computing distances
	//Real penetrationDepth = s1->radius+s2->radius-normal.Normalize();
	if (penetrationDepth>0 /*|| c->isReal*/){
		shared_ptr<SpheresContactGeometry> scm;
		if (c->interactionGeometry){
			// WARNING! 
			// FIXME - this must be dynamic cast until the contaners are rewritten to support multiple interactions types
			//         the problem is that scm can be either SDECLinkGeometry or SpheresContactGeometry and the only way CURRENTLY
			//         to check this is by dynamic cast. This has to be fixed.
			scm = YADE_PTR_CAST<SpheresContactGeometry>(c->interactionGeometry);
			#if 0
				// BEGIN .......  FIXME FIXME	- wrong hack, to make cohesion work.
					if(! scm) // this is not SpheresContactGeometry, so it is SDECLinkGeometry, dispatcher should do this job.
					{
						shared_ptr<SDECLinkGeometry> linkGeometry = dynamic_pointer_cast<SDECLinkGeometry>(c->interactionGeometry);
							cerr << "it is SpringGeometry ???: " << c->interactionGeometry->getClassName() << endl;
							assert(linkGeometry);
						if(linkGeometry)
						{
							linkGeometry->normal 			= se32.position-se31.position;
							linkGeometry->normal.Normalize();
							return true;
						}
						else
							return false; // SpringGeometry !!!???????
					}
					// END
			#endif
		} else scm = shared_ptr<SpheresContactGeometry>(new SpheresContactGeometry());
		penetrationDepth = s1->radius+s2->radius-normal.Normalize();
		scm->contactPoint = se31.position+(s1->radius-0.5*penetrationDepth)*normal;//0.5*(pt1+pt2);
		scm->normal = normal;
		scm->penetrationDepth = penetrationDepth;
		scm->radius1 = s1->radius;
		scm->radius2 = s2->radius;
		if (!c->interactionGeometry) c->interactionGeometry = scm;
		return true;
	} else return false;
}


bool InteractingSphere2InteractingSphere4SpheresContactGeometry::goReverse(	const shared_ptr<InteractingGeometry>& cm1,
								const shared_ptr<InteractingGeometry>& cm2,
								const Se3r& se31,
								const Se3r& se32,
								const shared_ptr<Interaction>& c)
{
	return go(cm1,cm2,se31,se32,c);
}

YADE_PLUGIN();
