#ifndef __OSCILLATOR_H__
#define __OSCILLATOR_H__

#include "KinematicEngine.hpp"

#ifdef WIN32
#include <windows.h>
#endif

class Rotor : public KinematicEngine
{
	// construction	
	public : Rotor ();
	public : virtual ~Rotor ();
	
	public : void moveToNextTimeStep(std::vector<shared_ptr<Body> >& bodies, float dt);
	
	public : void processAttributes();
	public : void registerAttributes();
	REGISTER_CLASS_NAME(Rotor);
};

REGISTER_CLASS(Rotor,false);

#endif // __OSCILLATOR_H__
