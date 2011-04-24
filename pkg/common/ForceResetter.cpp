#include<yade/pkg/common/ForceResetter.hpp>
#include<yade/core/Scene.hpp>

YADE_PLUGIN0((ForceResetter));

void ForceResetter::action(){
	scene->forces.reset(scene->iter);
	if(unlikely(scene->trackEnergy)) scene->energy->resetResettables();
}

