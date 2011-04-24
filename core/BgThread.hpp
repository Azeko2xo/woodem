/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/any.hpp>

class ThreadRunner;

/*! 
\brief	ThreadWorker contains information about tasks to be performed when
	the separate thread is executed.
 */

class ThreadWorker{
	private:
		/// You should check out ThreadRunner, it is used for execution control of this class
		friend class ThreadRunner;
		bool		m_should_terminate;
		bool		m_done;
		boost::mutex	m_mutex;
		boost::any	m_val;
		float		m_progress;
		std::string	m_status;
		void		callSingleAction();

	protected:
		void		setTerminate(bool);
		/// singleAction() can check whether someone asked for termination, and terminate if/when possible
		bool		shouldTerminate();
		/// if something must be returned, set the result using this method
		void		setReturnValue(boost::any);
		/// if you feel monitored for progress, you can set it here: a value between 0.0 and 1.0
		void		setProgress(float);
		/// if you feel being monitored for what currently is done, set the message here
		void		setStatus(std::string);
		/// derived classes must define this method, that's what is executed in separate thread
		virtual void	singleAction() = 0;

	public:
		ThreadWorker() : m_should_terminate(false), m_done(false), m_progress(0) {};
		virtual		~ThreadWorker() {};

		/// Returns a value between 0.0 and 1.0. Useful for updating a progress bar.
		float		progress(); // get_progress ? (pick a naming convention, efngh)
		/// You can display a message in GUI about what is the current work status
		std::string	getStatus();
		/// Check whether execution is finished,
		bool		done();
		/// then get the result.
		boost::any	getReturnValue();
};

/*! 
\brief	ThreadRunner takes care of starting/stopping (executing) the
	ThreadWorker in the separate thread. 

	It is achieved by either:
	- one execution of { ThreadWorker::singleAction(); } in separate thread
	- a loop { while(looping() ) ThreadWorker::singleAction(); } in separate thread

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

class ThreadRunner
{
	private :
		ThreadWorker*	m_thread_worker;
		bool		m_looping;
		boost::mutex	m_boolmutex;
		boost::mutex	m_callmutex;
		boost::mutex	m_runmutex;
		void		run();
		void		call();

		DECLARE_LOGGER;

	public :
		ThreadRunner(ThreadWorker* c) : m_thread_worker(c), m_looping(false), workerThrew(false) {};
		~ThreadRunner();

		/// perform ThreadWorker::singleAction() in separate thread
		void spawnSingleAction();
		/// start doing singleAction() in a loop in separate thread
		void start();
		/// stop the loop (changes the flag checked by looping() )
		void stop();
		/// kindly ask the separate thread to terminate
		void pleaseTerminate();
		/// precondition for the loop started with start().
		bool looping();
		//! if true, workerException is copy of the exception thrown by the worker
		bool workerThrew;
		//! last exception thrown by the worker, if any
		std::exception workerException;
};

// FIXME ; bad name
class SimulationFlow: public ThreadWorker{
	public: virtual void singleAction();
	DECLARE_LOGGER;
};




