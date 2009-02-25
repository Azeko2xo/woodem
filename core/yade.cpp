/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifdef EMBED_PYTHON
	#include<Python.h>
	extern int Py_OptimizeFlag;
#endif

#include<signal.h>
#include<cstdlib>
#include<iostream>
#include<string>
#include<getopt.h>
#include<boost/algorithm/string.hpp>
#include<boost/filesystem/operations.hpp>
#include<boost/filesystem/convenience.hpp>
#include<boost/preprocessor/stringize.hpp>
#include<boost/regex.hpp>
#include<yade/lib-factory/ClassFactory.hpp>
#include<yade/lib-base/Logging.hpp>
#include"Omega.hpp"
#include"FrontEnd.hpp"
#include"Preferences.hpp"

using namespace std;

#ifdef LOG4CXX
	// provides parent logger for everybody
	log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("yade");
#endif

void nullHandler(int sig){}
void warnOnceHandler(int sig){
	cerr<<"WARN: nullHandler (probably log4cxx error), signal "<<(sig==SIGSEGV?"SEGV":"[other]. Signal will be ignored since now.")<<endl;
	signal(sig,nullHandler);
}

void
sigHandler(int sig){
	switch(sig){
	#ifdef EMBED_PYTHON
		case SIGINT:
			LOG_DEBUG("Finalizing Python...");
			Py_Finalize();
			// http://www.cons.org/cracauer/sigint.html
			signal(SIGINT,SIG_DFL); // reset to default
			kill(getpid(),SIGINT); // kill ourselves, this time without Python
			break;
	#endif
	#ifdef YADE_DEBUG
		case SIGABRT:
		case SIGSEGV:
			signal(SIGSEGV,SIG_DFL); signal(SIGABRT,SIG_DFL); // prevent loops - default handlers
			cerr<<"SIGSEGV/SIGABRT handler called; gdb batch file is `"<<Omega::instance().gdbCrashBatch<<"'"<<endl;
			std::system((string("gdb -x ")+Omega::instance().gdbCrashBatch).c_str());
			unlink(Omega::instance().gdbCrashBatch.c_str()); // delete the crash batch file
			raise(sig); // reemit signal after exiting gdb
			break;
	#endif
	case SIGHUP:
		signal(SIGHUP,SIG_DFL);
		LOG_INFO("Received SIGHUP.");
		if(Omega::instance().getRootBody()){
			LOG_INFO("Attempting emergency save to "<<Omega::instance().recoveryFilename);
			Omega::instance().stopSimulationLoop();
			Omega::instance().joinSimulationLoop();
			Omega::instance().saveSimulation(Omega::instance().recoveryFilename);
		} else LOG_INFO("Nothing to save.");
		LOG_INFO("Emergency exit.");
		exit(1);
	}
}


void firstRunSetup(shared_ptr<Preferences>& pref)
{
	const char* libDirs[]={"extra","gui","lib","pkg-common","pkg-dem","pkg-fem","pkg-snow","pkg-lattice","pkg-mass-spring","pkg-realtime-rigidbody",NULL /* sentinel */};
	string cfgFile=Omega::instance().yadeConfigPath+"/preferences.xml";
	LOG_INFO("Creating default configuration file: "<<cfgFile<<". Please tune by hand if needed.");
	string expLibDir;
	for(int i=0; libDirs[i]!=NULL; i++) {
		expLibDir=string(PREFIX "/lib/yade" SUFFIX "/") + libDirs[i];
		if(!filesystem::exists(expLibDir)) continue; // skip modules that were not built/installed
		LOG_INFO("Adding plugin directory "<<expLibDir<<".");
		pref->dynlibDirectories.push_back(expLibDir);
	}
	LOG_INFO("Setting GUI: QtGUI.");
	pref->defaultGUILibName="QtGUI";
	IOFormatManager::saveToFile("XMLFormatManager",Omega::instance().yadeConfigPath+"/preferences.xml","preferences",pref);
}

string findRecoveryCandidate(filesystem::path dir, string start){
#if BOOST_VERSION > 103400
	if(!filesystem::exists(dir)) return false;
	filesystem::directory_iterator end;
	for(filesystem::directory_iterator I(dir); I!=end; ++I){
		if(filesystem::is_regular(I->status()) && I->path().leaf().find(start)==0 ){
			return (I->path()).string();
		}
	}
#endif
	return "";
}

