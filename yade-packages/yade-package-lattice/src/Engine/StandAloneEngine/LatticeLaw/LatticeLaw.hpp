/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef LATTICELAW_HPP
#define LATTICELAW_HPP


#include "LatticeBeamParameters.hpp"
#include <yade/yade-core/InteractionSolver.hpp>
#include <yade/yade-core/BodyContainer.hpp>


class PhysicalAction;


class LatticeLaw : public InteractionSolver
{

/// Attributes	
        
        private :
                vector<unsigned int> futureDeletes;
                bool nonlocal;
                
                bool deleteBeam(MetaBody* lattice , LatticeBeamParameters* beam, Body*);
                void calcBeamPositionOrientationNewLength(Body* body, BodyContainer* bodies);
        public :
                LatticeLaw();
		virtual ~LatticeLaw();
		void action(Body* body);

/// Serializtion
	REGISTER_CLASS_NAME(LatticeLaw);
	REGISTER_BASE_CLASS_NAME(InteractionSolver);
};


REGISTER_SERIALIZABLE(LatticeLaw,false);


#endif // LATTICELAW_HPP


