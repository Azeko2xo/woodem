#include<woo/pkg/dem/ContactContainer.hpp>
#include<woo/pkg/dem/Particle.hpp>

#ifdef WOO_OPENMP
	#include<omp.h>
#endif

CREATE_LOGGER(ContactContainer);

bool ContactContainer::IsReal::operator()(shared_ptr<Contact>& c){ return c && c->isReal(); }
bool ContactContainer::IsReal::operator()(const shared_ptr<Contact>& c){ return c && c->isReal(); }

bool ContactContainer::add(const shared_ptr<Contact>& c, bool threadSafe){
	assert(dem);
	#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
		boost::mutex::scoped_lock lock(*manipMutex);
	#endif
	if(!threadSafe){
		// make sure the contact does not exist yet
		assert(c->pA->contacts.find(c->pB->id)==c->pA->contacts.end());
		assert(c->pB->contacts.find(c->pA->id)==c->pB->contacts.end());
	} else {
		if(c->pA->contacts.find(c->pB->id)!=c->pA->contacts.end() || c->pB->contacts.find(c->pA->id)!=c->pB->contacts.end()) return false;
	}

	c->pA->contacts[c->pB->id]=c;
	c->pB->contacts[c->pA->id]=c;
	linView.push_back(c);
	// store the index back-reference in the interaction (so that it knows how to (re)move itself)
	c->linIx=linView.size()-1; 
	return true;
}

void ContactContainer::clear(){
	assert(dem);
	#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
		boost::mutex::scoped_lock lock(*manipMutex);
	#endif
	FOREACH(const shared_ptr<Particle>& p, *dem->particles) p->contacts.clear();
	linView.clear(); // clear the linear container
	clearPending();
	dirty=true;
}

bool ContactContainer::remove(shared_ptr<Contact> c, bool threadSafe){
	assert(dem);
	#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
		boost::mutex::scoped_lock lock(*manipMutex);
	#endif
	Particle::id_t idA(c->pA->id), idB(c->pB->id);
	// make sure the contact is inside the dem->particles
	Particle::MapParticleContact::iterator iA=c->pA->contacts.find(idB), iB=c->pB->contacts.find(idA);
	if(!threadSafe){
		// particle deleted from engine while at the same time pending; check that the particle vanished
		if((iA==c->pA->contacts.end() && c->pA.get()!=(*dem->particles)[idA].get()) || 
			(iB==c->pB->contacts.end() && c->pB.get()!=(*dem->particles)[idB].get())) return false;
		// this can happen if _contact_ were deleted directly, not by ContactLoop; but that is an error
		if(iA==c->pA->contacts.end() || iB==c->pB->contacts.end()){
			LOG_FATAL("Contact ##"<<idA<<"+"<<idB<<" vanished from particle!");
			if(iA==c->pA->contacts.end()) LOG_FATAL("not in O.dem.par["<<idA<<"].con (particle exists)");
			if(iB==c->pB->contacts.end()) LOG_FATAL("not in O.dem.par["<<idB<<"].con (particle exists)");
			abort();
		}
		// this is superfluous with the diagnostics above now
		assert(iA!=c->pA->contacts.end()); assert(iB!=c->pB->contacts.end());
	}
	else { if (iA==c->pA->contacts.end() || iB==c->pB->contacts.end()) return false; }
	// and in the linear container, that it is the same we got, and the same we found in dem->particles
	assert(linView.size()>c->linIx);	assert(linView[c->linIx]==c); assert(iA->second==c); assert(iB->second==c);
	// remove from dem->particles
	c->pA->contacts.erase(iA); c->pB->contacts.erase(iB);
	// remove from linear view, keeping it compact
	if(c->linIx<linView.size()-1){ // is not the last element
		//cerr<<"linIx="<<c->linIx<<"/"<<linView.size()<<endl;
		//if(!linView.back()) throw std::logic_error("linView.back it empty!");
		linView[c->linIx]=linView.back(); // move the last to the place of the one being removed
		//if(c->linIx<0) throw std::logic_error("##"+to_string(idA)+"+"+to_string(idB)+": linIx="+to_string(c->linIx)+"?!");
		//if(!linView[c->linIx]) throw std::logic_error("No contact at linIx="+to_string(c->linIx));
		assert(linView[c->linIx]);
		linView[c->linIx]->linIx=c->linIx; // and update its linIx
	}
	linView.resize(linView.size()-1);
	return true;
};

