/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef BEAM_RECORDER_HPP
#define BEAM_RECORDER_HPP 

#include <yade/yade-core/DataRecorder.hpp>

#include <string>
#include <fstream>
#include <list>
#include <vector>

//class PhysicalAction;

class BeamRecorder : public DataRecorder
{ // given a section plane it records all the beams that cross it: their deformation projected
  // on direction perpendicular to the plane multiplied by their longitudalStiffness
	private :
		std::ofstream ofile; 
		std::vector<std::pair<std::list<unsigned int>, int> >	subscribedBodies; // [region]<list of bodies' ids, direction>
		bool			first;
		int			nodeGroupMask,beamGroupMask;

	public :
		std::string	 outputFile;
		unsigned int	 interval;
		std::list<std::pair<Vector3r,std::pair< Real, int > > > sections; // a list of section planes: <a midpoint, half length, direction >
		// int direction refers to: which coordinate to take from the Beam's.direction: x, y or z (0, 1 or 2)

		BeamRecorder ();

		virtual void registerAttributes();
		virtual void action(Body* b);
		virtual bool isActivated();

	protected :
		virtual void postProcessAttributes(bool deserializing);
	REGISTER_CLASS_NAME(BeamRecorder);
	REGISTER_BASE_CLASS_NAME(DataRecorder);
};

REGISTER_SERIALIZABLE(BeamRecorder,false);

#endif // STRAIN_RECORDER_HPP 

