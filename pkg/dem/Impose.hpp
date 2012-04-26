/* the base class, Impose, is declared in the Particle.hpp file for simplicity */
#include<yade/pkg/dem/Particle.hpp>

struct HarmonicOscillation: public Impose{
	void operator()(const Scene* scene, const shared_ptr<Node>& n){
		// http://en.wikipedia.org/wiki/Simple_harmonic_motion
		Real omega=2*Mathr::PI*freq;
		Real vMag=amp*omega*cos(omega*(scene->time-t0));
		Vector3r& vv(n->getData<DemData>().vel);
		if(!perpFree) vv=dir*vMag;
		else{ /*subtract projection on dir*/ vv-=vv.dot(dir)*dir; /* apply new value instead */ vv+=vMag*dir; }
	}
	void postLoad(HarmonicOscillation&){ dir.normalize(); }
	YADE_CLASS_BASE_DOC_ATTRS(HarmonicOscillation,Impose,"Impose harmonic oscillation around initial center position, with given frequency and amplitude, by prescribing velocity.",
		((Real,freq,NaN,,"Frequence of oscillation"))
		((Real,amp,NaN,,"Amplitude of oscillation"))
		((Vector3r,dir,Vector3r::UnitX(),,"Direcrtion of oscillation (normalized automatically)"))
		((Real,t0,0,,"Time when the oscillator is in the center position (phase)"))
		((bool,perpFree,false,,"If true, only velocity in the *dir* sense will be prescribed, velocity in the perpendicular sense will be preserved."))
	);
};
REGISTER_SERIALIZABLE(HarmonicOscillation);

struct AlignedHarmonicOscillations: public Impose{
	void operator()(const Scene* scene, const shared_ptr<Node>& n){
		Vector3r& vv(n->getData<DemData>().vel);
		for(int ax:{0,1,2}){
			if(isnan(freqs[ax])||isnan(amps[ax])) continue;
			Real omega=2*Mathr::PI*freqs[ax];
			vv[ax]=amps[ax]*omega*cos(omega*scene->time);
		}
	}
	YADE_CLASS_BASE_DOC_ATTRS(AlignedHarmonicOscillations,Impose,"Imposes three independent harmonic oscillations along global coordinate system axes.",
		((Vector3r,freqs,Vector3r(NaN,NaN,NaN),,"Frequencies for individual axes. NaN value switches that axis off, the component will not be touched"))
		((Vector3r,amps,Vector3r::Zero(),,"Amplitudes along individual axes."))
	);
};
REGISTER_SERIALIZABLE(AlignedHarmonicOscillations);


