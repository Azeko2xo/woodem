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

#include<boost/python.hpp>
#include<boost/filesystem/convenience.hpp>


#ifdef WOO_LOG4CXX
	#include<log4cxx/consoleappender.h>
	#include<log4cxx/patternlayout.h>
	static log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("woo.boot");
	/* Initialize log4dcxx automatically when the library is loaded. */
	__attribute__((constructor)) void initLog4cxx() {
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

#ifdef WOO_DEBUG
	void crashHandler(int sig){
	switch(sig){
		case SIGABRT:
		case SIGSEGV:
			signal(SIGSEGV,SIG_DFL); signal(SIGABRT,SIG_DFL); // prevent loops - default handlers
			cerr<<"SIGSEGV/SIGABRT handler called; gdb batch file is `"<<Master::instance().gdbCrashBatch<<"'"<<endl;
			std::system((string("gdb -x ")+Master::instance().gdbCrashBatch).c_str());
			raise(sig); // reemit signal after exiting gdb
			break;
		}
	}		
#endif

/* Initialize woo, loading given plugins */
void wooInitialize(const std::string& confDir){

	PyEval_InitThreads();

	Master& master(Master::instance());
	master.confDir=confDir;
	// master.initTemps();
	#ifdef WOO_DEBUG
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
	//vector<string> ppp; for(int i=0; i<py::len(pp); i++) ppp.push_back(py::extract<string>(pp[i]));

	// register all python classes here
	master.pyRegisterAllClasses();
}
void wooFinalize(){ Master::instance().cleanupTemps(); }

WOO_PYTHON_MODULE(_cxxInternal);
BOOST_PYTHON_MODULE(_cxxInternal){
	py::scope().attr("initialize")=&wooInitialize;
	py::scope().attr("finalize")=&wooFinalize; //,"Finalize woo (only to be used internally).")
}
