#ifndef __OSCILLATOR_H__
#define __OSCILLATOR_H__

#include "KinematicEngine.hpp"

#ifdef WIN32
#include <windows.h>
#endif

#include "Vector3.hpp"

class Rotor : public KinematicEngine
{

	public : Real angularVelocity;
	public : Vector3r rotationAxis;

	public : void moveToNextTimeStep(Body * body);

	public : void registerAttributes();
	REGISTER_CLASS_NAME(Rotor);
};

REGISTER_SERIALIZABLE(Rotor,false);

#endif // __OSCILLATOR_H__
