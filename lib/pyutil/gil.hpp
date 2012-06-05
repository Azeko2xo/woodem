// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<Python.h>
#include<string>
//! class (scoped lock) managing python's Global Interpreter Lock (gil)
class GilLock{
	PyGILState_STATE state;
	public:
		GilLock(){ state=PyGILState_Ensure(); }
		~GilLock(){ PyGILState_Release(state); }
};
//! run string as python command; locks & unlocks GIL automatically
void pyRunString(const std::string& cmd);

