#include<woo/core/Timing.hpp>

namespace woo{
	void TimingDeltas::consolidate(int newSize){
		if(newSize<0){
			#ifdef WOO_OPENMP
				if(timingMap.empty()) return;
				// find maximum index
				for(const auto& t: timingMap) newSize=max(newSize,t.first);
			#else
				return;
			#endif
		}
		#ifdef WOO_OPENMP
			newSize*=4; // beter waster RAM than do expensive locking ops
		#endif
		assert(newSize>0);
		if((int)nExec.size()>=newSize) return; // already large enough
		nExec.resize(newSize); nsec.resize(newSize); labels.resize(newSize); 
		#ifdef WOO_OPENMP
			nThreads.resize(newSize,-1);
			// move actual data now
			for(const auto& t: timingMap){
				const auto& index=t.first;
				nExec.set(index,t.second.nExec);
				nsec.set(index,t.second.nsec);
				labels[index]=t.second.label;
			}
		#endif
	}
	void TimingDeltas::start(){
		if(!TimingInfo::enabled) return;
		consolidate();
		#ifdef WOO_OPENMP
			assert(!omp_in_parallel());
			std::fill(last.begin(),last.end(),TimingInfo::getNow());
		#else
			last=TimingInfo::getNow();
		#endif
	}
	void TimingDeltas::checkpoint(const int& index, const string& label){
		if(!TimingInfo::enabled) return;
		#ifdef WOO_OPENMP
			assert(omp_get_thread_num()<omp_get_max_threads());
		#endif
		assert(index>=0);
		TimingInfo::delta now=TimingInfo::getNow();
		#ifdef WOO_OPENMP
			TimingInfo::delta dt=now-last[omp_get_thread_num()];
			// if non-parallel, consolidate now
			if(index>=(int)nExec.size() && !omp_in_parallel()) consolidate(index+1);
			if(index<(int)nExec.size()){
		#else
			TimingInfo::delta dt=now-last;
			if(index>=(int)nExec.size()) consolidate(index+1);
		#endif
				/* UGLY: this block executes unconditionally without OpenMP (storage is always consolidated)
					and only sometimes with OpenMP (under the if-else condition)
				*/
				// fast lockfree accumulation
				nExec.add(index,1);
				nsec.add(index,dt);
				// if used for the first time
				if(labels[index].empty()){
					#ifdef WOO_OPENMP
						nThreads[index]=omp_get_num_threads();	
						// lock labels assignment to avoid double free of string
						#pragma omp critical
					#endif
					labels[index]=label;
				}
		#ifdef WOO_OPENMP
			} else {
				// slow mutex-protected
				boost::mutex::scoped_lock lock(mapMutex);
				auto& lct=timingMap[index];
				lct.nExec+=1;
				lct.nsec+=dt;
				if(lct.nExec==1) lct.label=label;
			}
		#endif

		#ifdef WOO_OPENMP
			if(!omp_in_parallel()){ std::fill(last.begin(),last.end(),now); }
			else last[omp_get_thread_num()]=now;
		#else
			last=now;
		#endif
	}
	void TimingDeltas::reset(){
		#ifdef WOO_OPENMP
			boost::mutex::scoped_lock lock(mapMutex);
			timingMap.clear();
		#endif
		nExec.resetAll();
		nsec.resetAll();
	}
	py::list TimingDeltas::pyData(){
		consolidate();
		py::list ret;
		for(size_t i=0; i<nExec.size(); i++){
			if(nExec[i]==0) continue;
			ret.append(py::make_tuple(labels[i],nsec[i],nExec[i],
				#ifdef WOO_OPENMP
					nThreads[i]
				#else
					1
				#endif
			));
		}
		return ret;
	};

}; // namespace woo