void printHelp()
{
	cerr << 
"\n" << Omega::instance().yadeVersionName << "\n\
\n\
	-h      : print this help.\n\
	-N name : specify the user interface (NullGUI, PythonUI, QtGUI; not all of them must be compiled)\n\
	-n      : use NullGUI (command line interface) instead of default GUI.\n\
	-w      : write default configuration (automatic at first run)\n\
	-c      : use the current directory ./ as configuration directory\n\
	-x      : don't use gdb\n\
	-C path : configuration directory different from the default ~/.yade-something/\n\
	-S file : load simulation from file (works with QtGUI only)\n\
	-v      : be verbose (may be repeated)\n\
\n\
	--      : pass all remaining options to the selected GUI\n\
";
	cerr <<
	"compilation flags:\n"
		"   PREFIX=" PREFIX  "\n"
	 	"   SUFFIX=" SUFFIX "\n"
	#ifdef YADE_DEBUG
		"   YADE_DEBUG (debug information, crash traces)\n"
	#endif
	#ifdef NDEBUG
		"   NDEBUG (heavy optimizations, no assertions and debugging features)\n"
	#endif
	#ifdef YADE_OPENMP
		"   YADE_OPENMP (supports openMP; set OMP_NUM_THREADS env. var to control parallelism.\n"
	#endif
	#ifdef BEX_CONTAINER
		"   BEX_CONTAINER (uses BexContainer instead of PhysicalActionContainer)\n"
	#endif
	"\n\n";
}


