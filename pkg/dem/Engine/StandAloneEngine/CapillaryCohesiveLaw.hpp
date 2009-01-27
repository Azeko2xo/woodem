//
// C++ Interface: CapillaryCohesiveLaw
/*************************************************************************
*  Copyright (C) 2006 by luc Scholtes                                    *
*  luc.scholtes@hmg.inpg.fr                                              *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include <yade/core/InteractionSolver.hpp>
#include <set>
#include <boost/tuple/tuple.hpp>

// ajouts
#include <vector>
#include <list>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

class MeniscusParameters
{
public :
	Real V;
	Real F;
	Real delta1;
	Real delta2;
	int index1;
	int index2;

	MeniscusParameters();
	MeniscusParameters(const MeniscusParameters &source);
	~MeniscusParameters();
};


const int NB_R_VALUES = 10;

class PhysicalAction;
class capillarylaw; // fait appel a la classe def plus bas
class Interaction;

///This container class is used to check meniscii overlaps. Wet interactions are put in a series of lists, with one list per body.
class BodiesMenisciiList
{
	private:
		vector< list< shared_ptr<Interaction> > > interactionsOnBody;
		
		//shared_ptr<Interaction> empty;
		
	public:
		BodiesMenisciiList();
		BodiesMenisciiList(Body* body);
		bool prepare(Body* body);
		bool insert(const shared_ptr<Interaction>& interaction);
		bool remove(const shared_ptr<Interaction>& interaction);
		list< shared_ptr<Interaction> >& operator[] (int index);
		int size();
		void display();
		
		
		bool initialized;
};


class CapillaryCohesiveLaw : public InteractionSolver
{
	private :
		shared_ptr<PhysicalAction> actionForce;
		shared_ptr<PhysicalAction> actionMomentum;
		
	public :
		int sdecGroupMask;
		Real CapillaryPressure;
		bool fusionDetection;//If yes, a BodiesMenisciiList is maintained and updated at each time step
		bool binaryFusion;//if true, capillary forces are set to zero as soon as 1 fusion at least is detected
		void checkFusion(MetaBody * ncb);
		shared_ptr<capillarylaw> capillary;
		BodiesMenisciiList bodiesMenisciiList;
						
		CapillaryCohesiveLaw();
		void action(MetaBody * ncb);

	protected : 
		void registerAttributes();
	NEEDS_BEX("Force","Momentum");
	REGISTER_CLASS_NAME(CapillaryCohesiveLaw);
	REGISTER_BASE_CLASS_NAME(InteractionSolver);

};

class TableauD
{
	public:
		Real D;
		std::vector<std::vector<Real> > data;
		MeniscusParameters Interpolate3(Real P, int& index);
		
  		TableauD();
  		TableauD(std::ifstream& file);
  		~TableauD();
};

// Fonction d'ecriture de tableau, utilisee dans le constructeur pour test 
class Tableau;
std::ostream& operator<<(std::ostream& os, Tableau& T);

class Tableau
{	
	public: 
		Real R;
		std::vector<TableauD> full_data;
		MeniscusParameters Interpolate2(Real D, Real P, int& index1, int& index2);
		
		std::ifstream& operator<< (std::ifstream& file);
		
		Tableau();
    		Tableau(const char* filename);
    		~Tableau();
};

class capillarylaw
{
	public:
		capillarylaw();
		std::vector<Tableau> data_complete;
		MeniscusParameters Interpolate(Real R1, Real R2, Real D, Real P, int* index);
		
		void fill (const char* filename);
};






REGISTER_SERIALIZABLE(CapillaryCohesiveLaw);





