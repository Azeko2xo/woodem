/***************************************************************************
 *   Copyright (C) 2005 by Olivier Galizzi   *
 *   olivier.galizzi@imag.fr   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SPERICALDEMSIMULATOR_HPP__
#define __SPERICALDEMSIMULATOR_HPP__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "SphericalDEM.hpp"
#include "Contact.hpp"
#include "PersistentSAPCollider.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-core/StandAloneSimulator.hpp>
#include <yade/yade-core/MetaBody.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class SphericalDEMSimulator : public StandAloneSimulator
{
	private : shared_ptr<MetaBody> rootBody;
	private : vector<SphericalDEM> spheres;
	private : ContactVecSet contacts;
	private : Real alpha;
	private : Real beta;
	private : Real gamma;
	private : Real dt;
	private : Real newDt;
	private : bool computedSomething;
	private : Vector3r gravity;
	private : Real forceDamping;
	private : Real momentumDamping;
	private : bool useTimeStepper;
	private : PersistentSAPCollider sap;

	private : void findRealCollision(const vector<SphericalDEM>& spheres, ContactVecSet& contacts);
	private : void computeResponse(vector<SphericalDEM>& spheres, ContactVecSet& contacts);
	private : void addDamping(vector<SphericalDEM>& spheres);
	private : void applyResponse(vector<SphericalDEM>& spheres);
	private : void timeIntegration(vector<SphericalDEM>& spheres);
	private : Real computeDt(const vector<SphericalDEM>& spheres, const ContactVecSet& contacts);

	private : void findTimeStepFromBody(const SphericalDEM& sphere);
	private : void findTimeStepFromInteraction(unsigned int id1, const Contact& contact, const vector<SphericalDEM>& spheres);

	public : SphericalDEMSimulator();
	public : virtual ~SphericalDEMSimulator();

	public : virtual void setTimeStep(Real dt);
	public : virtual void doOneIteration();
	public : virtual void run(int nbIterations);

	public : virtual void loadConfigurationFile(const string& fileName);

	REGISTER_CLASS_NAME(SphericalDEMSimulator);
	REGISTER_BASE_CLASS_NAME(StandAloneSimulator);

};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_FACTORABLE(SphericalDEMSimulator);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __SPERICALDEMSIMULATOR_HPP__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
