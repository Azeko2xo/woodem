#pragma once
#include<yade/pkg/dem/Particle.hpp>
#include<yade/core/Engine.hpp>

struct RadialForce: public GlobalEngine, private DemField::Engine {
	virtual void run();
	virtual void postLoad(RadialForce&){ axisDir.normalize(); }
	YADE_CLASS_BASE_DOC_ATTRS(RadialForce,GlobalEngine,"Apply force of given magnitude directed away from spatial axis on some nodes.",
		((vector<shared_ptr<Node>>,nodes,,,"Nodes on which the force is applied"))
		((Vector3r,axisPt,Vector3r::Zero(),,"Point on axis"))
		((Vector3r,axisDir,Vector3r::UnitX(),Attr::triggerPostLoad,"Axis direction (normalized automatically)"))
		((Real,fNorm,0,,"Applied force magnitude"))
	);
};
REGISTER_SERIALIZABLE(RadialForce);
