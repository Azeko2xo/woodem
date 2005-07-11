/***************************************************************************
 *   Copyright (C) 2005 by Olivier Galizzi                                 *
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "MyBodyContainer.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

MyBodyContainer::MyBodyContainer() : BodyContainer()
{
	vec.push_back(1);
	vec.push_back(2);	
	vec.push_back(3);
	vec.push_back(4);
	vec.push_back(5);			
}

MyBodyContainer::~MyBodyContainer()
{
}

BodyIteratorSharedPtr<BodyIterator> MyBodyContainer::begin()
{
	BodyContainer::iterator spi(new MyIterator);
	MyIterator* it = static_cast<MyIterator*>(spi.get());
	it->vi = vec.begin();
	return spi;
}

BodyIteratorSharedPtr<BodyIterator> MyBodyContainer::end()
{
	BodyContainer::iterator spi(new MyIterator);
	MyIterator* it = static_cast<MyIterator*>(spi.get());
	it->vi = vec.end();
	return spi;
}

BodyIteratorSharedPtr<BodyIterator> MyBodyContainer::emptyIterator() 
{
	return BodyContainer::iterator(new MyIterator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

MyIterator::MyIterator() : BodyIterator()
{

}

MyIterator::~MyIterator()
{

}

bool MyIterator::operator!=(const BodyIterator& i)
{
	return (vi != static_cast<const MyIterator&>(i).vi );
}

void MyIterator::operator++()
{
	++vi;
}

void MyIterator::operator++(int)
{
	vi++;
}

void MyIterator::operator=(const BodyIterator& i)
{
	vi = static_cast<const MyIterator&>(i).vi;
}

int MyIterator::getCurrent()
{
	return *vi;
}
