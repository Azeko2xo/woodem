/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
 
#ifndef BODY_PHYSICAL_PARAMETERS_DISPATCHER_HPP
#define BODY_PHYSICAL_PARAMETERS_DISPATCHER_HPP 

#include<yade/pkg-common/PhysicalParametersEngineUnit.hpp>
#include<yade/core/MetaEngine1D.hpp>
#include<yade/lib-multimethods/DynLibDispatcher.hpp>
#include<yade/core/PhysicalParameters.hpp>
#include<yade/core/Body.hpp>

class PhysicalParametersMetaEngine :	public MetaEngine1D
					<	
						PhysicalParameters ,
						PhysicalParametersEngineUnit,
						void ,
						TYPELIST_2(	  const shared_ptr<PhysicalParameters>&
								, Body*
				  			  )
					>
{
	public :
		virtual void action(MetaBody*);

	REGISTER_CLASS_NAME(PhysicalParametersMetaEngine);
	REGISTER_BASE_CLASS_NAME(MetaEngine1D);

};

REGISTER_SERIALIZABLE(PhysicalParametersMetaEngine,false);

#endif // BODY_PHYSICAL_PARAMETERS_DISPATCHER_HPP 