int main(int argc, char *argv[])
{
	/* Omega::init() cannot be called from Omega::Omega (invoked at first instance() call), since init calls resetRootBody,
	 * which locks renderMutex, calls instance() in turn, but since not constructed yet,
	 * instance() → Omega::Omega → init → resetRootBody → lock renderMutex → deadlock */
	Omega::instance().init();

	// This makes boost stop bitching about dot-files and other files that may not exist on MS-DOS 3.3;
	// see http://www.boost.org/libs/filesystem/doc/portability_guide.htm#recommendations for what all they consider bad.
	// Since it is a static variable, it infulences all boost::filesystem operations in this respect (fortunately).
	filesystem::path::default_name_check(filesystem::native);

	string configPath=string(getenv("HOME")) + "/.yade" SUFFIX; // this is the default, may be overridden by -c / -C

	bool useGdb=true;
	
	int ch; string gui=""; string simulationFileName=""; bool setup=false; int verbose=0; bool coreOptions=true; bool explicitUI=false;
	while(coreOptions && (ch=getopt(argc,argv,"hnN:wC:cxvS:"))!=-1)
		switch(ch){
			case 'h': printHelp(); return 1;
			case 'n': gui="NullGUI"; explicitUI=true; break;
			case 'N': gui=optarg; explicitUI=true; break;
			case 'w': setup=true; break;
			case 'C': configPath=optarg; break;
			case 'c': configPath="."; break;
			case 'x': useGdb=false; break;
			case 'v': verbose+=1; break;
			case 'S': simulationFileName=optarg; break;
			case '-': coreOptions=false; break;
			default: printHelp(); return 1;
		}
	// save original options
	Omega::instance().origArgv=argv; Omega::instance().origArgc=argc;
	// kill processed options, keep one more which is in fact non-option (normally the binary)
	argv=&(argv[optind-1]); argc-=optind-1;
	// reset getopt globals for next processing in frontends
	optind=0; opterr=0;

	#ifdef LOG4CXX
		// read logging configuration from file and watch it (creates a separate thread)
		std::string logConf=configPath+"/logging.conf";
		if(filesystem::exists(logConf)){
			log4cxx::PropertyConfigurator::configure(logConf);
			LOG_INFO("Loading "<<logConf);
		} else { // otherwise use simple console-directed logging
			log4cxx::BasicConfigurator::configure();
			logger->setLevel(log4cxx::Level::WARN);
			LOG_INFO("Logger uses basic (console) configuration since `"<<logConf<<"' was not found. INFO and DEBUG messages will be omitted.");
			LOG_INFO("Look at the file doc/logging.conf.sample in the source distribution as an example on how to customize logging.");
		}
	#endif

	Omega::instance().yadeVersionName = "Yet Another Dynamic Engine 0.12.x, beta, SVN snapshot.";
	Omega::instance().preferences    = shared_ptr<Preferences>(new Preferences);
	Omega::instance().yadeConfigPath = configPath; 
	filesystem::path yadeConfigPath  = filesystem::path(Omega::instance().yadeConfigPath, filesystem::native);
	filesystem::path yadeConfigFile  = filesystem::path(Omega::instance().yadeConfigPath + "/preferences.xml", filesystem::native);

	#ifdef LOG4CXX
		if(verbose==1) logger->setLevel(log4cxx::Level::INFO);
		else if (verbose>=2) logger->setLevel(log4cxx::Level::DEBUG);
		if(getenv("YADE_DEBUG")){
			LOG_INFO("YADE_DEBUG environment variable is defined, setting logging level to DEBUG.");
			logger->setLevel(log4cxx::Level::DEBUG);
		}
	#endif


	#ifdef EMBED_PYTHON
		/* see http://www.python.org/dev/peps/pep-0311 for threading with Python embedded */
		LOG_DEBUG("Initializing Python...");
		Py_OptimizeFlag=1;
		Py_Initialize();
		//PyEval_InitThreads(); // this locks the GIL as side-effect
		//PyGILState_STATE pyState=PyGILState_Ensure(); PyGILState_Release(pyState);
		signal(SIGINT,sigHandler);
	#endif

	if (!filesystem::exists(yadeConfigPath) || setup || !filesystem::exists(yadeConfigFile)){
		filesystem::create_directories(yadeConfigPath);
		firstRunSetup(Omega::instance().preferences);
	}

	#ifdef YADE_DEBUG
		if(useGdb){
			// postponed until the config dir has been created
			ofstream gdbBatch;
			Omega::instance().gdbCrashBatch=(yadeConfigPath/"gdb_crash_batch-pid").string()+lexical_cast<string>(getpid());
			gdbBatch.open(Omega::instance().gdbCrashBatch.c_str()); gdbBatch<<"attach "<<lexical_cast<string>(getpid())<<"\nset pagination off\nthread info\nthread apply all backtrace\ndetach\nquit\n"; gdbBatch.close();
			signal(SIGABRT,sigHandler);
			signal(SIGSEGV,sigHandler);
			LOG_DEBUG("ABRT/SEGV signal handlers set, crash batch created as "<<Omega::instance().gdbCrashBatch);
		}
	#endif
	
	LOG_INFO("Loading "<<yadeConfigFile.string()); IOFormatManager::loadFromFile("XMLFormatManager",yadeConfigFile.string(),"preferences",Omega::instance().preferences);

	LOG_INFO("Loading plugins"); Omega::instance().scanPlugins();
	Omega::instance().init();

	Omega::instance().setSimulationFileName(simulationFileName); //init() resets to "";

	// recovery file pattern
	Omega::instance().recoveryFilename=(yadeConfigPath/"recovery-pid").string()+lexical_cast<string>(getpid())+".xml";
	signal(SIGHUP,sigHandler);

	string recoveryCandidate=findRecoveryCandidate(/* directory */ yadeConfigPath, /* beginning of the filename */ "recovery-pid");
	if(!recoveryCandidate.empty()){
		if(!simulationFileName.empty()) LOG_WARN("Skipping recovery of `"<<recoveryCandidate<<"', since the file `"<<simulationFileName<<"' was given on the command-line.")
		else {
			LOG_INFO("Will recover simulation from `"<<recoveryCandidate<<"'.");
			Omega::instance().setSimulationFileName(recoveryCandidate);
		}
	}

	if(gui.size()==0) gui=Omega::instance().preferences->defaultGUILibName;
	if(!explicitUI && gui=="PythonUI" && !getenv("TERM")){ LOG_WARN("No $TERM, using QtGUI instead of PythonUI"); gui="QtGUI"; }
	if(gui=="QtGUI" && !getenv("DISPLAY")){ LOG_WARN("No $DISPLAY, using PythonUI instead of QtUI"); gui="PythonUI"; }
		
	shared_ptr<FrontEnd> frontEnd = dynamic_pointer_cast<FrontEnd>(ClassFactory::instance().createShared(gui));
	
 	// for(int i=0;i<argc; i++)cerr<<"Argument "<<i<<": "<<argv[i]<<endl;
	int ok = frontEnd->run(argc,argv);

	#ifdef EMBED_PYTHON
		/* pyFinalize crashes with boost:python<=1.35
		 * http://www.boost.org/libs/python/todo.html#pyfinalize-safety has explanation 
		 * once this is fixed, you should remove workaround that saves history from ipython session in gui/py/PythonUI_rc.py:63
		 *   import IPython.ipapi
		 *   IPython.ipapi.get().IP.atexit_operations()
		 */
		// LOG_DEBUG("Finalizing Python...");
		// Py_Finalize();
	#endif
	#ifdef YADE_DEBUG
		if(useGdb){
			signal(SIGABRT,SIG_DFL); signal(SIGHUP,SIG_DFL); // default handlers
			signal(SIGSEGV,warnOnceHandler); // FIXME: this is to cover up crash that occurs in log4cxx on i386 sometimes 
			unlink(Omega::instance().gdbCrashBatch.c_str());
		}
	#endif

	LOG_INFO("Yade: normal exit.");
	return ok;
}

