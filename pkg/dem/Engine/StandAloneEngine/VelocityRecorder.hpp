/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef VELOCITY_RECORDER_HPP
#define VELOCITY_RECORDER_HPP

#include<yade/core/DataRecorder.hpp>
#include <string>
#include <fstream>

class VelocityRecorder : public DataRecorder
{
	private :
		std::ofstream ofile;

	public :
		std::string outputFile;
		unsigned int interval;
		unsigned int bigBallId;
		VelocityRecorder ();
		virtual void registerAttributes();
		virtual void action(Body* b);
		virtual bool isActivated();
	
	protected :
		virtual void postProcessAttributes(bool deserializing);
	REGISTER_CLASS_NAME(VelocityRecorder);
	REGISTER_BASE_CLASS_NAME(DataRecorder);
};

REGISTER_SERIALIZABLE(VelocityRecorder,false);

#endif // VELOCITY_RECORDER_HPP 


