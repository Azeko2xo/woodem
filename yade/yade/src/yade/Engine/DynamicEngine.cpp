#include "DynamicEngine.hpp"

DynamicEngine::DynamicEngine() : Actor()
{

}
	
DynamicEngine::~DynamicEngine ()
{

}

void DynamicEngine::processAttributes()
{

}

void DynamicEngine::registerAttributes()
{

}

void DynamicEngine::action(Body* b)
{
	this->respondToCollisions(b);
}
