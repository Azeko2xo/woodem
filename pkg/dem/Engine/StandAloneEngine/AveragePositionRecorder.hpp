/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef AVERAGE_POSISTION_RECORDER_HPP
#define AVERAGE_POSISTION_RECORDER_HPP

#include<yade/core/DataRecorder.hpp>

#include <string>
#include <fstream>

class AveragePositionRecorder : public DataRecorder
{
	public :
		AveragePositionRecorder ();
		
		virtual void action(Body* b);
		virtual bool isActivated();
	
		virtual void registerAttributes();

	protected :
		virtual void postProcessAttributes(bool deserializing);


	public : 
		std::string outputFile;
		unsigned int interval;
	private : 
		std::ofstream ofile;
	
	REGISTER_CLASS_NAME(AveragePositionRecorder);
	REGISTER_BASE_CLASS_NAME(DataRecorder);

};

REGISTER_SERIALIZABLE(AveragePositionRecorder,false);

#endif // AVERAGE_POSISTION_RECORDER_HPP

