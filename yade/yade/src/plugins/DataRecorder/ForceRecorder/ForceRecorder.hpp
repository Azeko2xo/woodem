#ifndef FORCE_RECORDER_HPP_
#define FORCE_RECORDER_HPP_

#include "Actor.hpp"

#include <string>
#include <fstream>

class Action;

class ForceRecorder : public Actor
{
	public : std::string outputFile;
	public : unsigned int interval;
	public : int id;
	public : int bigBallId; // FIXME !!!!!!!!!!
	public : Real bigBallReleaseTime; // FIXME !!!!!!!!!!
	private : shared_ptr<Action> actionForce;
	
	private : std::ofstream ofile; 

	// construction
	public : ForceRecorder ();

	protected : virtual void postProcessAttributes(bool deserializing);
	public : virtual void registerAttributes();

	public : virtual void action(Body* b);
	public : virtual bool isActivated();
	REGISTER_CLASS_NAME(ForceRecorder);
};

REGISTER_SERIALIZABLE(ForceRecorder,false);

#endif // __BALLISTICDYNAMICENGINE_H__
