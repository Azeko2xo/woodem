#include "NarrowPhaseCollider.hpp"

NarrowPhaseCollider::NarrowPhaseCollider () : Serializable()
{
}

NarrowPhaseCollider::~NarrowPhaseCollider ()
{

}


void NarrowPhaseCollider::processAttributes()
{
	for(unsigned int i=0;i<collisionFunctors.size();i++)
		narrowManager.addPair(collisionFunctors[i][0],collisionFunctors[i][1],collisionFunctors[i][2]);
}

void NarrowPhaseCollider::registerAttributes()
{
	REGISTER_ATTRIBUTE(collisionFunctors);
}

void NarrowPhaseCollider::addCollisionFunctor(const string& str1,const string& str2,const string& str3)
{
	vector<string> v;
	v.push_back(str1);
	v.push_back(str2);
	v.push_back(str3);
	collisionFunctors.push_back(v);
}
