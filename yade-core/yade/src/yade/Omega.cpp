/***************************************************************************
 *   Copyright (C) 2004 by Olivier Galizzi                                 *
 *   olivier.galizzi@imag.fr                                               *
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

#include "Omega.hpp"
#include "yadeExceptions.hpp"
#include "MetaBody.hpp"
#include "FileGenerator.hpp"
#include "InteractionSolver.hpp"
#include "DeusExMachina.hpp"
#include "Body.hpp"
#include "GeometricalModel.hpp"
#include "InteractingGeometry.hpp"
#include "BoundingVolume.hpp"
#include "InteractionGeometry.hpp"
#include "InteractionPhysics.hpp"
#include "BroadInteractor.hpp"
#include "SimulationLoop.hpp"
#include "FrontEnd.hpp"
#include "MetaEngine.hpp"
#include "Preferences.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-lib-wm3-math/Vector3.hpp>
#include <yade/yade-lib-serialization/IOManager.hpp>
#include <yade/yade-lib-serialization/IOManager.hpp>
#include <yade/yade-lib-threads/ThreadSynchronizer.hpp>
#include <yade/yade-lib-multimethods/FunctorWrapper.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Omega::Omega()
{
	ThreadSafe::cerr("Constructing Omega  (if multiple times - check '-rdynamic' flag!)");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Omega::~Omega()
{
	//LOCK(omegaMutex);
	
	*logFile << "\t" << "<Summary Duration=\"" << sStartingSimulationTime-second_clock::local_time() << "\">" <<endl;
	*logFile << "</Simulation>" << endl << endl;
	logFile->close();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::init()
{
	simulationFileName="";
	currentIteration = 0;
	dt = 0.01;
	logFile = shared_ptr<ofstream>(new ofstream("../data/log.xml", ofstream::out | ofstream::app));
	// build simulation loop thread
	synchronizer = shared_ptr<ThreadSynchronizer>(new ThreadSynchronizer()); // FIXME - this should be optional
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::logError(const string& str)
{
	//LOCK(omegaMutex);
	
	*logFile << "\t" << "<Error Date=\"" << sStartingSimulationTime-second_clock::local_time() << "\" " << "Message =\""<< str << "\"" << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::logMessage(const string& str)
{
	//LOCK(omegaMutex);
		
	*logFile << "\t" << "<Message Date=\"" << sStartingSimulationTime-second_clock::local_time() << "\" " << "Message =\""<< str << "\"" << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::createSimulationLoop()
{
	//LOCK(omegaMutex);
	simulationLoop   = shared_ptr<SimulationLoop>(new SimulationLoop());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::finishSimulationLoop()
{
	//LOCK(omegaMutex);
	
	if (simulationLoop)
		simulationLoop->finish();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::joinSimulationLoop()
{
	//LOCK(omegaMutex);
	
	if (simulationLoop)
	{
		simulationLoop->join();
		simulationLoop   = shared_ptr<SimulationLoop>();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::doOneSimulationLoop()
{
	if (simulationLoop)
	{
		//if ((simulationLoop->isStopped()))
		//	simulationPauseDuration += microsec_clock::local_time()-msStartingPauseTime;
		//else
		msStartingPauseTime = microsec_clock::local_time();	
		simulationLoop->doOneLoop();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::startSimulationLoop()
{
//	LOCK(omegaMutex);

	if (simulationLoop && simulationLoop->isStopped())
	{
		simulationPauseDuration += microsec_clock::local_time()-msStartingPauseTime;
		simulationLoop->start();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::stopSimulationLoop()
{
//	LOCK(omegaMutex);
	if (simulationLoop && !(simulationLoop->isStopped()))
	{
		msStartingPauseTime = microsec_clock::local_time();
		simulationLoop->stop();	
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::registerPluginType(const string& name)
{	
	// called by BuildDynLibList
	shared_ptr<Factorable> f;
	try
	{
		f = ClassFactory::instance().createShared(name);
	}
	catch (FactoryError&)
	{
		return;
	}

	if (dynamic_pointer_cast<FileGenerator>(f))
		dynlibsType[name].baseClass = "FileGenerator";
	else if (dynamic_pointer_cast<InteractionSolver>(f))
		dynlibsType[name].baseClass = "InteractionSolver";
	else if (dynamic_pointer_cast<DeusExMachina>(f))
		dynlibsType[name].baseClass = "DeusExMachina";
	else if (dynamic_pointer_cast<Body>(f))
		dynlibsType[name].baseClass = "Body";
	else if (dynamic_pointer_cast<IOManager>(f))
		dynlibsType[name].baseClass = "IOManager";
	else if (dynamic_pointer_cast<BroadInteractor>(f))
		dynlibsType[name].baseClass = "BroadInteractor";
	else if (dynamic_pointer_cast<GeometricalModel>(f))
		dynlibsType[name].baseClass = "GeometricalModel";
	else if (dynamic_pointer_cast<InteractingGeometry>(f))
		dynlibsType[name].baseClass = "InteractingGeometry";
	else if (dynamic_pointer_cast<BoundingVolume>(f))
		dynlibsType[name].baseClass = "BoundingVolume";
	else if (dynamic_pointer_cast<InteractionGeometry>(f))
		dynlibsType[name].baseClass = "InteractionGeometry";
	else if (dynamic_pointer_cast<InteractionPhysics>(f))
		dynlibsType[name].baseClass = "InteractionPhysics";
	else if (dynamic_pointer_cast<MetaEngine>(f))
		dynlibsType[name].baseClass = "MetaEngine"; // FIXME : be calling getEngineUnitType possibility to classify all engine unit in the map
	else if (dynamic_pointer_cast<Engine>(f))
		dynlibsType[name].baseClass = "Engine";
	else
		dynlibsType[name].baseClass = "Unknown";

	dynlibsType[name].isIndexable    = (dynamic_pointer_cast<Indexable>(f));
	dynlibsType[name].isFactorable   = (dynamic_pointer_cast<Factorable>(f));
	dynlibsType[name].isSerializable = (dynamic_pointer_cast<Serializable>(f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::scanPlugins()
{

//	ClassFactory::instance().unloadAll();

	vector<string>::iterator dldi    = preferences->dynlibDirectories.begin();
	vector<string>::iterator dldiEnd = preferences->dynlibDirectories.end();
	for( ; dldi != dldiEnd ; ++dldi)
		ClassFactory::instance().addBaseDirectory((*dldi));
			
	vector< string > dynlibsList;

	vector<string>::iterator si = preferences->dynlibDirectories.begin();
	vector<string>::iterator siEnd = preferences->dynlibDirectories.end();
	for( ; si != siEnd ; ++si)
	{
		filesystem::path directory((*si));

		if ( filesystem::exists( directory ) )
		{
			filesystem::directory_iterator di( directory );
			filesystem::directory_iterator diEnd;
			for ( ; di != diEnd; ++di )
			{
				if (!filesystem::is_directory(*di) && !filesystem::symbolic_link_exists(*di) && filesystem::extension(*di)!=".a")
				{
					filesystem::path name(filesystem::basename((*di)));
					int prevLength = (*di).leaf().size();
					int length = name.leaf().size();
					while (length!=prevLength)
					{
						prevLength=length;
						name = filesystem::path(filesystem::basename(name));
						length = name.leaf().size();
					}					
					dynlibsList.push_back(ClassFactory::instance().systemNameToLibName(name.leaf()));
				}
			}
		}
		else
			cerr << "ERROR: trying to scan non existing directory for plugins: "<< directory.native_directory_string() << endl;
	}

	bool allLoaded = false;
	while (!allLoaded)
	{	
		vector< string >::iterator dlli    = dynlibsList.begin();
		vector< string >::iterator dlliEnd = dynlibsList.end();
		allLoaded = true;
		for( ; dlli!=dlliEnd ; ++dlli)
			allLoaded &= ClassFactory::instance().load((*dlli));
	}

	vector< string >::iterator dlli    = dynlibsList.begin();
	vector< string >::iterator dlliEnd = dynlibsList.end();
	for( ; dlli!=dlliEnd ; ++dlli)
		registerPluginType((*dlli));

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool Omega::getDynlibType(const string& libName, string& type)
{
	//LOCK(omegaMutex);
		
	map<string,DynlibType>::iterator it = dynlibsType.find(libName);
	if (it!=dynlibsType.end())
	{
		type = (*it).second.baseClass;
		return true;
	}
	else
		return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const map<string,DynlibType>& Omega::getDynlibsType()
{
	//LOCK(omegaMutex);
		
	return dynlibsType;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void Omega::incrementCurrentIteration()
{
	//LOCK(omegaMutex);
	
	++currentIteration;
}

long int Omega::getCurrentIteration()
{
	//LOCK(omegaMutex);
	
	return currentIteration;
}

void Omega::setSimulationFileName(const string f)
{
	//LOCK(omegaMutex);
	
	simulationFileName = f;
};

string Omega::getSimulationFileName()
{
	//LOCK(omegaMutex);
	
	return simulationFileName;
}

void Omega::loadSimulation()
{
	LOCK(Omega::instance().getRootBodyMutex());
	
	if( Omega::instance().getSimulationFileName().size() != 0  &&  filesystem::exists(simulationFileName) && filesystem::extension(simulationFileName)==".xml" )
	{
		freeRootBody();
		logMessage("Loading file " + simulationFileName);
		
		//{
		//	LOCK(rootBodyMutex);
		//	IOManager::loadFromFile("yade-lib-serialization-xml",simulationFileName,"rootBody",rootBody);
			IOManager::loadFromFile("XMLManager",simulationFileName,"rootBody",rootBody);
			sStartingSimulationTime = second_clock::local_time();
			msStartingSimulationTime = microsec_clock::local_time();
			simulationPauseDuration = msStartingSimulationTime-msStartingSimulationTime;
			msStartingPauseTime = msStartingSimulationTime;
			*logFile << "<Simulation" << " Date =\"" << sStartingSimulationTime << "\">" << endl;
			currentIteration = 0;
			simulationTime = 0;	
		//}
	}
	else
	{
		std::string error = yadeExceptions::BadFile + simulationFileName;
		cerr << error.c_str() << endl;
		throw yadeBadFile(error.c_str());
	}
}

void Omega::freeRootBody()
{
//	LOCK(rootBodyMutex);
	rootBody = shared_ptr<MetaBody>();
}

// FIXME - remove that
void Omega::setTimeStep(const Real t)
{
//	LOCK(omegaMutex);
	dt = t;
}

Real Omega::getTimeStep()
{
//	LOCK(omegaMutex);
	return dt;
}

shared_ptr<ThreadSynchronizer> Omega::getSynchronizer()
{
//	LOCK(omegaMutex); // we can safatly return it because it is constructed in Omega constructor
	return synchronizer;
}

const shared_ptr<MetaBody>& Omega::getRootBody()
{
//	LOCK(omegaMutex);
	return rootBody;
}

ptime Omega::getMsStartingSimulationTime()
{
//	LOCK(omegaMutex);
	return msStartingSimulationTime;
}

time_duration Omega::getSimulationPauseDuration()
{
	return simulationPauseDuration;
}


boost::mutex& Omega::getRootBodyMutex()
{
	return rootBodyMutex;
}

