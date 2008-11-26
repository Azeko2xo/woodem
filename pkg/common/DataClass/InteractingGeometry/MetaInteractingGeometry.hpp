/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef METAINTERACTINGGEOMETRY_HPP
#define METAINTERACTINGGEOMETRY_HPP

#include<yade/core/InteractingGeometry.hpp>

class MetaInteractingGeometry : public InteractingGeometry
{
	public :
		MetaInteractingGeometry ();
		virtual ~MetaInteractingGeometry ();

/// Serialization
	REGISTER_CLASS_NAME(MetaInteractingGeometry);
	REGISTER_BASE_CLASS_NAME(InteractingGeometry);
	
/// Indexable
	REGISTER_CLASS_INDEX(MetaInteractingGeometry,InteractingGeometry);
};

REGISTER_SERIALIZABLE(MetaInteractingGeometry);

#endif // METAINTERACTINGGEOMETRY_HPP

