#include "Omega.hpp"
#include "Vector3.hpp"

#include "IOManager.hpp" // is this allowed? perhaps loadTheFile should be in pimpl of Omega ? (pointer to implementation)
#include "NonConnexBody.hpp"

#include <boost/filesystem/operations.hpp>

Omega::Omega ()
{
	cerr << "Constructing Omega  (if multiple times - check '-rdynamic' flag!)" << endl;
	init();
}

Omega::~Omega ()
{
	*logFile << "\t" << "<Summary Duration=\"" << startingSimulationTime-second_clock::local_time() << "\">" <<endl;
	*logFile << "</Simulation>" << endl << endl;
	logFile->close();
}

void Omega::logError(const string& str)
{
	*logFile << "\t" << "<Error Date=\"" << startingSimulationTime-second_clock::local_time() << "\" " << "Message =\""<< str << "\"" << endl;
};

void Omega::logMessage(const string& str)
{
	*logFile << "\t" << "<Message Date=\"" << startingSimulationTime-second_clock::local_time() << "\" " << "Message =\""<< str << "\"" << endl;
};

void Omega::init()
{
	fileName="";
	maxiter = 0; // unlimited
	iter = 0;
	automatic=false;
	progress=false;

	gravity_x = 0.0;
	gravity_y = -10.0;
	//gravity_y = 0.0;
	gravity_z = 0.0;
	//	setGravity = Vector3r(0,-9.81,0);
	//dt = 0.04;
	dt = 0.01;
	//dt = 0.015; // max for cloth, rotating box is little slower, but both work.

	logFile = shared_ptr<ofstream>(new ofstream("../data/log.xml", ofstream::out | ofstream::app));

	startingSimulationTime = second_clock::local_time();

	*logFile << "<Simulation" << " Date =\"" << startingSimulationTime << "\">" << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

long int& Omega::getIterReference()
{
	return iter;
}

long int Omega::getIter()
{
	return iter;
}

/// FIXME - everything below SHOULD NOT be inside Omega.

void Omega::setGravity(Vector3r g)
{
	gravity_x = g.x();
	gravity_y = g.y();
	gravity_z = g.z();
}

Vector3r Omega::getGravity()
{
	return Vector3r(gravity_x,gravity_y,gravity_z);
}

/// FIXME - maybe some settings class, or something....

void Omega::setTimestep(const string t)
{
	dt = lexical_cast<float>(t);
}

float Omega::getTimestep()
{
	return dt;
}

void Omega::setFileName(const string f)
{
	fileName = f;
};

string Omega::getFileName()
{
	return fileName;
}

void Omega::loadTheFile()
{

	if( Omega::instance().getFileName().size() != 0  &&  boost::filesystem::exists(fileName) )
	{

		IOManager::loadFromFile("XMLManager",fileName,"rootBody",Omega::instance().rootBody);
		Omega::instance().logMessage("Loading file " + fileName);
	}
	else
	{
		cout << "\nWrong filename, please specify filename with -f, or through your GUI.\n";
		exit(1);
	}
}

void Omega::setMaxiter(const string m)
{
	maxiter = lexical_cast<long int>(m);
}

long int Omega::getMaxiter()
{
	return maxiter;
}

void Omega::setAutomatic(bool b)
{
	automatic = b;
}

bool Omega::getAutomatic()
{
	return automatic;
}

void Omega::setProgress(bool b)
{
	progress = b;
}

bool Omega::getProgress()
{
	return progress;
}

