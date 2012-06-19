// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<time.h>
#include<boost/python.hpp>

namespace yade{

struct TimingInfo{
	typedef unsigned long long delta;
	long nExec;
	delta nsec;
	TimingInfo():nExec(0),nsec(0){}
	static delta getNow(bool evenIfDisabled=false)
	{
		if(!enabled && !evenIfDisabled) return 0L;
		struct timespec ts; 
		clock_gettime(CLOCK_MONOTONIC,&ts); 
		return delta(1e9*ts.tv_sec+ts.tv_nsec);		
	}
	static bool enabled;
};

/* Create TimingDeltas object, then every call to checkpoint() will add
 * (or use existing) TimingInfo to data. It increases its nExec by 1
 * and nsec by time elapsed since construction or last checkpoint.
 */
class TimingDeltas{
		TimingInfo::delta last;
		size_t i;
	public:
		vector<TimingInfo> data;
		vector<string> labels;
		TimingDeltas():i(0){}
		void start(){if(!TimingInfo::enabled)return; i=0;last=TimingInfo::getNow();}
		void checkpoint(const string& label){
			if(!TimingInfo::enabled) return;
			if(data.size()<=i) { data.resize(i+1); labels.resize(i+1); labels[i]=label;}
			TimingInfo::delta now=TimingInfo::getNow();
			data[i].nExec+=1; data[i].nsec+=now-last; last=now; i++;
		}
		void reset(){ data.clear(); labels.clear(); }
		// python access
		boost::python::list pyData(){
			boost::python::list ret;
			for(size_t i=0; i<data.size(); i++){ ret.append(boost::python::make_tuple(labels[i],data[i].nsec,data[i].nExec));}
			return ret;
		}
	static void pyRegisterClass(){
		py::class_<TimingDeltas, shared_ptr<TimingDeltas>, boost::noncopyable>("TimingDeltas")
			.add_property("data",&TimingDeltas::pyData,"Get timing data as list of tuples (label, execTime[nsec], execCount) (one tuple per checkpoint)")
			.def("reset",&TimingDeltas::reset,"Reset timing information")
		;
	}

};

};
