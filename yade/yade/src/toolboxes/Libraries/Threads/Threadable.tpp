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
Threadable<Thread>::Threadable(shared_ptr<ThreadSynchronizer> s) : turn(new int(0)), saveTurn(new int(0))
{
	synchronizer = s;
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
void Threadable<Thread>::stop()
{
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

//	*saveTurn = *turn;
//	*turn = -2;
//	synchronizer->signal();
	synchronizer->removeThread(*turn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::start()
{
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

	*turn = *saveTurn;
	*turn = synchronizer->insertThread();

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::operator()()
{

	if (synchronizer)
	{
		while (notEnd())
		{
			{
				boost::mutex::scoped_lock lock(*(synchronizer->getMutex()));

				while (synchronizer->notMyTurn(*turn))
					synchronizer->wait(lock);

				oneLoop();

				synchronizer->setNextCurrentThread();

				synchronizer->signal();
			}
		}
		synchronizer->removeThread(*turn);
	}
	else
	{
		while (notEnd())
			oneLoop();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
int Threadable<Thread>::getTurn()
{
	return *turn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::createThread(bool autoStart,shared_ptr<ThreadSynchronizer> s)
{
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

	synchronizer = s;
	if (synchronizer)
	{
		*turn = synchronizer->insertThread();
		if (!autoStart)
			stop();
	}
	thread = shared_ptr<boost::thread>(new boost::thread(*(dynamic_cast<Thread*>(this))));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Thread>
void Threadable<Thread>::sleep(int ms)
{
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

	boost::xtime xt;
	boost::xtime_get(&xt, boost::TIME_UTC);
	xt.nsec += ms*1000000;

	boost::thread::sleep(xt);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
