/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
 
#pragma once

#include<yade/pkg-common/StateEngineUnit.hpp>
#include<yade/core/MetaEngine.hpp>
#include<yade/lib-multimethods/DynLibDispatcher.hpp>
#include<yade/core/State.hpp>
#include<yade/core/Body.hpp>
#include<yade/core/BexContainer.hpp>

class StateMetaEngine :	public MetaEngine1D
					<	
						State ,
						StateEngineUnit,
						void ,
						TYPELIST_3(	  const shared_ptr<State>&
								, Body*, BexContainer&
				  			  )
					>
{
	public :
		virtual void action(MetaBody*);

	REGISTER_CLASS_NAME(StateMetaEngine);
	REGISTER_BASE_CLASS_NAME(MetaEngine1D);

};

REGISTER_SERIALIZABLE(StateMetaEngine);


