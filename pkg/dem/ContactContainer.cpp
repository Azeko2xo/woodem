#include<yade/pkg/dem/ContactContainer.hpp>
#include<yade/pkg/dem/Particle.hpp>

#ifdef YADE_OPENMP
	#include<omp.h>
#endif

bool ContactContainer::IsReal::operator()(shared_ptr<Contact>& c){ return c && c->isReal(); }
bool ContactContainer::IsReal::operator()(const shared_ptr<Contact>& c){ return c && c->isReal(); }

void ContactContainer::add(const shared_ptr<Contact>& c){
	assert(dem);
	#ifdef YADE_OPENGL
		boost::mutex::scoped_lock lock(*manipMutex);
	#endif
	// make sure the contact does not exist yet
	assert(c->pA->contacts.find(c->pB->id)==c->pA->contacts.end());
	assert(c->pB->contacts.find(c->pA->id)==c->pB->contacts.end());

	c->pA->contacts[c->pB->id]=c;
	c->pB->contacts[c->pA->id]=c;
	linView.push_back(c);
	// store the index back-reference in the interaction (so that it knows how to (re)move itself)
	c->linIx=linView.size()-1; 
}

void ContactContainer::clear(){
	assert(dem);
	#ifdef YADE_OPENGL
		boost::mutex::scoped_lock lock(*manipMutex);
	#endif
	FOREACH(const shared_ptr<Particle>& p, dem->particles) p->contacts.clear();
	linView.clear(); // clear the linear container
	pending.clear();
	dirty=true;
}

void ContactContainer::remove(const shared_ptr<Contact>& c){
	assert(dem);
	#ifdef YADE_OPENGL
		boost::mutex::scoped_lock lock(*manipMutex);
	#endif
	// make sure the contact is inside the dem->particles
	Particle::MapParticleContact::iterator iA=c->pA->contacts.find(c->pB->id), iB=c->pB->contacts.find(c->pA->id);
	assert(iA!=c->pA->contacts.end()); assert(iB!=c->pB->contacts.end());
	// and in the linear container, that it is the same we got, and the same we found in dem->particles
	assert(linView.size()>c->linIx);	assert(linView[c->linIx]==c); assert(iA->second==c); assert(iB->second==c);
	// remove from dem->particles
	c->pA->contacts.erase(iA); c->pB->contacts.erase(iB);
	// remove from linear view, keeping it compact
	if(c->linIx<linView.size()-1){ // is not the last element
		linView[c->linIx]=*linView.rbegin(); // move the last to the place of the one being removed
		linView[c->linIx]->linIx=c->linIx; // and update its linIx
	}
	linView.resize(linView.size()-1);
};

shared_ptr<Contact> ContactContainer::find(ParticleContainer::id_t idA, ParticleContainer::id_t idB){
	assert(dem);
	if(!dem->particles.exists(idA)) return shared_ptr<Contact>();
	Particle::MapParticleContact::iterator I(dem->particles[idA]->contacts.find(idB));
	if(I!=dem->particles[idA]->contacts.end()) return I->second;
	return shared_ptr<Contact>();
};



void ContactContainer::requestRemoval(const shared_ptr<Contact>& c, bool force){
	c->reset(); PendingContact v={c,force};
	#ifdef YADE_OPENMP
		threadsPending[omp_get_thread_num()].push_back(v);
	#else
		pending.push_back(v);
	#endif
}

void ContactContainer::clearPending(){
	#ifdef YADE_OPENMP
		FOREACH(list<PendingContact>& pending, threadsPending){
			pending.clear();
		}
	#else
		pending.clear();
	#endif
}

int ContactContainer::removeAllPending(){
	int ret=0;
	#ifdef YADE_OPENMP
		// shadow this->pendingErase by the local variable, to share code
		FOREACH(list<PendingContact>& pending, threadsPending){
	#endif
			if(!pending.empty()){
				FOREACH(const PendingContact& p, pending){ ret++; remove(p.contact); }
				pending.clear();
			}
	#ifdef YADE_OPENMP
		}
	#endif
	return ret;
}

void ContactContainer::removeNonReal(){
	std::list<shared_ptr<Contact> > cc;
	FOREACH(const shared_ptr<Contact>& c, linView){ if(!c->isReal()) cc.push_back(c); }
	FOREACH(const shared_ptr<Contact>& c, cc){ this->remove(c); }
}


shared_ptr<Contact> ContactContainer::pyByIds(const Vector2i& ids){return find(ids[0],ids[1]); };

shared_ptr<Contact> ContactContainer::pyNth(int n){
	int sz(size());
	if(n<-sz || n>sz-1) IndexError("Linear index out of range ("+lexical_cast<string>(-sz)+".."+lexical_cast<string>(sz-1)+")");
	return linView[n>=0?n:size()-n];
}
ContactContainer::pyIterator ContactContainer::pyIter(){ return ContactContainer::pyIterator(this); }

ContactContainer::pyIterator::pyIterator(ContactContainer* _cc): cc(_cc), ix(0){}
shared_ptr<Contact> ContactContainer::pyIterator::next(){
	if(ix>=(int)cc->size()) yade::StopIteration();
	return (*cc)[ix++];
}
ContactContainer::pyIterator ContactContainer::pyIterator::iter(){ return *this; }

