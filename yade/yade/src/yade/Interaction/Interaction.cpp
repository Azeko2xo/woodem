#include "Interaction.hpp"

Interaction::Interaction ()
{
	// FIXME : -1
	id1 = 0;
	id2 = 0;
	isNew = true;
	isReal = false; // maybe we can remove this, and check if BodyInteractionGeometry, and InteractionPhysics are empty?
}

Interaction::Interaction(unsigned int newId1,unsigned int newId2) : id1(newId1) , id2(newId2)
{	
	isNew = true;
	isReal = false;
}

Interaction::~Interaction ()
{

}

void Interaction::postProcessAttributes(bool)
{

}

void Interaction::registerAttributes()
{
	REGISTER_ATTRIBUTE(id1);
	REGISTER_ATTRIBUTE(id2);
	REGISTER_ATTRIBUTE(isReal);
	REGISTER_ATTRIBUTE(interactionGeometry);
	REGISTER_ATTRIBUTE(interactionPhysics);
}
