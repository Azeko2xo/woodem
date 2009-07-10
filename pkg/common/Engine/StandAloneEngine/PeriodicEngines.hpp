// 2008 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include<time.h>
#include<yade/core/StandAloneEngine.hpp>
#include<yade/core/Omega.hpp>
/* run an action with given fixed periodicity (real time, virtual time, iteration number), by setting any of 
 * those criteria to a number > 0.
 *
 * The number of times this engine is activated can be limited by setting nDo>0. In the contrary case, or if
 * the number of activations was already reached, no action will be called even if any of active period has elapsed.
 *
 * If initRun is set, the engine will run when called for the first time; otherwise it will only set *Last and will be
 * called after desired period elapses for the first time.
 */
class PeriodicEngine:  public StandAloneEngine {
	public:
		static Real getClock(){ timeval tp; gettimeofday(&tp,NULL); return tp.tv_sec+tp.tv_usec/1e6; }
		Real virtPeriod, virtLast, realPeriod, realLast; long iterPeriod,iterLast,nDo,nDone;
		bool initRun;
		PeriodicEngine(): virtPeriod(0),virtLast(0),realPeriod(0),realLast(0),iterPeriod(0),iterLast(0),nDo(-1),nDone(0),initRun(false) { realLast=getClock(); }
		virtual bool isActivated(MetaBody*){
			Real virtNow=Omega::instance().getSimulationTime();
			Real realNow=getClock();
			long iterNow=Omega::instance().getCurrentIteration();
			if((nDo<0 || nDone<nDo) &&
				((virtPeriod>0 && virtNow-virtLast>=virtPeriod) ||
				 (realPeriod>0 && realNow-realLast>=realPeriod) ||
				 (iterPeriod>0 && iterNow-iterLast>=iterPeriod))){
				realLast=realNow; virtLast=virtNow; iterLast=iterNow; nDone++;
				return true;
			}
			if(nDone==0){
				realLast=realNow; virtLast=virtNow; iterLast=iterNow; nDone++;
				if(initRun) return true;
				return false;
			}
			return false;
		}
	REGISTER_ATTRIBUTES(StandAloneEngine,
		(virtPeriod)
		(realPeriod)
		(iterPeriod)
		(virtLast)
		(realLast)
		(iterLast)
		(nDo)
		(nDone)
	)
	REGISTER_CLASS_AND_BASE(PeriodicEngine,StandAloneEngine);
};
REGISTER_SERIALIZABLE(PeriodicEngine);

#if 0
class StridePeriodicEngine: public PeriodicEngine{
	public:
		StridePeriodicEngine(): stride, maxStride
}
#endif

/* PeriodicEngine but with constraint that may be stretched by a given stretchFactor (default 2).
 * Limits for each periodicity criterion may be set and the mayStretch bool says whether the period
 * can be stretched (default: doubled) without active criteria getting off limits.
 *
 * stretchFactor must be positive; if >1, period is stretched, for <1, it is shrunk.
 *
 * Limit consistency (whether actual period is not over/below the limit) is checked: period is set to the 
 * limit value if we are off. If the limit is zero, however, and the period is non-zero, the limit is set
 * to the period value (therefore, if you initialize only iterPeriod, you will get what you expect: engine
 * running at iterPeriod).
 */
class StretchPeriodicEngine: public PeriodicEngine{
	public:
	StretchPeriodicEngine(): PeriodicEngine(), realLim(0.), virtLim(0.), iterLim(0), stretchFactor(2.), mayStretch(false){}
	Real realLim, virtLim; long iterLim;
	Real stretchFactor;
	bool mayStretch;
	virtual bool isActivated(MetaBody* rootBody){
		assert(stretchFactor>0);
		if(iterLim==0 && iterPeriod!=0){iterLim=iterPeriod;} else if(iterLim!=0 && iterPeriod==0){iterPeriod=iterLim;}
		if(realLim==0 && realPeriod!=0){realLim=realPeriod;} else if(realLim!=0 && realPeriod==0){realPeriod=realLim;}
		if(virtLim==0 && virtPeriod!=0){virtLim=virtPeriod;} else if(virtLim!=0 && virtPeriod==0){virtPeriod=virtLim;}
		if(stretchFactor>1){iterPeriod=min(iterPeriod,iterLim); realPeriod=min(realPeriod,realLim); virtPeriod=min(virtPeriod,virtLim);}
		else {iterPeriod=max(iterPeriod,iterLim); realPeriod=max(realPeriod,realLim); virtPeriod=max(virtPeriod,virtLim);}
		mayStretch=((virtPeriod<0 || (stretchFactor>1 ? stretchFactor*virtPeriod<=virtLim : stretchFactor*virtPeriod>=virtLim))
		&& (realPeriod<0 || (stretchFactor>1 ? stretchFactor*realPeriod<=realLim : stretchFactor*realPeriod>=realLim))
		&& (iterPeriod<0 || (stretchFactor>1 ? stretchFactor*iterPeriod<=iterLim : stretchFactor*iterPeriod>=iterLim)));
		return PeriodicEngine::isActivated(rootBody);
	}
	protected:
		void registerAttributes(){ PeriodicEngine::registerAttributes();
			REGISTER_ATTRIBUTE(realLim);
			REGISTER_ATTRIBUTE(virtLim);
			REGISTER_ATTRIBUTE(iterLim);
			REGISTER_ATTRIBUTE(mayStretch);
			REGISTER_ATTRIBUTE(stretchFactor);
		}
	REGISTER_CLASS_NAME(StretchPeriodicEngine);
	REGISTER_BASE_CLASS_NAME(PeriodicEngine);
};
REGISTER_SERIALIZABLE(StretchPeriodicEngine);

