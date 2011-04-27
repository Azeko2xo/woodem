// 2004 © Janek Kozicki <cosurgi@berlios.de> 
// 2009 © Václav Šmilauer <eudoxos@arcig.cz> 

#pragma once

#include<yade/core/Engine.hpp>

class ForceEngine: public PartialEngine{
	public:
		virtual void action();
	YADE_CLASS_BASE_DOC_ATTRS(ForceEngine,PartialEngine,"Apply contact force on some particles at each step.",
		((Vector3r,force,Vector3r::Zero(),,"Force to apply."))
	);
};
REGISTER_SERIALIZABLE(ForceEngine);

/* Engine for applying force of varying magnitude but constant direction
 * on subscribed bodies. times and magnitudes must have the same length,
 * direction (normalized automatically) gives the orientation.
 *
 * As usual with interpolating engines: the first magnitude is used before the first
 * time point, last magnitude is used after the last time point. Wrap specifies whether
 * time wraps around the last time point to the first time point.
 */
class InterpolatingDirectedForceEngine: public ForceEngine{
	size_t _pos;
	public:
		virtual void action();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(InterpolatingDirectedForceEngine,ForceEngine,"Engine for applying force of varying magnitude but constant direction on subscribed bodies. times and magnitudes must have the same length, direction (normalized automatically) gives the orientation. \n\n\
	\
	As usual with interpolating engines: the first magnitude is used before the first time point, last magnitude is used after the last time point. Wrap specifies whether time wraps around the last time point to the first time point.",
		((vector<Real>,times,,,"Time readings [s]"))
		((vector<Real>,magnitudes,,,"Force magnitudes readings [N]"))
		((Vector3r,direction,Vector3r::UnitX(),,"Contact force direction (normalized automatically)"))
		((bool,wrap,false,,"wrap to the beginning of the sequence if beyond the last time point")),
		/*ctor*/ _pos=0
	);
};
REGISTER_SERIALIZABLE(InterpolatingDirectedForceEngine);

struct RadialForceEngine: public PartialEngine{
	virtual void action();
	virtual void postLoad(RadialForceEngine&);
	YADE_CLASS_BASE_DOC_ATTRS(RadialForceEngine,PartialEngine,"Apply force of given magnitude directed away from spatial axis.",
		((Vector3r,axisPt,Vector3r::Zero(),,"Point on axis"))
		((Vector3r,axisDir,Vector3r::UnitX(),Attr::triggerPostLoad,"Axis direction (normalized automatically)"))
		((Real,fNorm,0,,"Applied force magnitude"))
	);
};
REGISTER_SERIALIZABLE(RadialForceEngine);
