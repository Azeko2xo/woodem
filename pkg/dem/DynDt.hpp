#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/IntraForce.hpp>

struct DynDt: public PeriodicEngine{
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void nodalStiffAdd(const shared_ptr<Node>&, Vector3r& kt, Vector3r& kr) const;
	Real nodalCritDtSq(const shared_ptr<Node>&) const;
	virtual void run();
	// virtual func common to all engines
	Real critDt() WOO_CXX11_OVERRIDE { return critDt_stiffness(); }
	// non-virtual func called from run() and from critDt(), the actual implementation
	Real critDt_stiffness() const;
	Real critDt_compute(const shared_ptr<Scene>& s, const shared_ptr<DemField>& f){ scene=s.get(); field=f; return critDt_compute(); }
	Real critDt_compute() const;
	void postLoad(DynDt&,void*);
	DECLARE_LOGGER;
	mutable IntraForce* intraForce; // hack: cache the dispatcher, if available
	WOO_CLASS_BASE_DOC_ATTRS(DynDt,PeriodicEngine,"Adjusts :obj:`Scene.dt` based on current stiffness of particle contacts.",
		((Real,maxRelInc,1e-4,AttrTrait<Attr::triggerPostLoad>(),"Maximum relative increment of timestep within one step, to void abrupt changes in timestep leading to numerical artefacts."))
		((bool,dryRun,false,,"Only set :obj:`dt` to the value of timestep, don't apply it really."))
		((Real,dt,NaN,,"New timestep value which would be used if :obj:`dryRun` were not set. Unused when :obj:`dryRun` is false."))
	);
};
WOO_REGISTER_OBJECT(DynDt);

