#include<woo/pkg/dem/ContactContainer.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Contact.hpp>

#ifdef WOO_OPENMP
	#include<omp.h>
#endif

WOO_IMPL_LOGGER(ContactContainer);
WOO_PLUGIN(dem,(ContactContainer));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ContactContainer__CLASS_BASE_DOC_ATTRS_PY);


bool ContactContainer::IsReal::operator()(shared_ptr<Contact>& c){ return c && c->isReal(); }
bool ContactContainer::IsReal::operator()(const shared_ptr<Contact>& c){ return c && c->isReal(); }

bool ContactContainer::add(const shared_ptr<Contact>& c, bool threadSafe){
	assert(dem);
	#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
		boost::mutex::scoped_lock lock(manipMutex);
	#endif
	Particle *pA=c->leakPA(), *pB=c->leakPB();
	if(!threadSafe){
		// make sure the contact does not exist yet
		assert(pA->contacts.find(pB->id)==pA->contacts.end());
		assert(pB->contacts.find(pA->id)==pB->contacts.end());
	} else {
		if(pA->contacts.find(pB->id)!=pA->contacts.end() || pB->contacts.find(pA->id)!=pB->contacts.end()) return false;
	}

	pA->contacts[pB->id]=c;
	pB->contacts[pA->id]=c;
	linView.push_back(c);
	// store the index back-reference in the interaction (so that it knows how to (re)move itself)
	c->linIx=linView.size()-1; 
	return true;
}

void ContactContainer::clear(){
	assert(dem);
	#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
		boost::mutex::scoped_lock lock(manipMutex);
	#endif
	FOREACH(const shared_ptr<Particle>& p, *dem->particles) p->contacts.clear();
	linView.clear(); // clear the linear container
	clearPending();
	dirty=true;
}

void ContactContainer::addMaybe_fast(const shared_ptr<Contact>& c){
	Particle *pA=c->leakPA(), *pB=c->leakPB();
	if((*dem->particles)[pA->id]->contacts.count(pB->id)==1) return;
	pA->contacts[pB->id]=c;
	pB->contacts[pA->id]=c;
	linView.push_back(c);
	c->linIx=linView.size()-1;
}

void ContactContainer::removeMaybe_fast(const shared_ptr<Contact>& c){
	Particle *pA=c->leakPA(), *pB=c->leakPB();
	const auto iA=pA->contacts.find(pB->id);
	if(iA==pA->contacts.end()) return;
	pA->contacts.erase(iA);
	pB->contacts.erase(pB->contacts.find(pA->id));
	linView_remove(c->linIx);
}

void ContactContainer::linView_remove(const size_t& ix){
	if(ix<linView.size()-1){ // is not the last element
		//cerr<<"linIx="<<ix<<"/"<<linView.size()<<endl;
		//if(!linView.back()) throw std::logic_error("linView.back it empty!");
		linView[ix]=linView.back(); // move the last to the place of the one being removed
		//if(ix<0) throw std::logic_error("##"+to_string(idA)+"+"+to_string(idB)+": linIx="+to_string(ix)+"?!");
		//if(!linView[ix]) throw std::logic_error("No contact at linIx="+to_string(ix));
		assert(linView[ix]);
		linView[ix]->linIx=ix; // and update its linIx
	}
	linView.resize(linView.size()-1);
}

