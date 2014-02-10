// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include<cmath> // workaround for http://boost.2283326.n4.nabble.com/Boost-Python-Compile-Error-s-GCC-via-MinGW-w64-tp3165793p3166760.html
#include<boost/python.hpp>
#include<boost/chrono/chrono.hpp>
#include<boost/thread/mutex.hpp>

#include<woo/lib/base/Types.hpp>
#include<woo/lib/base/openmp-accu.hpp>

namespace woo{

struct TimingInfo{
	typedef unsigned long long delta;
	long nExec;
	delta nsec;
	TimingInfo():nExec(0),nsec(0){}
	static delta getNow(bool evenIfDisabled=false)
	{
		if(!enabled && !evenIfDisabled) return 0L;
		auto now=boost::chrono::steady_clock::now();
		return delta(boost::chrono::duration_cast<boost::chrono::nanoseconds>(now.time_since_epoch()).count());
	}
	static bool enabled;
};

/* Create TimingDeltas object, then every call to checkpoint() will add
 * (or use existing) TimingInfo to data. It increases its nExec by 1
 * and nsec by time elapsed since construction or last checkpoint.
 */
struct TimingDeltas{
	private:
		/* mutex-protected (high-overhead) storage for data not yet in arrays from parallel checkpoints
		   used when the index is out-of-range for arrays; arrays may be resized
			in non-parallel checkpoints or from start, via the call to consolidate()
		*/
		#ifdef WOO_OPENMP
			// last time point, separate for each thread
			vector<TimingInfo::delta> last;
			boost::mutex mapMutex;
			struct LabelCountTime{ string label; long nExec; TimingInfo::delta nsec; LabelCountTime(): nExec(0), nsec(0){}; };
			std::map<int,LabelCountTime> timingMap;
			vector<int> nThreads;
		#else
			TimingInfo::delta last;
		#endif

		// contiguous (low overhead - lockfree write) storage
		OpenMPArrayAccumulator<long> nExec;
		OpenMPArrayAccumulator<TimingInfo::delta> nsec;
		vector<string> labels;
		// resize arrays, put mapped data to arrays with OpenMP
		// does nothing when called with -1 without OpenMP
		void consolidate(int newSize=-1);
	public:
		TimingDeltas()
			#ifdef WOO_OPENMP
				: last(omp_get_max_threads())
			#endif
			{}
		void start();
		void checkpoint(const int& index, const string& label);
		void reset();
		py::list pyData();
		
	static void pyRegisterClass();

};

};
