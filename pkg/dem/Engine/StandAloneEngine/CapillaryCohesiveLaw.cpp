/*************************************************************************
*  Copyright (C) 2006 by luc Scholtes                                    *
*  luc.scholtes@hmg.inpg.fr                                              *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

//Modifs : Parameters renamed as MeniscusParameters
//id1/id2 classés pour que id1 soit toujours le plus petit grain, FIXME : angle de mouillage?
//FIXME : dans triaxialStressController, changer le test de nullité de la force dans updateStiffness


#include "CapillaryCohesiveLaw.hpp"
#include <yade/pkg-dem/BodyMacroParameters.hpp>
#include <yade/pkg-dem/SpheresContactGeometry.hpp>

#include <yade/pkg-dem/CapillaryParameters.hpp>
#include <yade/core/Omega.hpp>
#include <yade/core/MetaBody.hpp>
#include <Wm3Vector3.h>
#include <yade/lib-base/yadeWm3.hpp>

#include <yade/core/GeometricalModel.hpp>


#include <iostream>
#include <fstream>

using namespace std;

//int compteur1 = 0;
//int compteur2 = 0;

CapillaryCohesiveLaw::CapillaryCohesiveLaw() : InteractionSolver()
{
        sdecGroupMask=1;

        CapillaryPressure=0;
        fusionDetection = false;
        binaryFusion = true;

		  // capillary setup moved to postProcessAttributes

}

void CapillaryCohesiveLaw::postProcessAttributes(bool deserializing){
	if(!deserializing) return;

  capillary = shared_ptr<capillarylaw>(new capillarylaw); // ????????
  capillary->fill("M(r=1)");
  capillary->fill("M(r=1.1)");
  capillary->fill("M(r=1.25)");
  capillary->fill("M(r=1.5)");
  capillary->fill("M(r=1.75)");
  capillary->fill("M(r=2)");
  capillary->fill("M(r=3)");
  capillary->fill("M(r=4)");
  capillary->fill("M(r=5)");
  capillary->fill("M(r=10)");
}

void CapillaryCohesiveLaw::registerAttributes()
{
        InteractionSolver::registerAttributes();
        REGISTER_ATTRIBUTE(sdecGroupMask);
        REGISTER_ATTRIBUTE(CapillaryPressure);
        REGISTER_ATTRIBUTE(fusionDetection);
        REGISTER_ATTRIBUTE(binaryFusion);

}

MeniscusParameters::MeniscusParameters()
{
        V = 0;
        F = 0;
        delta1 = 0;
        delta2 = 0;
};

MeniscusParameters::MeniscusParameters(const MeniscusParameters &source)
{
        V = source.V;
        F = source.F;
        delta1 = source.delta1;
        delta2 = source.delta2;
}

MeniscusParameters::~MeniscusParameters()
{}


//FIXME : remove bool first !!!!!
void CapillaryCohesiveLaw::action(MetaBody* ncb)
{
//	cerr << "capillaryLawAction" << endl;
        //compteur1 = 0;
        //compteur2 = 0;
        //cerr << "CapillaryCohesiveLaw::action" << endl;

//         MetaBody * ncb = static_cast<MetaBody*>(body);
        shared_ptr<BodyContainer>& bodies = ncb->bodies;

        if (fusionDetection) {
                if (!bodiesMenisciiList.initialized)
                        bodiesMenisciiList.prepare(ncb);
                //bodiesMenisciiList.display();
        }


        /// Non Permanents Links ///

        InteractionContainer::iterator ii    = ncb->transientInteractions->begin();
        InteractionContainer::iterator iiEnd = ncb->transientInteractions->end();

        /// initialisation du volume avant calcul
        //Real Vtotal = 0;

        for(  ; ii!=iiEnd ; ++ii ) {

                if ((*ii)->isReal()) {//FIXME : test to be removed when using DistantPersistentSAPCollider?

                        const shared_ptr<Interaction>& interaction = *ii;
                        unsigned int id1 = interaction->getId1();
                        unsigned int id2 = interaction->getId2();
			
			if( !( (*bodies)[id1]->getGroupMask() & (*bodies)[id2]->getGroupMask() & sdecGroupMask)  )
                                continue; // skip other groups, BTW: this is example of a good usage of 'continue' keyword
			
                        /// interaction geometry search
			int geometryIndex1 = (*bodies)[id1]->geometricalModel->getClassIndex(); // !!!
                        //cerr << "geo1 =" << geometryIndex1 << endl;
                        int geometryIndex2 = (*bodies)[id2]->geometricalModel->getClassIndex();
                        //cerr << "geo2 =" << geometryIndex2 << endl;

                        if (!(geometryIndex1 == geometryIndex2))
                                continue;

                        /// definition of interacting objects (not in contact)

                        BodyMacroParameters* de1 		=
                                static_cast<BodyMacroParameters*>((*bodies)[id1]->physicalParameters.get());
                        BodyMacroParameters* de2 		=
                                static_cast<BodyMacroParameters*>((*bodies)[id2]->physicalParameters.get());

                        SpheresContactGeometry* currentContactGeometry 	=
                                static_cast<SpheresContactGeometry*>(interaction->interactionGeometry.get());

                        CapillaryParameters* currentContactPhysics 	=
                                static_cast<CapillaryParameters*>(interaction->interactionPhysics.get());

                        //SDECLinkPhysics* currentContactPhysics	= static_cast<SDECLinkPhysics*>(interaction->interactionPhysics.get());

                        //                         CapillaryParameters* meniscusParameters
                        // = static_cast<CapillaryParameters*>(interaction->interactionPhysics.get());

                        /// Capillary components definition:

                        Real liquidTension = 0.073; 	// superficial water tension N/m (20�C)

                        //Real teta = 0;		// mouillage parfait (eau pure/billes de verre)

                        /// Interacting Grains:
                        // definition du rapport tailleReelle/TailleYADE

                        Real alpha=1; // OK si pas de gravite!!!

//                         Real R1, R2;
//                         if (currentContactGeometry->radius2 > currentContactGeometry->radius1) {
//                                 R1=currentContactGeometry->radius1;
//                                 R2=currentContactGeometry->radius2;
//                         } else {
//                                 R1=currentContactGeometry->radius2;
//                                 R2=currentContactGeometry->radius1;
//                         }


                        Real R1 = 0;
                        R1=alpha*std::min(currentContactGeometry->radius2,currentContactGeometry->        radius1 ) ;
                        Real R2 = 0;
                        R2=alpha*std::max(currentContactGeometry->radius2,currentContactGeometry->       radius1 ) ;
                        //cerr << "R1 = " << R1 << " R2 = "<< R2 << endl;

                        /// intergranular distance

                        Real D =                                alpha*(de2->se3.position-de1->se3.position).Length()-alpha*(                       currentContactGeometry->radius1+ currentContactGeometry->radius2);

// 			Real intergranularDistance = currentContactGeometry->penetrationDepth;
			//cerr << "D = " << intergranularDistance << endl;

                        if ((currentContactGeometry->penetrationDepth>=0)||(D<=0)) //||(Omega::instance().getCurrentIteration() < 1) ) // a simplified way to define meniscii everywhere
			{
                                D=0;
				//intergranularDistance = 0;	// def Fcap when spheres interpenetrate//FIXME : lead to wrong interpolation? D<0 has no solution in the interpolation : this is not physically interpretable!! even if, interpenetration << grain radius.
                                if (fusionDetection && !currentContactPhysics->meniscus)                   bodiesMenisciiList.insert((*ii));
                                currentContactPhysics->meniscus=true;
                        }

			//currentContactPhysics->meniscus=true; /// a way to create menisci everywhere			

                        //Real Dinterpol = -(intergranularDistance)/R2;
			Real Dinterpol = D/R2;

                        /// Suction (Capillary pressure):

                        Real Pinterpol =CapillaryPressure*(R2/liquidTension);
                        currentContactPhysics->CapillaryPressure = CapillaryPressure;

                        //Real r = R2/R1;

                        /// Capillary solution finder:
                        //cerr << "solution finder " << endl;

                        if ((Pinterpol>=0) && (currentContactPhysics->meniscus==true)) 
			{	//cerr << "Pinterpol = "<< Pinterpol << endl;

                                MeniscusParameters
                                solution(capillary->Interpolate(R1,R2,
                                                                Dinterpol, Pinterpol, currentContactPhysics->currentIndexes));

                                /// capillary adhesion force

                                Real Finterpol = solution.F;
                                Vector3r Fcap =
                                        Finterpol*(2*Mathr::PI*(R2/alpha)*liquidTension)*currentContactGeometry->
                                        normal; /// unites !!!

                                currentContactPhysics->Fcap = Fcap;

                                /// meniscus volume

                                Real Vinterpol = solution.V;
                                currentContactPhysics->Vmeniscus =                                       Vinterpol*(R2*R2*R2)/(alpha*alpha*alpha);

                                if (currentContactPhysics->Vmeniscus != 0) {
                                        currentContactPhysics->meniscus = true;
                                        //cerr <<"currentContactPhysics->meniscus = true;"<<endl;
                                } else {
                                        if (fusionDetection)
                                                bodiesMenisciiList.remove((*ii));
                                        currentContactPhysics->meniscus = false;
                                        //cerr <<"currentContactPhysics->meniscus = false;"<<endl;
                                }

                                /// wetting angles
                                /// wetting angles
                                currentContactPhysics->Delta1 = max(solution.delta1,solution.delta2);
                                currentContactPhysics->Delta2 = min(solution.delta1,solution.delta2);

//                                 if (currentContactGeometry->radius2 > currentContactGeometry->radius1)
// 				{
//                                         currentContactPhysics->Delta1 = solution.delta1;
//                                         currentContactPhysics->Delta2 = solution.delta2;
//                                 } else {
//                                         currentContactPhysics->Delta1 = solution.delta2;
//                                         currentContactPhysics->Delta2 = solution.delta1;
//                                 }

                                currentContactPhysics->prevNormal = currentContactGeometry->normal;

                        } else if (fusionDetection)
                                bodiesMenisciiList.remove((*ii));//If the interaction is not real, it should not be in the list
                }
        }

        if (fusionDetection)
                checkFusion(ncb);

        for(ii= ncb->transientInteractions->begin(); ii!=iiEnd ; ++ii ) 
	{	//cerr << "interaction " << ii << endl;
                if ((*ii)->isReal()) 
		{
                        CapillaryParameters* currentContactPhysics	=	static_cast<CapillaryParameters*>((*ii)->interactionPhysics.get());
                        if (currentContactPhysics->meniscus) 
			{
                                if (fusionDetection) 
				{//version with effect of fusion
                                        //BINARY VERSION : if fusionNumber!=0 then no capillary force
                                        if (binaryFusion)
					{
						if (currentContactPhysics->fusionNumber !=0) 
						{	//cerr << "fusion" << endl;
                                                        currentContactPhysics->Fcap = Vector3r::ZERO;
                                                        continue;
                                                }
                                        }
                                        //LINEAR VERSION : capillary force is divided by (fusionNumber + 1) - NOTE : any decreasing function of fusionNumber can be considered in fact
					else if (currentContactPhysics->fusionNumber !=0)
						currentContactPhysics->Fcap /= (currentContactPhysics->fusionNumber+1);
                                }
											ncb->bex.addForce((*ii)->getId1(), currentContactPhysics->Fcap);
											ncb->bex.addForce((*ii)->getId2(),-currentContactPhysics->Fcap);

				//cerr << "id1/id2 " << (*ii)->getId1() << "/" << (*ii)->getId2() << " Fcap= " << currentContactPhysics->Fcap << endl;

                        }
					 }
        }


        //if (fusionDetection)
                //bodiesMenisciiList.display();
        //cerr << "end of capillarylaw" << endl;
}
capillarylaw::capillarylaw()
{}

void capillarylaw::fill(const char* filename)
{
        data_complete.push_back(Tableau(filename));

}

void CapillaryCohesiveLaw::checkFusion(MetaBody * ncb)
{

	//Reset fusion numbers
	InteractionContainer::iterator ii    = ncb->transientInteractions->begin();
        InteractionContainer::iterator iiEnd = ncb->transientInteractions->end();
        for( ; ii!=iiEnd ; ++ii ) if ((*ii)->isReal()) static_cast<CapillaryParameters*>((*ii)->interactionPhysics.get())->fusionNumber=0;
	
	
	list< shared_ptr<Interaction> >::iterator firstMeniscus, lastMeniscus, currentMeniscus;
	Real angle1, angle2;

	for ( int i=0; i< bodiesMenisciiList.size(); ++i )// i is the index (or id) of the body being tested
	{
		if ( !bodiesMenisciiList[i].empty() )
		{
			lastMeniscus = bodiesMenisciiList[i].end();
			//cerr << "size = "<<bodiesMenisciiList[i].size() << " empty="<<bodiesMenisciiList[i].empty() <<endl;
			for ( firstMeniscus=bodiesMenisciiList[i].begin(); firstMeniscus!=lastMeniscus; ++firstMeniscus )//FOR EACH MENISCUS ON THIS BODY...
			{
				CapillaryParameters* interactionPhysics1 = static_cast<CapillaryParameters*>((*firstMeniscus)->interactionPhysics.get());
				currentMeniscus = firstMeniscus; ++currentMeniscus;
				
				if (i == (*firstMeniscus)->getId1()) angle1=interactionPhysics1->Delta1;//get angle of meniscus1 on body i
				else angle1=interactionPhysics1->Delta2;

				for ( ;currentMeniscus!= lastMeniscus; ++currentMeniscus) {//... CHECK FUSION WITH ALL OTHER MENISCII ON THE BODY
					CapillaryParameters* interactionPhysics2 = static_cast<CapillaryParameters*>((*currentMeniscus)->interactionPhysics.get());

					if (i == (*currentMeniscus)->getId1()) angle2=interactionPhysics2->Delta1;//get angle of meniscus2 on body i
					else angle2=interactionPhysics2->Delta2;

					if (angle1==0 || angle2==0) cerr << "THIS SHOULD NOT HAPPEN!!"<< endl;

					//cerr << "angle1 = " << angle1 << " | angle2 = " << angle2 << endl;	
					
					Vector3r normalFirstMeniscus = static_cast<SpheresContactGeometry*>((*firstMeniscus)->interactionGeometry.get())->normal;
					Vector3r normalCurrentMeniscus = static_cast<SpheresContactGeometry*>((*currentMeniscus)->interactionGeometry.get())->normal;
					
					//if (i != (*firstMeniscus)->getId1()) normalFirstMeniscus = -normalFirstMeniscus;
					//if (i != (*currentMeniscus)->getId1()) normalCurrentMeniscus = -normalCurrentMeniscus;
					// normal is always from id1 to id2
					
					Real normalDot = 0;
					if ((*firstMeniscus)->getId1() ==  (*currentMeniscus)->getId1() ||  (*firstMeniscus)->getId2()  == (*currentMeniscus)->getId2()) normalDot = normalFirstMeniscus.Dot(normalCurrentMeniscus);
					else
					normalDot = - (normalFirstMeniscus.Dot(normalCurrentMeniscus));
					
					//cerr << "normalDot ="<< normalDot << endl;
					
					Real normalAngle = 0;
					if (normalDot >= 0 ) normalAngle = Mathr::FastInvCos0(normalDot);
					else normalAngle = ((Mathr::PI) - Mathr::FastInvCos0(-(normalDot)));
					
					//cerr << "sumMeniscAngle= " << (angle1+angle2)<< "| normalAngle" << normalAngle*Mathr::RAD_TO_DEG << endl;

					if ((angle1+angle2)*Mathr::DEG_TO_RAD > normalAngle)

					//if ((angle1+angle2)*Mathr::DEG_TO_RAD > Mathr::FastInvCos0(normalFirstMeniscus.Dot(normalCurrentMeniscus)))
					
// 					if (//check here if wet angles are overlaping (check with squares is faster since SquaredLength of cross product gives squared sinus)
// 					(angle1+angle2)*Mathr::DEG_TO_RAD > Mathr::FastInvCos0(static_cast<SpheresContactGeometry*>((*firstMeniscus)->interactionGeometry.get())->normal
// 					.Dot(
// 					static_cast<SpheresContactGeometry*>((*currentMeniscus)->interactionGeometry.get())->normal))) 
					{
						++(interactionPhysics1->fusionNumber); ++(interactionPhysics2->fusionNumber);//count +1 if 2 meniscii are overlaping
					};
				}
// 				if ( *firstMeniscus )
// 					if ( firstMeniscus->get() )
// 						cerr << "(" << ( *firstMeniscus )->getId1() << ", " << ( *firstMeniscus )->getId2() <<") ";
// 					else cerr << "(void)";
			}
			//cerr << endl;
		}
		//else cerr << "empty" << endl;
	}
}

MeniscusParameters capillarylaw::Interpolate(Real R1, Real R2, Real D, Real P, int* index)
{	//cerr << "interpolate" << endl;
        if (R1 > R2) {
                Real R3 = R1;
                R1 = R2;
                R2 = R3;
        }

        Real R = R2/R1;
        //cerr << "R = " << R << endl;

        MeniscusParameters result_inf;
        MeniscusParameters result_sup;
        MeniscusParameters result;
        int i = 0;

        for ( ; i < (NB_R_VALUES); i++)
        {
                Real data_R = data_complete[i].R;
                //cerr << "i = " << i << endl;

                if (data_R > R)	// Attention � l'ordre ds lequel
                        //vont �tre rang�s les tableau R (croissant)

                {
                        Tableau& tab_inf=data_complete[i-1];
                        Tableau& tab_sup=data_complete[i];

                        Real r=(R-tab_inf.R)/(tab_sup.R-tab_inf.R);

                        result_inf = tab_inf.Interpolate2(D,P,index[0], index[1]);
                        result_sup = tab_sup.Interpolate2(D,P,index[2], index[3]);

                        result.V = result_inf.V*(1-r) + r*result_sup.V;
                        result.F = result_inf.F*(1-r) + r*result_sup.F;
                        result.delta1 = result_inf.delta1*(1-r) + r*result_sup.delta1;
                        result.delta2 = result_inf.delta2*(1-r) + r*result_sup.delta2;

                        i=NB_R_VALUES;
                        //cerr << "i = " << i << endl;

                }
                else if (data_complete[i].R == R)
                {
                        result = data_complete[i].Interpolate2(D,P, index[0], index[1]);
                        i=NB_R_VALUES;
                        //cerr << "i = " << i << endl;
                }
        }
        return result;
}

Tableau::Tableau()
{}

Tableau::Tableau(const char* filename)

{
        ifstream file (filename);
        file >> R;
        //cerr << "r = " << R << endl;
        int n_D;	//number of D values
        file >> n_D;

        if (!file.is_open())
	{
		static bool first=true;
		if(first)
		{
	                cout << "WARNING: cannot open file used for capillary law, in TriaxalTestWater" << endl;
			first=false;
		}
		return;
	}
        for (int i=0; i<n_D; i++)
                full_data.push_back(TableauD(file));
        file.close();
        //cerr << *this;	// exemple d'utilisation de la fonction d'�criture (this est le pointeur vers l'objet courant)
}

Tableau::~Tableau()
{}

MeniscusParameters Tableau::Interpolate2(Real D, Real P, int& index1, int& index2)

{	//cerr << "interpolate2" << endl;
        MeniscusParameters result;
        MeniscusParameters result_inf;
        MeniscusParameters result_sup;

        for ( unsigned int i=0; i < full_data.size(); ++i)
        {
                if (full_data[i].D > D )	// ok si D rang�s ds l'ordre croissant

                {
                        Real rD = (D-full_data[i-1].D)/(full_data[i].D-full_data[i-1].D);

                        result_inf = full_data[i-1].Interpolate3(P, index1);
                        result_sup = full_data[i].Interpolate3(P, index2);

                        result.V = result_inf.V*(1-rD) + rD*result_sup.V;
                        result.F = result_inf.F*(1-rD) + rD*result_sup.F;
                        result.delta1 = result_inf.delta1*(1-rD) + rD*result_sup.delta1;
                        result.delta2 = result_inf.delta2*(1-rD) + rD*result_sup.delta2;

                        i = full_data.size();
                }
                else if (full_data[i].D == D)
                {
                        result=full_data[i].Interpolate3(P, index1);

                        i=full_data.size();
                }

        }
        return result;
}

TableauD::TableauD()
{} // ?? constructeur

TableauD::TableauD(ifstream& file)

{
        int i=0;
        Real x;
        int n_lines;	//pb: n_lines is real!!!
        file >> n_lines;
        //cout << n_lines << endl;

        file.ignore(200, '\n'); // saute les caract�res (200 au maximum) jusque au caract�re \n (fin de ligne)*_

        if (n_lines!=0)
                for (; i<n_lines; ++i) {
                        data.push_back(vector<Real> ());
                        for (int j=0; j < 6; ++j)	// [D,P,V,F,delta1,delta2]
                        {
                                file >> x;
                                data[i].push_back(x);
                        }
                }
        D = data[i-1][0];
}

MeniscusParameters TableauD::Interpolate3(Real P, int& index)

{	//cerr << "interpolate3" << endl;
        MeniscusParameters result;
        int dataSize = data.size();
        
        if (index < dataSize && index>0)
        {
        	if (data[index][1] >= P && data[index-1][1] < P)
        	{
        		//compteur1+=1;	
        		Real Pinf=data[index-1][1];
                        Real Finf=data[index-1][3];
                        Real Vinf=data[index-1][2];
                        Real Delta1inf=data[index-1][4];
                        Real Delta2inf=data[index-1][5];

                        Real Psup=data[index][1];
                        Real Fsup=data[index][3];
                        Real Vsup=data[index][2];
                        Real Delta1sup=data[index][4];
                        Real Delta2sup=data[index][5];

                        result.V = Vinf+((Vsup-Vinf)/(Psup-Pinf))*(P-Pinf);
                        result.F = Finf+((Fsup-Finf)/(Psup-Pinf))*(P-Pinf);
                        result.delta1 = Delta1inf+((Delta1sup-Delta1inf)/(Psup-Pinf))*(P-Pinf);
                        result.delta2 = Delta2inf+((Delta2sup-Delta2inf)/(Psup-Pinf))*(P-Pinf);
                        return result;
        		
        	}
        }
	//compteur2+=1;	
        for (int k=1; k < dataSize; ++k) 	// Length(data) ??

        {	//cerr << "k = " << k << endl;
                if ( data[k][1] > P) 	// OK si P rangés ds l'ordre croissant

                {	//cerr << "if" << endl;
                        Real Pinf=data[k-1][1];
                        Real Finf=data[k-1][3];
                        Real Vinf=data[k-1][2];
                        Real Delta1inf=data[k-1][4];
                        Real Delta2inf=data[k-1][5];

                        Real Psup=data[k][1];
                        Real Fsup=data[k][3];
                        Real Vsup=data[k][2];
                        Real Delta1sup=data[k][4];
                        Real Delta2sup=data[k][5];

                        result.V = Vinf+((Vsup-Vinf)/(Psup-Pinf))*(P-Pinf);
                        result.F = Finf+((Fsup-Finf)/(Psup-Pinf))*(P-Pinf);
                        result.delta1 = Delta1inf+((Delta1sup-Delta1inf)/(Psup-Pinf))*(P-Pinf);
                        result.delta2 = Delta2inf+((Delta2sup-Delta2inf)/(Psup-Pinf))*(P-Pinf);
                        index = k;

                        k=dataSize;
                }
                else if (data[k][1] == P)

                {	//cerr << "elseif" << endl;
                        result.V = data[k][2];
                        result.F = data[k][3];
                        result.delta1 = data[k][4];
                        result.delta2 = data[k][5];
                        index = k;

                        k=dataSize;
                }

        }
        return result;
}

TableauD::~TableauD()
{} // ?? destructeur

std::ostream& operator<<(std::ostream& os, Tableau& T)
{
        os << "Tableau : R=" << T.R << endl;
        for (unsigned int i=0; i<T.full_data.size(); i++) {
                os << "TableauD : D=" << T.full_data[i].D << endl;
                for (unsigned int j=0; j<T.full_data[i].data.size();j++) {
                        for (unsigned int k=0; k<T.full_data[i].data[j].size(); k++)
                                os << T.full_data[i].data[j][k] << " ";
                        os << endl;
                }
        }
        os << endl;
        return os;
}

BodiesMenisciiList::BodiesMenisciiList(Body * body)
{
	initialized=false;
	prepare(body);
}

bool BodiesMenisciiList::prepare(Body * body)
{
	//cerr << "preparing bodiesInteractionsList" << endl;
	interactionsOnBody.clear();
	MetaBody * ncb = static_cast<MetaBody*>(body);
	shared_ptr<BodyContainer>& bodies = ncb->bodies;
	
	body_id_t MaxId = -1;
	BodyContainer::iterator bi    = bodies->begin();
	BodyContainer::iterator biEnd = bodies->end();
	for(  ; bi!=biEnd ; ++bi )
	{
		MaxId=max(MaxId, (*bi)->getId());
	}
	interactionsOnBody.resize(MaxId+1);
	for ( unsigned int i=0; i<interactionsOnBody.size(); ++i )
	{
		interactionsOnBody[i].clear();
	}
	
        InteractionContainer::iterator ii    = ncb->transientInteractions->begin();
        InteractionContainer::iterator iiEnd = ncb->transientInteractions->end();
        for(  ; ii!=iiEnd ; ++ii ) {
                if ((*ii)->isReal()) {
                	if (static_cast<CapillaryParameters*>((*ii)->interactionPhysics.get())->meniscus) insert(*ii);
                }
        }
                	
	return initialized=true;
}

bool BodiesMenisciiList::insert(const shared_ptr< Interaction >& interaction)
{
	interactionsOnBody[interaction->getId1()].push_back(interaction);
	interactionsOnBody[interaction->getId2()].push_back(interaction);
	return true;	
}


bool BodiesMenisciiList::remove(const shared_ptr< Interaction >& interaction)
{
	interactionsOnBody[interaction->getId1()].remove(interaction);
	interactionsOnBody[interaction->getId2()].remove(interaction);	
	return true;
}

list< shared_ptr<Interaction> >&  BodiesMenisciiList::operator[] (int index)
{
	return interactionsOnBody[index];
}

int BodiesMenisciiList::size()
{
	return interactionsOnBody.size();
}

void BodiesMenisciiList::display()
{
	list< shared_ptr<Interaction> >::iterator firstMeniscus;
	list< shared_ptr<Interaction> >::iterator lastMeniscus;
	for ( unsigned int i=0; i<interactionsOnBody.size(); ++i )
	{
		if ( !interactionsOnBody[i].empty() )
		{
			lastMeniscus = interactionsOnBody[i].end();
			//cerr << "size = "<<interactionsOnBody[i].size() << " empty="<<interactionsOnBody[i].empty() <<endl;
			for ( firstMeniscus=interactionsOnBody[i].begin(); firstMeniscus!=lastMeniscus; ++firstMeniscus )
			{
				if ( *firstMeniscus )
					if ( firstMeniscus->get() )
						cerr << "(" << ( *firstMeniscus )->getId1() << ", " << ( *firstMeniscus )->getId2() <<") ";
					else cerr << "(void)";
			}
			cerr << endl;
		}
		else cerr << "empty" << endl;
	}
}

BodiesMenisciiList::BodiesMenisciiList()
{
	initialized=false;
}
