#pragma once

#include<boost/date_time/posix_time/posix_time.hpp>
#include<fstream>
#include<set>
#include<list>
#include<time.h>
#include<boost/thread/thread.hpp>
#include<iostream>
#include<boost/python.hpp>
#include<boost/foreach.hpp>

#include<yade/lib/base/Math.hpp>
#include<yade/lib/object/ClassFactory.hpp>
#include<yade/lib/base/Singleton.hpp>
#include<yade/lib/base/Types.hpp>

#ifndef FOREACH
# define FOREACH BOOST_FOREACH
#endif

class Scene;
namespace yade { class Object; };

namespace py=boost::python;

struct DynlibDescriptor{ set<string> baseClasses; };

class Omega: public Singleton<Omega>{

	map<string,DynlibDescriptor> dynlibs; // FIXME: should store that in ClassFactory ?
	void initializePlugins(const vector<std::pair<std::string, std::string> >& dynlibsList); 
	
	shared_ptr<Scene> scene;
	shared_ptr<Scene> sceneAnother; // used for temporarily running different simulation, in Omega().switchscene()

	boost::posix_time::ptime startupLocalTime;

	map<string,string> memSavedSimulations;

	// to avoid accessing simulation when it is being loaded (should avoid crashes with the UI)
	boost::mutex loadingSimulationMutex;
	boost::mutex tmpFileCounterMutex;
	long tmpFileCounter;
	std::string tmpFileDir;

	public:
		// void reset();
		void cleanupTemps();
		const map<string,DynlibDescriptor>& getDynlibsDescriptor();
		void loadPlugins(vector<string> pluginFiles);

		bool isInheritingFrom(const string& className, const string& baseClassName );
		bool isInheritingFrom_recursive(const string& className, const string& baseClassName );

		string gdbCrashBatch;

		// do not change by hand
		/* Mutex for:
		* 1. GLViewer::paintGL (deffered lock: if fails, no GL painting is done)
		* 2. other threads that wish to manipulate GL
		* 3. Omega when substantial changes to the scene are being made (bodies being deleted, simulation loaded etc) so that GL doesn't access those and crash */
		boost::try_mutex renderMutex;


		shared_ptr<Object> loadTmp(const string& name);
		void saveTmp(shared_ptr<Object> s, const string& name, bool quiet=false);


		const shared_ptr<Scene>& getScene();
		void setScene(const shared_ptr<Scene>& s);
		//! Return unique temporary filename. May be deleted by the user; if not, will be deleted at shutdown.
		string tmpFilename();
		Real getRealTime();
		boost::posix_time::time_duration getRealTime_duration();

		// configuration directory used for logging config and possibly other things
		std::string confDir;
		// default OpenCL device passed from the command-line
		Vector2i defaultClDev;

	DECLARE_LOGGER;

	Omega();

	FRIEND_SINGLETON(Omega);
	friend class pyOmega;
};


/*! Function returning class name (as string) for given index and topIndexable (top-level indexable, such as Shape, Material and so on)
This function exists solely for debugging, is quite slow: it has to traverse all classes and ask for inheritance information.
It should be used primarily to convert indices to names in Dispatcher::dictDispatchMatrix?D; since it relies on Omega for RTTI,
this code could not be in Dispatcher itself.
s*/
template<class topIndexable>
std::string Dispatcher_indexToClassName(int idx){
	boost::scoped_ptr<topIndexable> top(new topIndexable);
	std::string topName=top->getClassName();
	typedef std::pair<string,DynlibDescriptor> classItemType;
	FOREACH(classItemType clss, Omega::instance().getDynlibsDescriptor()){
		if(Omega::instance().isInheritingFrom_recursive(clss.first,topName) || clss.first==topName){
			// create instance, to ask for index
			shared_ptr<topIndexable> inst=dynamic_pointer_cast<topIndexable>(ClassFactory::instance().createShared(clss.first));
			assert(inst);
			if(inst->getClassIndex()<0 && inst->getClassName()!=top->getClassName()){
				throw logic_error("Class "+inst->getClassName()+" didn't use REGISTER_CLASS_INDEX("+inst->getClassName()+","+top->getClassName()+") and/or forgot to call createIndex() in the ctor. [[ Please fix that! ]]");
			}
			if(inst->getClassIndex()==idx) return clss.first;
		}
	}
	throw std::runtime_error("No class with index "+boost::lexical_cast<string>(idx)+" found (top-level indexable is "+topName+")");
}


//! Return class index of given indexable
template<typename TopIndexable>
int Indexable_getClassIndex(const shared_ptr<TopIndexable> i){return i->getClassIndex();}

//! Return sequence (hierarchy) of class indices of given indexable; optionally convert to names
template<typename TopIndexable>
py::list Indexable_getClassIndices(const shared_ptr<TopIndexable> i, bool convertToNames){
	int depth=1; py::list ret; int idx0=i->getClassIndex();
	if(convertToNames) ret.append(Dispatcher_indexToClassName<TopIndexable>(idx0));
	else ret.append(idx0);
	if(idx0<0) return ret; // don't continue and call getBaseClassIndex(), since we are at the top already
	while(true){
		int idx=i->getBaseClassIndex(depth++);
		if(convertToNames) ret.append(Dispatcher_indexToClassName<TopIndexable>(idx));
		else ret.append(idx);
		if(idx<0) return ret;
	}
}




