/***************************************************************************
 *   Copyright (C) 2004 by Olivier Galizzi                                 *
 *   olivier.galizzi@imag.fr                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/thread/xtime.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ThreadSynchronizer.hpp"
#include "ThreadSafe.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
	
template<class Thread>
Threadable<Thread>::Threadable(shared_ptr<ThreadSynchronizer> s) :	mutex(new boost::mutex),
									finished(new bool(false)), 
									blocked(new bool(true)), 
									turn(new int(0)),
									synchronizer(s)
{
	assert(s!=0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
Threadable<Thread>::~Threadable()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::createThread()
{	
	synchronizer->insertThread(turn);
	thread = shared_ptr<boost::thread>(new boost::thread(*(dynamic_cast<Thread*>(this))));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::stop()
{
	boost::mutex::scoped_lock lock(*mutex);
	*blocked = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::start()
{
	boost::mutex::scoped_lock lock(*mutex);
	*blocked = false;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::finish()
{
	boost::mutex::scoped_lock lock(*mutex);
	*finished = true;
	*blocked = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::join()
{
	//boost::mutex::scoped_lock lock(*mutex);
	thread->join();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::operator()()
{
	ThreadSafe::cout("beginning Of thread number : "+lexical_cast<string>(*turn));
	while (notEnd() && !*finished)
	{
		{	
		//	ThreadSafe::cout(lexical_cast<string>(*turn)+" : 1");
			boost::mutex::scoped_lock lock(synchronizer->getMutex());
		//	ThreadSafe::cout(lexical_cast<string>(*turn)+" : 2");
			while (synchronizer->notMyTurn(*turn))
				synchronizer->wait(lock);
		//	ThreadSafe::cout(lexical_cast<string>(*turn)+" : 3");
			if (!*blocked)
				oneLoop();
		//	ThreadSafe::cout(lexical_cast<string>(*turn)+" : 4");
			synchronizer->setNextCurrentThread();
		//	ThreadSafe::cout(lexical_cast<string>(*turn)+" : 5");
			synchronizer->signal();
		//	ThreadSafe::cout(lexical_cast<string>(*turn)+" : 6");
		}
	}
	synchronizer->removeThread(*turn);
	ThreadSafe::cout("Ending Of thread number : "+lexical_cast<string>(*turn));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::sleep(int ms)
{
	boost::mutex::scoped_lock lock(*mutex);
	boost::xtime xt;
	boost::xtime_get(&xt, boost::TIME_UTC);
	xt.nsec += ms*1000000;

	boost::thread::sleep(xt);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
