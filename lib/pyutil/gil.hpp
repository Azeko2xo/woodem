// 2009 © Václav Šmilauer <eudoxos@arcig.cz>

#include<Python.h>
//! class (scoped lock) managing python's Global Interpreter Lock (gil)
class gilLock{
	PyGILState_STATE state;
	public:
		gilLock(){ state=PyGILState_Ensure(); }
		~gilLock(){ PyGILState_Release(state); }
};

//! run string as python command; locks & unlocks GIL automatically
void pyRunString(const std::string& cmd){
	gilLock lock; PyRun_SimpleString(cmd.c_str());
};
