#include "InteractionHashMap.hpp"

InteractionHashMap::InteractionHashMap()
{
	clear();
}

InteractionHashMap::~InteractionHashMap()
{
}

bool InteractionHashMap::insert(shared_ptr<Interaction>& i)
{
	unsigned int id1 = i->getId1();
	unsigned int id2 = i->getId2();
	if (id1>id2)
		swap(id1,id2);

	return volatileInteractions.insert( IHashMap::value_type( pair<unsigned int,unsigned int>(id1,id2) , i )).second;
}

bool InteractionHashMap::insert(unsigned int id1,unsigned int id2)
{
	shared_ptr<Interaction> i(new Interaction(id1,id2) );
	return insert(i);	
};

void InteractionHashMap::clear()
{
	volatileInteractions.clear();
}

bool InteractionHashMap::erase(unsigned int id1,unsigned int id2)
{
	if (id1>id2)
		swap(id1,id2);

	unsigned int oldSize = volatileInteractions.size();
	pair<unsigned int,unsigned int> p(id1,id2);
	unsigned int size = volatileInteractions.erase(p);

	return size!=oldSize;

}

const shared_ptr<Interaction>& InteractionHashMap::find(unsigned int id1,unsigned int id2)
{
	if (id1>id2)
		swap(id1,id2);

	hmii = volatileInteractions.find(pair<unsigned int,unsigned int>(id1,id2));
	if (hmii!=volatileInteractions.end())
		return (*hmii).second;
	else
	{
		empty = shared_ptr<Interaction>(); 
		return empty;
	}
}

void InteractionHashMap::gotoFirstPotential()
{
	hmii    = volatileInteractions.begin();
	hmiiEnd = volatileInteractions.end();
}

bool InteractionHashMap::notAtEndPotential()
{
	return ( hmii != hmiiEnd );
}

void InteractionHashMap::gotoNextPotential()
{
	++hmii;
}

void InteractionHashMap::gotoFirst()
{
	gotoFirstPotential();
	if (notAtEnd() && !getCurrent()->isReal)
		gotoNext();
}

void InteractionHashMap::gotoNext()
{
	gotoNextPotential();
	while( notAtEnd() && !getCurrent()->isReal)
		gotoNextPotential();
}

bool InteractionHashMap::notAtEnd()
{
	return notAtEndPotential();
}

const shared_ptr<Interaction>& InteractionHashMap::getCurrent()
{
	return (*hmii).second;
}


void InteractionHashMap::eraseCurrentAndGotoNextPotential()
{
	if (notAtEnd())
	{
		IHashMap::iterator tmpHmii=hmii;
		++hmii;
		volatileInteractions.erase(tmpHmii);
	}
}

void InteractionHashMap::eraseCurrentAndGotoNext()
{
	IHashMap::iterator tmpHmii=hmii;	
	while (notAtEnd() && !((*hmii).second->isReal))
		++hmii;	
	volatileInteractions.erase(tmpHmii);
}

unsigned int InteractionHashMap::size()
{
	return volatileInteractions.size();
}
