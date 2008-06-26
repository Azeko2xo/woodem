#include"ParallelEngine.hpp"
YADE_PLUGIN("ParallelEngine");
list<string> ParallelEngine::getNeededBex(){
	list<string> ret;
	FOREACH(const vector<shared_ptr<Engine> >& ve, slaves){
		FOREACH(const shared_ptr<Engine>& e, ve){
			list<string> rret=e->getNeededBex();
			FOREACH(const string& bex, rret) {ret.push_back(bex);}
		}
	}
	return ret;
}

void ParallelEngine::action(MetaBody* rootBody){
	// openMP warns if the iteration variable is unsigned...
	const int size=(int)slaves.size();
	#pragma omp for schedule(dynamic,1)
	for(int i=0; i<size; i++){
		// run every slave group sequentially
		FOREACH(const shared_ptr<Engine>& e, slaves[i])
			if(e->isActivated()) e->action(rootBody);
	}
}
