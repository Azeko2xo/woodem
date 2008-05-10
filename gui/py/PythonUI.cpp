//#include<Python.h>
#include<boost/thread/thread.hpp>
#include<boost/python.hpp>
#include<boost/algorithm/string.hpp>
#include<errno.h>

#include<stdlib.h>

#include"PythonUI.hpp"

#include <X11/Xlib.h>

using namespace boost;

struct termios PythonUI::tios, PythonUI::tios_orig;

CREATE_LOGGER(PythonUI);

void PythonUI::help(){
	cerr<<" PythonUI (python console) frontend.\n\
\n\
	-h       print this help\n\
	-s file  run this python script before entering interactive prompt\n\
	/* -c cmd   run python command (may be specified multiple times, newline is appended every time) */ \n\
\n\
	Sample session:\n\
\n\
	>>> p=Preprocessor\n\
	>>> p.generator='Funnel'\n\
	>>> p['density']=2000\n\
	>>> p.output='/tmp/funnel.xml'\n\
	>>> p.generate()\n\
	>>> o=Omega()\n\
	>>> o.run()\n\
	>>> # ^D to exit\n\
";
}

void PythonUI::execScript(string script){
	LOG_DEBUG("Python will now run file `"<<script<<"'.");
	FILE* scriptFILE=fopen(script.c_str(),"r");
	if(scriptFILE){
		PyRun_SimpleFile(scriptFILE,script.c_str());
	}
	else{
		string strerr(strerror(errno));
		LOG_ERROR("Unable to open file `"<<script<<"': "<<strerr<<".");
	}
}

void PythonUI::termRestore(void){
	LOG_DEBUG("Restoring terminal discipline.");
	tcsetattr(STDIN_FILENO,TCSANOW,&PythonUI::tios_orig);
}

int PythonUI::run(int argc, char *argv[]) {
	string runScript;
	string runCommands;
	bool stopAfter=false;
	
	int ch;
	while((ch=getopt(argc,argv,"hs:"))!=-1)
	switch(ch){
		case 'h': help(); return 1;
		case 's': runScript=string(optarg); break;
		case 'x': stopAfter=true; break;
		//case 'c': runCommands+=string(optarg)+"\n"; break;
		default:
			LOG_ERROR("Unhandled option string: `"<<string(optarg)<<"' (try -h for help on options)");
			break;
	}
	if(optind<argc){ // process non-option arguments
		if(boost::algorithm::ends_with(string(argv[optind]),string(".py"))) runScript=string(argv[optind]);
		else if(boost::algorithm::ends_with(string(argv[optind]),string(".xml"))) Omega::instance().setSimulationFileName(string(argv[optind]));
		else optind--;
	}
	for (int index = optind+1; index<argc; index++) LOG_ERROR("Unprocessed non-option argument: `"<<argv[index]<<"'");

	/* In threaded ipython, receiving SIGINT from ^C leads to segfault (for reasons I don't know).
	 * Hence we remap ^C (keycode in c_cc[VINTR]) to killing the line (c_cc[VKILL]) and disable VINTR afterwards.
	 * The behavior is restored back by the PythonUI::termRestore registered with atexit.
	 * */
	LOG_DEBUG("Setting terminal discipline (^C kills line, ^U does nothing)");
	tcgetattr(STDIN_FILENO,&PythonUI::tios);
	memcpy(&PythonUI::tios_orig,&PythonUI::tios,sizeof(struct termios));
	atexit(PythonUI::termRestore);
	tios.c_cc[VKILL]=tios.c_cc[VINTR]; // assign ^C what ^U normally does (delete line)
	tios.c_cc[VINTR] = 0; // disable sending SIGINT at ^C
	tcsetattr(STDIN_FILENO,TCSANOW,&PythonUI::tios);
	tcflush(STDIN_FILENO,TCIFLUSH);


	XInitThreads();
	PyEval_InitThreads();

	PyGILState_STATE pyState = PyGILState_Ensure();

		/* import yade (for startUI()) and yade.runtime (initially empty) namespaces */
		PyRun_SimpleString("import sys; sys.path.insert(0,'" PREFIX "/lib/yade" SUFFIX "/gui')");
		PyRun_SimpleString("import yade");

		#define PYTHON_DEFINE_STRING(pyName,cxxName) PyRun_SimpleString((string("yade.runtime." pyName "='")+cxxName+"'").c_str())
		#define PYTHON_DEFINE_BOOL(pyName,cxxName) PyRun_SimpleString((string("yade.runtime." pyName "=")+(cxxName?"True":"False")).c_str())
			// wrap those in python::handle<> ??
			PYTHON_DEFINE_STRING("prefix",PREFIX);
			PYTHON_DEFINE_STRING("suffix",SUFFIX);
			PYTHON_DEFINE_STRING("executable",Omega::instance().origArgv[0]);
			PYTHON_DEFINE_STRING("simulation",Omega::instance().getSimulationFileName());
			PYTHON_DEFINE_STRING("script",runScript);
			PYTHON_DEFINE_STRING("commands",runCommands);
			PYTHON_DEFINE_BOOL("stopAfter",stopAfter);
		#undef PYTHON_DEFINE_STRING
		#undef PYTHON_DEFINE_BOOL
		execScript(PREFIX "/lib/yade" SUFFIX "/gui/PythonUI_rc.py");

	PyGILState_Release(pyState);

	tcsetattr(STDIN_FILENO,TCSANOW,&tios_orig);

	//boost::thread cmdlineThread(&cmdlineThreadStart);
	//cmdlineThread.join();

	return 0;
}

