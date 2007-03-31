/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "MeasurePoisson.hpp"
#include<yade/core/MetaBody.hpp>
#include "LatticeBeamParameters.hpp"
#include "LatticeNodeParameters.hpp"
// to calculate strain of whole speciemen - first two bodies in subscribedBodies are Nodes. FIXME - make it clean!
#include <boost/lexical_cast.hpp>

#include <cmath>

MeasurePoisson::MeasurePoisson () : DataRecorder()
{
	outputFile = "";
	interval = 50;
	horizontal = vertical -1.0;
	bottom = upper = left = right = -1;
	idx=0;
}


void MeasurePoisson::postProcessAttributes(bool deserializing)
{
	if(deserializing)
	{
	//	outputFile += "_"+boost::lexical_cast<std::string>(Omega::instance().getTimeStep());
	//	std::cerr << "using dt for poisson output file: " << outputFile << "\n";

		ofile.open(outputFile.c_str());

	}
}


void MeasurePoisson::registerAttributes()
{
	DataRecorder::registerAttributes();
	REGISTER_ATTRIBUTE(outputFile);
	REGISTER_ATTRIBUTE(interval);
	REGISTER_ATTRIBUTE(horizontal);
	REGISTER_ATTRIBUTE(vertical);
	REGISTER_ATTRIBUTE(bottom);
	REGISTER_ATTRIBUTE(upper);
	REGISTER_ATTRIBUTE(left);
	REGISTER_ATTRIBUTE(right);
}


bool MeasurePoisson::isActivated()
{
	return ((Omega::instance().getCurrentIteration() % interval == 0) && (ofile));
}


void MeasurePoisson::action(Body * body)
{


	return;

	if(bottom==-1 || upper==-1 || left==-1 || right==-1)
	{
		std::cerr << "bad nodes!\n";
		return;
	}
	MetaBody * ncb = static_cast<MetaBody*>(body);

	LatticeNodeParameters* node_left   = YADE_CAST<LatticeNodeParameters*>( (*(ncb->bodies))[left  ]->physicalParameters . get() );
	(*(ncb->bodies))[left  ]->geometricalModel->diffuseColor = Vector3r(1.0,1.0,0.0); // FIXME [1]
	LatticeNodeParameters* node_right  = YADE_CAST<LatticeNodeParameters*>( (*(ncb->bodies))[right ]->physicalParameters . get() );
	(*(ncb->bodies))[right ]->geometricalModel->diffuseColor = Vector3r(1.0,1.0,0.0); // FIXME [1]
	LatticeNodeParameters* node_bottom = YADE_CAST<LatticeNodeParameters*>( (*(ncb->bodies))[bottom]->physicalParameters . get() );
	(*(ncb->bodies))[bottom]->geometricalModel->diffuseColor = Vector3r(1.0,1.0,0.0); // FIXME [1]
	LatticeNodeParameters* node_upper  = YADE_CAST<LatticeNodeParameters*>( (*(ncb->bodies))[upper ]->physicalParameters . get() );
	(*(ncb->bodies))[upper ]->geometricalModel->diffuseColor = Vector3r(1.0,1.0,0.0); // FIXME [1]
	
	// FIXME - zamiast �ledzi� tylko dwa punkty (jeden na dole i jeden u g�ry), to lepiej zaznaczy� dwa obszary punkt�w i liczy� �redni� ich po�o�enia,
	// bo teraz, je�li kt�ry� punkt zostanie wykasowany, to nie jest mo�liwe kontynuowanie pomiar�w.

	Real	d  = horizontal;
	Real	dd = ((node_right ->se3.position[0] - node_left  ->se3.position[0])-horizontal);
	Real	L  = vertical;
	Real	dL = ((node_upper ->se3.position[1] - node_bottom->se3.position[1])-vertical  );

	Real 	poisson1 = -1.0* ( (((node_right ->se3.position[0] - node_left  ->se3.position[0])-horizontal)/horizontal)
			         / (((node_upper ->se3.position[1] - node_bottom->se3.position[1])-vertical  )/vertical  ));
	
	Real 	poisson2 = -1.0* ( (dd/d)
			          / (dL/L));

	last20[(idx++)%20] = poisson2;
	Real avg=0;
	if(idx>22)
		for(int i=0;i<20;i++)
			avg+=last20[i]*0.05;

	Real	poisson3 = std::log(d/(dd-d))/std::log(dL/L+1.0);
	
	ofile	<< lexical_cast<string>(poisson1) << " " 
		<< lexical_cast<string>(poisson2) << " " 
		<< lexical_cast<string>(poisson3) << " " 
		<< " avg: " << lexical_cast<string>(avg) << " " 
		<< endl; 
		
	// [1]
	// mark colors what is monitored - FIXME - provide a way to do that for ALL DataRecorders in that parent class.
	// GLDrawSomething can just put a getClassName()
	 
}