bool ContactContainer::remove(shared_ptr<Contact> c, bool threadSafe){
	assert(dem);
	#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
		boost::mutex::scoped_lock lock(manipMutex);
	#endif

	// take ownership of particles
	shared_ptr<Particle> pA=c->pA.lock(), pB=c->pB.lock();
	for(const auto& p:{pA, pB}){
		if(!p) continue; // particle vanished in the meantime
		Particle::id_t id=p->id, id2=(p.get()==pA.get()?(pB?pB->id:-1):(pA?pA->id:-1));
		if(id2>=0){
			Particle::MapParticleContact::iterator iC=p->contacts.find(id2);
			// find either nothing or the right particle
			assert(iC==p->contacts.end() || iC->second.get()==c.get());
			if(iC==p->contacts.end()){
				if(!threadSafe){
					// particle was deleted from an engine, and had pending contact at the same time
					// check that the particle really is not in dem-particles
					if(((long)dem->particles->size()<=id2 || p.get()!=(*dem->particles)[id2].get())) return false;
					// if the particle is still there, but not the contact, we've a problem somewhere
					LOG_FATAL("Contact ##"<<id<<"+"<<id2<<" vanished from particle #"<<id<<"!");
					abort();
				}
				return false;
			}
			p->contacts.erase(iC); // remove contact from the particle's list
		} else {
			// damn, we have no idea what id2 is, what to do now?
			// if the contact is still there, it is serious
			if(linView.size()>c->linIx && linView[c->linIx].get()==c.get()){
				throw std::logic_error("Contact @ "+lexical_cast<string>(c)+": "+to_string(id)+" + ??; contact object still in Scene.dem.con["+to_string(c->linIx)+"]. Dropping to python so that you can inspect what's going on.");
			} else {
				// it seems the contact is not in DemField::contacts where it should be either
				// so let's assume it was taken care of by DemField::deleteParticle already
				return false;
			}
		}
	}
	// not even a single particle survived, this would not be caught in the loop above
	if(!pA && !pB){
		// contact still in contacts, that is error
		if(linView.size()>c->linIx && linView[c->linIx].get()==c.get()){
			throw std::logic_error("Contact @ "+lexical_cast<string>(c)+": ?? + ??; contact object still in Scene.dem.con["+to_string(c->linIx)+"]. Dropping to python so that you can inspect what's going on.");
		}
		// if not in contacts anymore, it is OK
		return false;
	}

	assert(linView.size()>c->linIx);
	assert(linView[c->linIx]==c);

	linView_remove(c->linIx);

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

bool ContactContainer::existsReal(ParticleContainer::id_t idA, ParticleContainer::id_t idB) const {
	const auto& c=find(idA,idB);
	return c && c->isReal();
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
		for(auto& pending: threadsPending){
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
		for(auto& pending: threadsPending){
	#endif
			if(!pending.empty()){
				for(const PendingContact& p: pending){ if(remove(p.contact)) ret++; }
				pending.clear();
			}
	#ifdef WOO_OPENMP
		}
	#endif
	return ret;
}

void ContactContainer::removeNonReal(){
	std::list<shared_ptr<Contact> > cc;
	for(const shared_ptr<Contact>& c: linView){ if(!c->isReal()) cc.push_back(c); }
	for(const shared_ptr<Contact>& c: cc){ this->remove(c); }
}

int ContactContainer::countReal() const{
	int ret=0;
	FOREACH(__attribute__((unused)) const shared_ptr<Contact>& c, *this) ret++;
	return ret;
};

Real ContactContainer::realRatio() const { return countReal()*1./size(); }


shared_ptr<Contact> ContactContainer::pyByIds(const Vector2i& ids){return find(ids[0],ids[1]); };

shared_ptr<Contact> ContactContainer::pyNth(int n){
	int sz(size());
	if(n<-sz || n>sz-1) IndexError("Linear index out of range ("+lexical_cast<string>(-sz)+".."+lexical_cast<string>(sz-1)+")");
	return linView[n>=0?n:sz+n];
}
ContactContainer::pyIterator ContactContainer::pyIter(){ return ContactContainer::pyIterator(this); }

ContactContainer::pyIterator::pyIterator(ContactContainer* _cc): cc(_cc), ix(0){}
shared_ptr<Contact> ContactContainer::pyIterator::next(){
	int sz=(int)cc->size();
	while(ix<sz && !(*cc)[ix]->isReal()) ix++;
	if(ix>=sz) woo::StopIteration();
	return (*cc)[ix++];
}
ContactContainer::pyIterator ContactContainer::pyIterator::iter(){ return *this; }

