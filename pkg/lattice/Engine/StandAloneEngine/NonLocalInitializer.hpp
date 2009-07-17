/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/StandAloneEngine.hpp>
#include<yade/pkg-lattice/LatticeSetParameters.hpp>

class Body;
class BodyContainer;
class InteractionContainer; 

class NonLocalInitializer : public StandAloneEngine
{
	private:
		//void calcNonLocal(Body* body1, Body* body2, BodyContainer* bodies,InteractionContainer* ints);
		//bool calcNonLocal(Body* body1, Body* body2, BodyContainer* bodies,std::vector<std::list<LatticeSetParameters::NonLocalInteraction , std::__malloc_alloc_template<sizeof(LatticeSetParameters::NonLocalInteraction)> > >& nonl);
		bool calcNonLocal(Body* body1, Body* body2, BodyContainer* bodies,void* nonl);

	public :
		Real 	range;
		
		NonLocalInitializer ();
		virtual ~NonLocalInitializer ();
		virtual void action(MetaBody*);

	REGISTER_ATTRIBUTES(StandAloneEngine,(range));
	REGISTER_CLASS_NAME(NonLocalInitializer);
	REGISTER_BASE_CLASS_NAME(StandAloneEngine);
};

REGISTER_SERIALIZABLE(NonLocalInitializer);


