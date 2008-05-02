/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef INTERACTION_GEOMETRY_METAENGINE_HPP
#define INTERACTION_GEOMETRY_METAENGINE_HPP

#include <vector>
#include <list>

#include<yade/core/Interaction.hpp>
#include<yade/lib-multimethods/DynLibDispatcher.hpp>
#include<yade/core/MetaEngine2D.hpp>
#include<yade/core/InteractingGeometry.hpp>
#include "InteractionGeometryEngineUnit.hpp"

class Body;

class InteractionGeometryMetaEngine :	public MetaEngine2D
					<	
						InteractingGeometry,						// base classe for dispatch
						InteractingGeometry,						// base classe for dispatch
						InteractionGeometryEngineUnit,					// class that provides multivirtual call
						bool ,								// return type
						TYPELIST_5(	  const shared_ptr<InteractingGeometry>&	// arguments
								, const shared_ptr<InteractingGeometry>&
								, const Se3r&
								, const Se3r&
								, const shared_ptr<Interaction>&
							  )
						, false								// disable auto symmetry handling
					>
{
	public :
		virtual void action(MetaBody*);

	REGISTER_CLASS_NAME(InteractionGeometryMetaEngine);
	REGISTER_BASE_CLASS_NAME(MetaEngine2D);
};

REGISTER_SERIALIZABLE(InteractionGeometryMetaEngine,false);

#endif //  INTERACTION_GEOMETRY_METAENGINE_HPP

