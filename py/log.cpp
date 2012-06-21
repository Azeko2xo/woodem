#include<boost/python.hpp>
#include<string>
#include<woo/lib/base/Logging.hpp>
#include<woo/lib/base/Types.hpp>
#include<woo/lib/pyutil/doc_opts.hpp>
enum{ll_TRACE,ll_DEBUG,ll_INFO,ll_WARN,ll_ERROR,ll_FATAL};

#ifdef WOO_LOG4CXX

	log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("yade.log");

	#include<log4cxx/logmanager.h>

	void logSetLevel(std::string loggerName,int level){
		std::string fullName(loggerName.empty()?"yade":("yade."+loggerName));
		if(!log4cxx::LogManager::exists(fullName)){
			LOG_WARN("No logger named "<<loggerName<<", ignoring level setting.");			
			// throw std::invalid_argument("No logger named `"+fullName+"'");
		} 
		log4cxx::LevelPtr l;
		switch(level){
			#ifdef LOG4CXX_TRACE
				case ll_TRACE: l=log4cxx::Level::getTrace(); break;
				case ll_DEBUG: l=log4cxx::Level::getDebug(); break;
				case ll_INFO:  l=log4cxx::Level::getInfo(); break;
				case ll_WARN:  l=log4cxx::Level::getWarn(); break;
				case ll_ERROR: l=log4cxx::Level::getError(); break;
				case ll_FATAL: l=log4cxx::Level::getFatal(); break;
			#else
				case ll_TRACE: l=log4cxx::Level::DEBUG; break;
				case ll_DEBUG: l=log4cxx::Level::DEBUG; break;
				case ll_INFO:  l=log4cxx::Level::INFO; break;
				case ll_WARN:  l=log4cxx::Level::WARN; break;
				case ll_ERROR: l=log4cxx::Level::ERROR; break;
				case ll_FATAL: l=log4cxx::Level::FATAL; break;
			#endif
			default: throw std::invalid_argument("Unrecognized logging level "+lexical_cast<std::string>(level));
		}
		log4cxx::LogManager::getLogger("yade."+loggerName)->setLevel(l);
	}
	void logLoadConfig(std::string f){ log4cxx::PropertyConfigurator::configure(f); }
#else
	bool warnedOnce=false;
	void logSetLevel(std::string loggerName, int level){
		// better somehow python's raise RuntimeWarning, but not sure how to do that from c++
		// it shouldn't be trapped by boost::python's exception translator, just print warning
		// Do like this for now.
		if(warnedOnce) return;
		LOG_WARN("Yade was compiled without log4cxx support. Setting log levels from python will have no effect (warn once).");
		warnedOnce=true;
	}
	void logLoadConfig(std::string f){
		if(warnedOnce) return;
		LOG_WARN("Yade was compiled without log4cxx support. Loading log file will have no effect (warn once).");
		warnedOnce=true;
	}
#endif

BOOST_PYTHON_MODULE(log){
	py::scope().attr("__doc__") = "Access and manipulation of log4cxx loggers.";

	WOO_SET_DOCSTRING_OPTS;

	py::def("setLevel",logSetLevel,(py::arg("logger"),py::arg("level")),"Set minimum severity *level* (constants ``TRACE``, ``DEBUG``, ``INFO``, ``WARN``, ``ERROR``, ``FATAL``) for given logger. \nLeading 'yade.' will be appended automatically to the logger name; if logger is '', the root logger 'yade' will be operated on.");
	py::def("loadConfig",logLoadConfig,(py::arg("fileName")),"Load configuration from file (log4cxx::PropertyConfigurator::configure)");
	py::scope().attr("TRACE")=(int)ll_TRACE;
	py::scope().attr("DEBUG")=(int)ll_DEBUG;
	py::scope().attr("INFO")= (int)ll_INFO;
	py::scope().attr("WARN")= (int)ll_WARN;
	py::scope().attr("ERROR")=(int)ll_ERROR;
	py::scope().attr("FATAL")=(int)ll_FATAL;
}
