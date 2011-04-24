#pragma once

// qt3 sucks
#ifdef QT_MOC_CPP
	#undef slots
	#include<Python.h>
	#define slots slots
#else
	#ifdef slots
	 #undef slots
	 #include<Python.h>
	 #define slots
	#else
	 #include<Python.h>
	#endif
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <set>
#include <list>
#include <time.h>
#include <boost/thread/thread.hpp>
#include <iostream>

#include<yade/lib/base/Math.hpp>
#include<yade/lib/factory/ClassFactory.hpp>

#include<yade/lib/base/Singleton.hpp>

#include<yade/core/BgThread.hpp>


#ifndef FOREACH
# define FOREACH BOOST_FOREACH
#endif

class Scene;
class ThreadRunner;

using namespace boost;
using namespace boost::posix_time;
using namespace std;

struct DynlibDescriptor{
	set<string> baseClasses;
	bool isIndexable, isSerializable;
};

class Omega: public Singleton<Omega>{
	shared_ptr<ThreadRunner> simulationLoop;
	SimulationFlow simulationFlow_;
	map<string,DynlibDescriptor> dynlibs; // FIXME : should store that in ClassFactory ?
	void initializePlugins(const vector<std::pair<std::string, std::string> >& dynlibsList); 
	
	shared_ptr<Scene> scene;
	shared_ptr<Scene> sceneAnother; // used for temporarily running different simulation, in Omega().switchscene()

	ptime startupLocalTime;

	map<string,string> memSavedSimulations;

	// to avoid accessing simulation when it is being loaded (should avoid crashes with the UI)
	boost::mutex loadingSimulationMutex;
	boost::mutex tmpFileCounterMutex;
	long tmpFileCounter;
	std::string tmpFileDir;

	public:
		// management, not generally useful
		void init();
		void reset();
		void timeInit();
		void initTemps();
		void cleanupTemps();
		const map<string,DynlibDescriptor>& getDynlibsDescriptor();
		void loadPlugins(vector<string> pluginFiles);
		bool isInheritingFrom(const string& className, const string& baseClassName );
		bool isInheritingFrom_recursive(const string& className, const string& baseClassName );
		void createSimulationLoop();
		bool hasSimulationLoop(){return (bool)(simulationLoop);}
		string gdbCrashBatch;
		char** origArgv; int origArgc;
		// do not change by hand
		/* Mutex for:
		* 1. GLViewer::paintGL (deffered lock: if fails, no GL painting is done)
		* 2. other threads that wish to manipulate GL
		* 3. Omega when substantial changes to the scene are being made (bodies being deleted, simulation loaded etc) so that GL doesn't access those and crash */
		boost::try_mutex renderMutex;


		void run();
		void pause();
		void step();
		void stop(); // resets the simulationLoop
		bool isRunning();
		std::string sceneFile; // updated at load/save automatically
		void loadSimulation(const string& name, bool quiet=false);
		void saveSimulation(const string& name, bool quiet=false);

		void resetScene();
		const shared_ptr<Scene>& getScene();
		//! Return unique temporary filename. May be deleted by the user; if not, will be deleted at shutdown.
		string tmpFilename();
		Real getRealTime();
		time_duration getRealTime_duration();

		// configuration directory used for logging config and possibly other things
		std::string confDir;

	DECLARE_LOGGER;

	Omega(){ LOG_DEBUG("Constructing Omega."); }
	~Omega(){}

	FRIEND_SINGLETON(Omega);
	friend class pyOmega;
};


