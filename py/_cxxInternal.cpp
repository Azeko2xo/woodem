#include<woo/core/Master.hpp>
#include<woo/core/Timing.hpp>
#include<woo/lib/object/Object.hpp>
#include<woo/lib/base/Logging.hpp>

#include<signal.h>
#include<cstdlib>
#include<cstdio>
#include<iostream>
#include<string>
#include<stdexcept>

#include<boost/filesystem/convenience.hpp>
#include<boost/preprocessor/cat.hpp>
#include<boost/preprocessor/stringize.hpp>


#ifdef WOO_LOG4CXX
	#include<log4cxx/consoleappender.h>
	#include<log4cxx/patternlayout.h>
	static log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("woo.boot");
	/* Initialize log4dcxx automatically when the library is loaded. */
	WOO__ATTRIBUTE__CONSTRUCTOR void initLog4cxx() {
		#ifdef LOG4CXX_TRACE
			log4cxx::LevelPtr debugLevel=log4cxx::Level::getDebug(), infoLevel=log4cxx::Level::getInfo(), warnLevel=log4cxx::Level::getWarn();
			// LOG4CXX_STR: http://old.nabble.com/Link-error-when-using-Layout-on-MS-Windows-td20906802.html
			log4cxx::LayoutPtr layout(new log4cxx::PatternLayout(LOG4CXX_STR("%-5r %-5p %-10c %m%n")));
			log4cxx::AppenderPtr appender(new log4cxx::ConsoleAppender(layout));
			log4cxx::LoggerPtr localLogger=log4cxx::Logger::getLogger("woo");
			localLogger->addAppender(appender);
		#else // log4cxx 0.9
			log4cxx::LevelPtr debugLevel=log4cxx::Level::DEBUG, infoLevel=log4cxx::Level::INFO, warnLevel=log4cxx::Level::WARN;
			log4cxx::BasicConfigurator::configure();
			log4cxx::LoggerPtr localLogger=log4cxx::Logger::getLogger("woo");
		#endif
		localLogger->setLevel(getenv("WOO_DEBUG")?debugLevel:warnLevel);
		LOG4CXX_DEBUG(localLogger,"Log4cxx initialized.");
	}
#endif

#if defined(WOO_DEBUG) && !defined(__MINGW64__)
	void crashHandler(int sig){
	switch(sig){
		case SIGABRT:
		case SIGSEGV:
			signal(SIGSEGV,SIG_DFL); signal(SIGABRT,SIG_DFL); // prevent loops - default handlers
			cerr<<"SIGSEGV/SIGABRT handler called; gdb batch file is `"<<Master::instance().gdbCrashBatch<<"'"<<endl;
			int ret=std::system((string("gdb -x ")+Master::instance().gdbCrashBatch).c_str());
			if(ret) cerr<<"Running the debugger failed (exit status "<<ret<<"); do you have gdb?"<<endl;
			raise(sig); // reemit signal after exiting gdb
			break;
		}
	}		
#endif

/* Initialize woo - load config files, register python classes, set signal handlers */
void wooInitialize(){

	PyEval_InitThreads();

	Master& master(Master::instance());
	
	string confDir;
	if(getenv("XDG_CONFIG_HOME")){
		confDir=getenv("XDG_CONFIG_HOME");
	} else {
		#ifndef __MINGW64__ // POSIX
			if(getenv("HOME")) confDir=string(getenv("HOME"))+"/.config";
		#else
			// windows stuff; not tested
			// http://stackoverflow.com/a/4891126/761090
			if(getenv("USERPROFILE")) confDir=getenv("USERPROFILE");
			else if(getenv("HOMEDRIVE") && getenv("HOMEPATH")) confDir=string(getenv("HOMEDRIVE"))+string(getenv("HOMEPATH"));
		#endif
		else LOG_WARN("Unable to determine home directory; no user-configuration will be loaded.");
	}

	confDir+="/woo";


	master.confDir=confDir;
	#if defined(WOO_DEBUG) && !defined(__MINGW64__)
		std::ofstream gdbBatch;
		master.gdbCrashBatch=master.tmpFilename();
		gdbBatch.open(master.gdbCrashBatch.c_str()); gdbBatch<<"attach "<<lexical_cast<string>(getpid())<<"\nset pagination off\nthread info\nthread apply all backtrace\ndetach\nquit\n"; gdbBatch.close();
		signal(SIGABRT,crashHandler);
		signal(SIGSEGV,crashHandler);
	#endif
	#ifdef WOO_LOG4CXX
		// read logging configuration from file and watch it (creates a separate thread)
		if(boost::filesystem::exists(confDir+"/logging.conf")){
			std::string logConf=confDir+"/logging.conf";
			log4cxx::PropertyConfigurator::configure(logConf);
			LOG_INFO("Loaded "<<logConf);
		}
	#endif

	// check that the decimal separator is "." (for GTS imports)
	if(atof("0.5")==0.0){
		LOG_WARN("Decimal separator is not '.'; this can cause erratic mesh imports from GTS and perhaps other problems. Please report this to http://bugs.launchpad.net/woo .");
	}

	// register all python classes here
	master.pyRegisterAllClasses();
}

#ifdef WOO_GTS
	// this module is compiled from separate sources (in py/3rd-party/pygts)
	// but we will register it here
	WOO_PYTHON_MODULE(_gts);
#endif

// NB: this module does NOT use WOO_PYTHON_MODULE, since the file is really called _cxxInternal[_flavor][_debug].so
// and is a real real python module
// 
#ifdef WOO_DEBUG
	BOOST_PYTHON_MODULE(BOOST_PP_CAT(BOOST_PP_CAT(_cxxInternal,WOO_CXX_FLAVOR),_debug))
#else
	BOOST_PYTHON_MODULE(BOOST_PP_CAT(_cxxInternal,WOO_CXX_FLAVOR))
#endif
{
	LOG_DEBUG_EARLY("Initializing the _cxxInternal" BOOST_PP_STRINGIZE(WOO_CXX_FLAVOR) " module.");
	py::scope().attr("__doc__")="This module's binary contains all compiled Woo modules (such as :obj:`woo.core`), which are created dynamically when this module is imported for the first time. In itself, it is empty and only to be used internally.";
	// call automatically at module import time
	wooInitialize();
}
