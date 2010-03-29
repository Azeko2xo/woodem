/*************************************************************************
*  Copyright (C) 2006 by luc Scholtes                                    *
*  luc.scholtes@hmg.inpg.fr                                              *
*  Copyright (C) 2008 by Bruno Chareyre                                  *
*  bruno.chareyre@hmg.inpg.fr                                            *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/


#pragma once

#include <yade/pkg-common/Recorder.hpp>

/*! \brief Record the stress-strain state of a sample in simulations using TriaxialCompressionEngine

	The output is a text file where each line is a record, with the format 
	IterationNumber sigma11 sigma22 sigma33 epsilon11 epsilon22 epsilon33
	
 */


class TriaxialCompressionEngine;

class TriaxialStateRecorder : public Recorder
{
	private :
		shared_ptr<TriaxialCompressionEngine> triaxialCompressionEngine;
		bool changed;
	public :
		virtual ~TriaxialStateRecorder ();
		virtual void action();
	YADE_CLASS_BASE_DOC_ATTRS(TriaxialStateRecorder,Recorder,"Engine recording triaxial variables (needs :yref:TriaxialCompressionEngine present in the simulation).",
		((Real,porosity,1,"porosity of the packing [-]")));
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(TriaxialStateRecorder);
