/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef THREAD_RUNNER_HPP
#define THREAD_RUNNER_HPP

#include <boost/thread/mutex.hpp>

/*! 
\brief	ThreadRunner takes care of starting/stopping (executing) the
	ThreadWorker in the separate thread. 

	It is achieved by either:
	- one execution of { ThreadWorker::singleAction(); } in separate thread
	- a loop { while(isRunning() ) ThreadWorker::singleAction(); } in separate thread

	Lifetime of ThreadRunner is guaranteed to be longer or equal to
	the lifetime of	the separate thread of execution.

	The ThreadRunner owner must make sure that ThreadWorker has longer or
	equal lifetime than instance of ThreadRunner. Otherwise ThreadRunner
	will try to execute a dead object, which will lead to crash.

	Do not destroy immediately after call to singleAction(). Destructor can
	kick in before a separate thread starts, which will lead to a crash.

	User can explicitly ask the running thread to terminate execution. If
	the thread supports it, it will terminate.

\note	This code is reentrant. Simultaneous requests from other threads to
	start/stop or perform singleAction() are expected.
	   
	So ThreadWorker(s) are running, while the user is interacting with the
	UI frontend (doesn't matter whether the UI is graphical, ncurses or
	any other).

 */

class ThreadWorker;

class ThreadRunner
{
	private :
		ThreadWorker*	m_thread_worker;
		bool		running_;
		boost::mutex	boolmutex_;
		boost::mutex	callmutex_;
		boost::mutex	runmutex_;
		void		run();
		void		call();

	public :
		ThreadRunner(ThreadWorker* c) : m_thread_worker(c), running_(false) {};
		~ThreadRunner();

		void spawnSingleAction();
		void start();
		void stop();
		void pleaseTerminate();
		bool isRunning();
};

#endif // SIMULATIONLOOP_HPP

