/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
 
#pragma once

#include<yade/pkg-common/InteractingGeometryEngineUnit.hpp>

#include<yade/core/Dispatcher.hpp>
#include<yade/lib-multimethods/DynLibDispatcher.hpp>
#include<yade/core/GeometricalModel.hpp>
#include<yade/core/InteractingGeometry.hpp>
#include<yade/core/Body.hpp>

class InteractingGeometryMetaEngine : 	public Dispatcher2D
					<	
						GeometricalModel,						// base classe for dispatch
						InteractingGeometry,						// base classe for dispatch
						InteractingGeometryEngineUnit,					// class that provides multivirtual call
						void ,								// return type
						TYPELIST_4(	  const shared_ptr<GeometricalModel>&		// arguments
								, shared_ptr<InteractingGeometry>& 		// is not working when const, because functors are supposed to modify it!
								, const Se3r&
								, const Body* 					// with that - functors have all the data they may need
							  )
					>
{
	public :
		virtual void action(World*);

	REGISTER_CLASS_NAME(InteractingGeometryMetaEngine);
	REGISTER_BASE_CLASS_NAME(Dispatcher2D);
};

REGISTER_SERIALIZABLE(InteractingGeometryMetaEngine);


