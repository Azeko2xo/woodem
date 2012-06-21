#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/core/Engine.hpp>

struct RadialForce: public GlobalEngine {
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual void run();
	virtual void postLoad(RadialForce&){ axisDir.normalize(); }
	WOO_CLASS_BASE_DOC_ATTRS(RadialForce,GlobalEngine,"Apply force of given magnitude directed away from spatial axis on some nodes.",
		((vector<shared_ptr<Node>>,nodes,,,"Nodes on which the force is applied"))
		((Vector3r,axisPt,Vector3r::Zero(),,"Point on axis"))
		((Vector3r,axisDir,Vector3r::UnitX(),AttrTrait<Attr::triggerPostLoad>(),"Axis direction (normalized automatically)"))
		((Real,fNorm,0,,"Applied force magnitude"))
	);
};
REGISTER_SERIALIZABLE(RadialForce);