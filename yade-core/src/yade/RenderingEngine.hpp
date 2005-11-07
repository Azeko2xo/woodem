/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef RENDERINGENGINE_HPP
#define RENDERINGENGINE_HPP

#include "MetaBody.hpp"
#include <yade/yade-lib-serialization/Serializable.hpp>

class RenderingEngine :  public Serializable
{	
	public :
		RenderingEngine():Serializable() {};
		virtual ~RenderingEngine() {} ;
	
		virtual void render(const shared_ptr<MetaBody>& ) {throw;};
		virtual void init() {throw;};

	REGISTER_CLASS_NAME(RenderingEngine);
	REGISTER_BASE_CLASS_NAME(Serializable);
};

REGISTER_SERIALIZABLE(RenderingEngine,false);

#endif // RENDERINGENGINE_HPP

