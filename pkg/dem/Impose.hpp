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
REGISTER_SERIALIZABLE(HarmonicOscillation);

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
REGISTER_SERIALIZABLE(AlignedHarmonicOscillations);

struct RadialForce: public Impose{
	void force(const Scene* scene, const shared_ptr<Node>& n);
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(RadialForce,Impose,"Impose constant force towards an axis in 3d.",
		((shared_ptr<Node>,nodeA,,,"First node defining the axis"))
		((shared_ptr<Node>,nodeB,,,"Second node defining the axis"))
		((Real,F,0,,"Magnitude of the force applied. Positive value means away from the axis given by *nodeA* and *nodeB*."))
		,/*ctor*/ what=Impose::FORCE;
	);
};
REGISTER_SERIALIZABLE(RadialForce);

struct Local6Dofs: public Impose{
	void velocity(const Scene* scene, const shared_ptr<Node>& n){ doImpose(scene,n,/*velocity*/true); }
	void force(const Scene* scene, const shared_ptr<Node>& n)   { doImpose(scene,n,/*velocity*/false); }
	// function to impose both, distinguished by the last argument
	void doImpose(const Scene* scene, const shared_ptr<Node>& n, bool velocity){
		DemData& dyn=n->getData<DemData>();
		for(int i=0;i<6;i++){
			if( velocity&&(whats[i]&Impose::VELOCITY)){
				if(i<3){
					//dyn.vel+=ori.conjugate()*(Vector3r::Unit(i)*(values[i]-(ori*dyn.vel).dot(Vector3r::Unit(i))));
					Vector3r locVel=ori*dyn.vel; locVel[i]=values[i]; dyn.vel=ori.conjugate()*locVel;
				} else {
					//dyn.angVel+=ori.conjugate()*(Vector3r::Unit(i%3)*(values[i]-(ori*dyn.angVel).dot(Vector3r::Unit(i%3))));
					Vector3r locAngVel=ori*dyn.angVel; locAngVel[i%3]=values[i]; dyn.angVel=ori.conjugate()*locAngVel;
				}
			}
			if(!velocity&&(whats[i]&Impose::FORCE)){
				// set absolute value, cancel anything set previously (such as gravity)
				if(i<3){ dyn.force =ori.conjugate()*(values[i]*Vector3r::Unit(i)); }
				else   { dyn.torque=ori.conjugate()*(values[i]*Vector3r::Unit(i%3)); }
			}
		}
	}
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
REGISTER_SERIALIZABLE(Local6Dofs);
