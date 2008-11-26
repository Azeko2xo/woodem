/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef LATTICE_SET_PARAMETERS_HPP
#define LATTICE_SET_PARAMETERS_HPP 


#include<yade/core/PhysicalParameters.hpp>
#include <list>

class LatticeSetParameters : public PhysicalParameters
{
	public :
                int      beamGroupMask
                        ,nodeGroupMask;

                bool     useBendTensileSoftening,useStiffnessSoftening;
                        
                unsigned long int total;
                
                struct NonLocalInteraction
                {
                        unsigned int    id1,id2;
                //      Real            gaussValue; // must use malloc()
                        //float                 gaussValue; // must use malloc()
                };
                //std::vector<std::list<NonLocalInteraction , std::__malloc_alloc_template<sizeof(NonLocalInteraction)> > > nonl;
                void* nonl;
                Real range;
                
                LatticeSetParameters();
                virtual ~LatticeSetParameters();

/// Serializable
	protected :
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(LatticeSetParameters);
	REGISTER_BASE_CLASS_NAME(PhysicalParameters);

/// Indexable
	REGISTER_CLASS_INDEX(LatticeSetParameters,PhysicalParameters);

};

REGISTER_SERIALIZABLE(LatticeSetParameters);

#endif // LATTICE_SET_PARAMETERS_HPP 