const shared_ptr<Contact>& ContactContainer::find(ParticleContainer::id_t idA, ParticleContainer::id_t idB) const {
	assert(dem);
	assert(!nullContactPtr);
	if(!dem->particles->exists(idA)) return nullContactPtr;
	Particle::MapParticleContact::iterator I((*dem->particles)[idA]->contacts.find(idB));
	if(I!=(*dem->particles)[idA]->contacts.end()) return I->second;
	return nullContactPtr;
};

// query existence; use find(...) to get the instance if it exists as well
bool ContactContainer::exists(ParticleContainer::id_t idA, ParticleContainer::id_t idB) const {
	assert(dem);
	if(!dem->particles->exists(idA)) return false;
	Particle::MapParticleContact::iterator I((*dem->particles)[idA]->contacts.find(idB));
	if(I==(*dem->particles)[idA]->contacts.end()) return false;
	return true;
}



void ContactContainer::requestRemoval(const shared_ptr<Contact>& c, bool force){
	c->reset(); PendingContact v={c,force};
	#ifdef WOO_OPENMP
		threadsPending[omp_get_thread_num()].push_back(v);
	#else
		pending.push_back(v);
	#endif
}

void ContactContainer::clearPending(){
	#ifdef WOO_OPENMP
		FOREACH(list<PendingContact>& pending, threadsPending){
			pending.clear();
		}
	#else
		pending.clear();
	#endif
}

int ContactContainer::removeAllPending(){
	int ret=0;
	#ifdef WOO_OPENMP
		// shadow this->pendingErase by the local variable, to share code
		FOREACH(list<PendingContact>& pending, threadsPending){
	#endif
			if(!pending.empty()){
				FOREACH(const PendingContact& p, pending){ if(remove(p.contact)) ret++; }
				pending.clear();
			}
	#ifdef WOO_OPENMP
		}
	#endif
	return ret;
}

void ContactContainer::removeNonReal(){
	std::list<shared_ptr<Contact> > cc;
	FOREACH(const shared_ptr<Contact>& c, linView){ if(!c->isReal()) cc.push_back(c); }
	FOREACH(const shared_ptr<Contact>& c, cc){ this->remove(c); }
}

int ContactContainer::countReal() const{
	int ret=0;
	FOREACH(__attribute__((unused)) const shared_ptr<Contact>& c, *this) ret++;
	return ret;
};


shared_ptr<Contact> ContactContainer::pyByIds(const Vector2i& ids){return find(ids[0],ids[1]); };

shared_ptr<Contact> ContactContainer::pyNth(int n){
	int sz(size());
	if(n<-sz || n>sz-1) IndexError("Linear index out of range ("+lexical_cast<string>(-sz)+".."+lexical_cast<string>(sz-1)+")");
	return linView[n>=0?n:size()-n];
}
ContactContainer::pyIterator ContactContainer::pyIter(){ return ContactContainer::pyIterator(this); }

ContactContainer::pyIterator::pyIterator(ContactContainer* _cc): cc(_cc), ix(0){}
shared_ptr<Contact> ContactContainer::pyIterator::next(){
	int sz=(int)cc->size();
	while(ix<sz && !(*cc)[ix]->isReal()) ix++;
	if(ix>=sz) yade::StopIteration();
	return (*cc)[ix++];
}
ContactContainer::pyIterator ContactContainer::pyIterator::iter(){ return *this; }

