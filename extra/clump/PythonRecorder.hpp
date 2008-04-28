// (c) 2007 Vaclav Smilauer <eudoxos@arcig.cz> 

#ifndef PYTHON_RECORDER_HPP
#define PYTHON_RECORDER_HPP

#ifndef EMBED_PYTHON
	#error EMBED_PYTHON must be defined for this module to work!
#endif

#include<Python.h>
#include<string>
#include<yade/core/DataRecorder.hpp>
#include<yade/lib-base/Logging.hpp>

/*! Recorder that executes arbitrary python expression when called.
 *
 * The expressions to access yade's internals are described in documentation for pyade.py and pyade.cpp.
 *
 * */
class PythonRecorder: public DataRecorder
{
	private:
		//! simple call to 
		PyObject *compiledRunExprCall;
		//! dictionary of globals, for passing to PyEval_EvalCode
		PyObject *mainDict;
		//! runExpr, but string-transformed to python function definition
		std::string runExprAsDef;
	public :
		//! Constructor that imports pyade module (warns if there is an error).
		PythonRecorder();
		virtual void action(MetaBody*);
		virtual bool isActivated(){return true;}
		virtual void registerAttributes(){DataRecorder::registerAttributes(); REGISTER_ATTRIBUTE(runExpr); REGISTER_ATTRIBUTE(initExpr); REGISTER_ATTRIBUTE(outputFile);}
		//! This expression will be interpreted when the engine is called (every iteration)
		std::string runExpr;
		//! Piece of python code run on intialization (deserialization)
		std::string initExpr;
		//! If not empty, python sys.stdout will be redirected to this file (overwritten every time!)
		std::string outputFile;
	protected :
		virtual void postProcessAttributes(bool deserializing);
	DECLARE_LOGGER;

	REGISTER_CLASS_NAME(PythonRecorder);
	REGISTER_BASE_CLASS_NAME(DataRecorder);

};

REGISTER_SERIALIZABLE(PythonRecorder,false);

#endif // PYTHON_RECORDER_HPP

