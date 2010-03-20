/*************************************************************************
*  Copyright (C) 2008 by Bruno Chareyre                                  *
*  bruno.chareyre@hmg.inpg.fr                                            *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/GlobalEngine.hpp>
#include<yade/pkg-dem/TriaxialCompressionEngine.hpp>
#include<yade/lib-triangulation/KinematicLocalisationAnalyser.hpp>
#include <set>
#include <boost/tuple/tuple.hpp>
#include <string>
#include <fstream>

/*! \brief Compute fabric tensor, local porosity, local deformation, and other micromechanicaly defined quantities based on triangulation/tesselation of the packing.
	
 */

namespace CGT {
class TriaxialState;
class Tenseur3;
}

class MicroMacroAnalyser : public GlobalEngine
{
/// Attributes
	private :
		std::ofstream ofile;		
		shared_ptr<TriaxialCompressionEngine> triaxialCompressionEngine;
		bool initialized;
		/// A number apended at the end of deformation output files 
		unsigned int stateNumber;
		unsigned int incrtNumber;
		
	public :
		std::string	 outputFile;
		std::string	 stateFileName;
		
		MicroMacroAnalyser();
		void action();
		/// Set current state as initial (state=1) or final (state=2) congiguration for later kinematic analysis on the increment; if requested : save snapshots (with specific format) - possibly including contact forces increments on the state1->state2 interval
		void setState(unsigned int state, bool save_states = false, bool computeIncrement = false);
		/// Copy the current simulation in a TriaxialState structure. If filename!=NULL, save it to a file that can be reloaded later for computing strain increments, state must be 1 or 2.
		CGT::TriaxialState& makeState(unsigned int state, const char* filename = NULL);
		//const vector<CGT::Tenseur3>& makeDeformationArray(const char* state_file1, const char* state_file0);
		int interval;
		/// Is the engine just saving states or also computing and outputing deformations for each increment?
		bool compDeformation;
		/// Should increments of force and displacements be defined on [n,n+1]? If not, states will be saved with only positions and forces
		bool compIncrt;
		shared_ptr<CGT::KinematicLocalisationAnalyser> analyser;
		
	DECLARE_LOGGER;
	
	protected :
		virtual void postProcessAttributes(bool deserializing);
		REGISTER_ATTRIBUTES(GlobalEngine,(stateNumber)(incrtNumber)(outputFile)(stateFileName)(interval)(compDeformation)(compIncrt));
	REGISTER_CLASS_NAME(MicroMacroAnalyser);
	REGISTER_BASE_CLASS_NAME(GlobalEngine);
};

REGISTER_SERIALIZABLE(MicroMacroAnalyser);


