/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

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
		VelocityRecorder ();
		virtual void action();
		virtual bool isActivated(Scene*);
	
	protected :
		virtual void postProcessAttributes(bool deserializing);
	REGISTER_ATTRIBUTES(DataRecorder,(outputFile)(interval));
	REGISTER_CLASS_NAME(VelocityRecorder);
	REGISTER_BASE_CLASS_NAME(DataRecorder);
};

REGISTER_SERIALIZABLE(VelocityRecorder);



