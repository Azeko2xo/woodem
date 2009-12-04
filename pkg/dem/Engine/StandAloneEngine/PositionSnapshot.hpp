/*************************************************************************
*  Copyright (C) 2008 by Jerome Duriez                                   *
*  duriez@geo.hmg.inpg.fr                                                *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/DataRecorder.hpp>
#include <string>
#include <fstream>

/*! \brief Exports a data file to study displacement fields

When called from a simulation this engine is executed on some iterations (defined in "list_id" variable). At each execution an output file is generated, containing, the Ids and positions of all dynamic bodies of the simulation.
*/

class PositionSnapshot : public DataRecorder
{
	private :
		std::string nom_int;		// variable intermediaire de nom de fichier
		std::ofstream ofile;
		std::ofstream myfile;		// an output object used for exporting datas
		unsigned int i;			//variable incrementee a chaq snapshot, utile pour les noms de fichiers resultats
	public :
		std::string outputFile;		// le suffixe commun a tous les fichiers resultats : defini dans le Preprocessor
		std::vector<int> list_id;	//tableau des nos d'id auxquelles un snapshot doit etre effectue
		
		PositionSnapshot ();
		~PositionSnapshot ();
		virtual void action(Scene*);
		virtual bool isActivated(Scene*);

	protected :
		virtual void postProcessAttributes(bool deserializing);
	REGISTER_ATTRIBUTES(DataRecorder,(outputFile)(list_id)(i));
	REGISTER_CLASS_NAME(PositionSnapshot);
	REGISTER_BASE_CLASS_NAME(DataRecorder);
};

REGISTER_SERIALIZABLE(PositionSnapshot);


