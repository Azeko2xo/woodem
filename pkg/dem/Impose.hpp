/* the base class, Impose, is declared in the Particle.hpp file for simplicity */
#pragma once
#include<woo/pkg/dem/Particle.hpp>

struct HarmonicOscillation: public Impose{
	void velocity(const Scene* scene, const shared_ptr<Node>& n){
		// http://en.wikipedia.org/wiki/Simple_harmonic_motion
		Real omega=2*M_PI*freq;
		Real vMag=amp*omega*cos(omega*(scene->time-t0));
		Vector3r& vv(n->getData<DemData>().vel);
		if(!perpFree) vv=dir*vMag;
		else{ /*subtract projection on dir*/ vv-=vv.dot(dir)*dir; /* apply new value instead */ vv+=vMag*dir; }
	}
	void postLoad(HarmonicOscillation&,void*){ dir.normalize(); }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(HarmonicOscillation,Impose,"Impose harmonic oscillation around initial center position, with given frequency and amplitude, by prescribing velocity.",
		((Real,freq,NaN,,"Frequence of oscillation"))
		((Real,amp,NaN,,"Amplitude of oscillation"))
		((Vector3r,dir,Vector3r::UnitX(),,"Direcrtion of oscillation (normalized automatically)"))
		((Real,t0,0,,"Time when the oscillator is in the center position (phase)"))
		((bool,perpFree,false,,"If true, only velocity in the *dir* sense will be prescribed, velocity in the perpendicular sense will be preserved."))
		,/*ctor*/ what=Impose::VELOCITY;
	);
};
WOO_REGISTER_OBJECT(HarmonicOscillation);

struct CircularOrbit: public Impose{
	void velocity(const Scene* scene, const shared_ptr<Node>&n) WOO_CXX11_OVERRIDE;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(CircularOrbit,Impose,"Imposes circual orbiting around the local z-axis; the velocity is prescribed using midstep position.",
		((shared_ptr<Node>,node,make_shared<Node>(),,"Local coordinate system."))
		((bool,rotate,false,,"Impose rotational velocity so that orientation relative to the local z-axis is always the same.\n\n.. warning:: This is not yet implemented."))
		((Real,omega,NaN,,"Orbiting angular velocity."))
		((Real,angle,0,,"Cumulative angle turned, incremented at every step."))
		,/*ctor*/ what=Impose::VELOCITY;
	);
};
WOO_REGISTER_OBJECT(CircularOrbit);

struct AlignedHarmonicOscillations: public Impose{
	void velocity(const Scene* scene, const shared_ptr<Node>& n){
		Vector3r& vv(n->getData<DemData>().vel);
		for(int ax:{0,1,2}){
			if(isnan(freqs[ax])||isnan(amps[ax])) continue;
			Real omega=2*M_PI*freqs[ax];
			vv[ax]=amps[ax]*omega*cos(omega*scene->time);
		}
	}
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(AlignedHarmonicOscillations,Impose,"Imposes three independent harmonic oscillations along global coordinate system axes.",
		((Vector3r,freqs,Vector3r(NaN,NaN,NaN),,"Frequencies for individual axes. NaN value switches that axis off, the component will not be touched"))
		((Vector3r,amps,Vector3r::Zero(),,"Amplitudes along individual axes."))
		,/*ctor*/ what=Impose::VELOCITY;
	);
};
WOO_REGISTER_OBJECT(AlignedHarmonicOscillations);

struct RadialForce: public Impose{
	void force(const Scene* scene, const shared_ptr<Node>& n);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(RadialForce,Impose,"Impose constant force towards an axis in 3d.",
		((shared_ptr<Node>,nodeA,,,"First node defining the axis"))
		((shared_ptr<Node>,nodeB,,,"Second node defining the axis"))
		((Real,F,0,,"Magnitude of the force applied. Positive value means away from the axis given by *nodeA* and *nodeB*."))
		,/*ctor*/ what=Impose::FORCE;
	);
};
WOO_REGISTER_OBJECT(RadialForce);

struct Local6Dofs: public Impose{
	void velocity(const Scene* scene, const shared_ptr<Node>& n){ doImpose(scene,n,/*velocity*/true); }
	void force(const Scene* scene, const shared_ptr<Node>& n)   { doImpose(scene,n,/*velocity*/false); }
	void doImpose(const Scene* scene, const shared_ptr<Node>& n, bool velocity);
	void postLoad(Local6Dofs&,void*){
		for(int i=0;i<6;i++) if(whats[i]!=0 && whats[i]!=Impose::FORCE && whats[i]!=Impose::VELOCITY) throw std::runtime_error("Local6Dofs.whats components must be 0, "+to_string(Impose::FORCE)+" or "+to_string(Impose::VELOCITY)+" (whats["+to_string(i)+"] invalid: "+lexical_cast<string>(whats.transpose())+")");
	}
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(Local6Dofs,Impose,"Impose force or velocity along all local 6 axes given by the *trsf* matrix.",
		((Quaternionr,ori,Quaternionr::Identity(),,"Local coordinates rotation"))
		((Vector6r,values,Vector6r::Zero(),,"Imposed values; their meaning depends on the *whats* vector"))
		((Vector6i,whats,Vector6i::Zero(),,"Meaning of *values* components: 0 for nothing imposed (i.e. zero force), 1 for velocity, 2 for force values"))
		,/*ctor*/ what=Impose::VELOCITY | Impose::FORCE;
	);
};
WOO_REGISTER_OBJECT(Local6Dofs);

struct VariableAlignedRotation: public Impose{
	void velocity(const Scene* scene, const shared_ptr<Node>& n);
	void postLoad(VariableAlignedRotation&,void*);
	size_t _interpPos; // cookie for interpolation routine, doees not need to be saved
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(VariableAlignedRotation,Impose,"Impose piecewise-linear angular valocity along :obj:`axis`, based on the :obj:`timeAngVel`.",
		((int,axis,0,,"Rotation axis."))
		((vector<Vector2r>,timeAngVel,,,"Angular vlocity values in time. Time values must be increasing."))
		, /*ctor*/ what=Impose::VELOCITY;
	);
};
WOO_REGISTER_OBJECT(VariableAlignedRotation);