// obsolete, too complicated etc
#if 0 

/* Run an action with adjustable and constrained periodicity (real time, virtual time, iteration)
 *
 * 3 criteria can be used: iteration period, realTime (wallclock) period, virtTime (simulation time) period.
 * Each of them is composed of a Vector3r with the meaning [lower limit, actual period value, upper limit].
 * The criterion is disabled if the lower limit is < 0, which is the default.
 * If _any_ criterion from the enabled ones is satisfied (elapsed iteration period etc.), the engine becomes active.
 *
 * At the first run after construction, the engine will ensure period consistency,
 * i.e. that on enabled periods, limits are properly ordered (swapped otherwise)
 * and that the actual value is not over the upper limit (will be made equal to it otherwise) ot under the lower limit.
 *
 * If it is useful to make the actual periods smaller/larger, mayDouble and mayHalve signify whether actual periods
 * (considering only enabled criteria) may be halved/doubled without getting off limits.
 *
 * This engine may be used only by deriving an engine with something useful in action(MetaBody*);
 * if used as-is, it will throw when activated.
 */
class __attribute__((deprecated)) RangePeriodicEngine: public StandAloneEngine {
	private:
		Vector3r virtTimeLim,realTimeLim,iterLim;
		Real lastRealTime,lastVirtTime; long lastIter;
		bool mayDouble,mayHalve;
		bool perhapsInconsistent;
		void ensureConsistency(Vector3r& v){
			if(v[0]<0) return; // not active
			if(v[2]<v[0]){Real lo=v[2]; v[2]=v[0]; v[0]=lo;} // swap limits if necessary
			if(v[1]<v[0]) v[1]=v[0]; // put actual value below the lower limit to the lower limit
			if(v[1]>v[2]) v[2]=v[1]; // put actual value above the upper limit to the upper limit
		}
	public :
		RangePeriodicEngine(): virtTimeLim(-1,0,0),realTimeLim(-1,0,0),iterLim(-1,0,0), lastRealTime(0.),lastVirtTime(0.),lastIter(0),mayDouble(false),mayHalve(false),perhapsInconsistent(true){};
		virtual void action(MetaBody* b) { throw; }
		virtual bool isActivated(MetaBody* rootBody){
			if(perhapsInconsistent){ ensureConsistency(virtTimeLim); ensureConsistency(realTimeLim); ensureConsistency(iterLim); perhapsInconsistent=false; }

			mayDouble=((virtTimeLim[0]<0 || 2*virtTimeLim[1]<=virtTimeLim[2]) && (realTimeLim[0]<0 || 2*realTimeLim[1]<=realTimeLim[2]) && (iterLim[0]<0 || 2*iterLim[1]<=iterLim[2]));
			mayHalve=((virtTimeLim[0]<0 || .5*virtTimeLim[1]>=virtTimeLim[0]) && (realTimeLim[0]<0 && .5*realTimeLim[1]>=realTimeLim[0]) && (iterLim[0]<0 || .5*iterLim[1]>=iterLim[0]));
			
			long nowIter=Omega::instance().getCurrentIteration();
			Real nowVirtTime=Omega::instance().getSimulationTime();
			//Real nowRealTime=boost::posix_time::microsec_clock::universal_time()/1e6;
			timeval tp; gettimeofday(&tp,NULL); Real nowRealTime=tp.tv_sec+tp.tv_usec/1e6;
			//cerr<<"--------------------"<<endl; cerr<<"virt:"<<virtTimeLim<<";"<<lastVirtTime<<";"<<nowVirtTime<<endl; cerr<<"real:"<<realTimeLim<<";"<<lastRealTime<<";"<<nowRealTime<<endl; cerr<<"iter:"<<iterLim<<";"<<lastIter<<";"<<nowIter<<endl;
			if (  (virtTimeLim[0]>=0 && nowVirtTime-lastVirtTime>virtTimeLim[1])
			 || (realTimeLim[0]>=0 && nowRealTime-lastRealTime>realTimeLim[1])
			 || (iterLim[0]>=0     && nowIter    -lastIter    >iterLim[1])){
				lastVirtTime=nowVirtTime; lastRealTime=nowRealTime; lastIter=nowIter;
				return true;
			} else return false;
		}
		virtual void registerAttributes(){
			StandAloneEngine::registerAttributes();
			REGISTER_ATTRIBUTE(virtTimeLim);
			REGISTER_ATTRIBUTE(realTimeLim);
			REGISTER_ATTRIBUTE(iterLim);
			REGISTER_ATTRIBUTE(lastRealTime);
			REGISTER_ATTRIBUTE(lastVirtTime);
			REGISTER_ATTRIBUTE(lastIter);
			REGISTER_ATTRIBUTE(mayDouble);
			REGISTER_ATTRIBUTE(mayHalve);
		}
	protected :
		virtual void postProcessAttributes(bool deserializing){}
	DECLARE_LOGGER;
	REGISTER_CLASS_NAME(RangePeriodicEngine);
	REGISTER_BASE_CLASS_NAME(StandAloneEngine);
};
REGISTER_SERIALIZABLE(RangePeriodicEngine);

#endif
