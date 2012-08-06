// 2010 © Václav Šmilauer <eudoxos@arcig.cz>

#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>
#include<woo/pkg/dem/Clump.hpp>

#ifdef WOO_OPENMP
	#include<omp.h>
#endif

WOO_PLUGIN(dem,(ParticleContainer));


CREATE_LOGGER(ParticleContainer);
 
void ParticleContainer::clear(){
	parts.clear();
	#ifdef WOO_SUBDOMAINS
		subDomains.clear();
	#endif
}

Particle::id_t ParticleContainer::findFreeId(){
	long size=(long)parts.size();
	auto idI=freeIds.begin();
	while(!freeIds.empty()){
		id_t id=(*idI); assert(id>=0);
		idI=freeIds.erase(idI); // remove the current element, advances the iterator
		if(id<=size){
			if(parts[id]) throw std::logic_error("ParticleContainer::findFreeId: freeIds contained "+to_string(id)+", but it is occupied?!");
			return id;
		}
		// if the id is bigger than our size, it could be due to container shrinking, which is ok
	}
	return size; // all particles busy, past-the-end will cause resize
}

#ifdef WOO_SUBDOMAINS
	Particle::id_t ParticleContainer::findFreeDomainLocalId(int subDom){
		#ifdef WOO_OPENMP
			assert(subDom<(int)subDomains.size());
			id_t max=subDomains[subDom].size();
			// LOG_TRACE("subDom="<<subDom<<", max="<<max);
			id_t& low(subDomainsLowestFree[subDom]);
			for(; low<max; low++){
				if(!(bool)subDomains[subDom][low]) return low;
			}
			return subDomains[subDom].size();
		#else
			assert(false); // this function should never be called in OpenMP-less build
		#endif
	}
#endif

void ParticleContainer::insertAt(shared_ptr<Particle>& p, id_t id){
	assert(id>=0);
	if((size_t)id>=parts.size()){
		boost::mutex::scoped_lock lock(*manipMutex);
		parts.resize(id+1);
	}
	// can be an empty shared_ptr, check needed
	if(p) p->id=id;
	parts[id]=p;
	#ifdef WOO_SUBDOMAINS
		setParticleSubdomain(p,0); // add it to subdomain #0; only effective if subdomains are set up
	#endif
}

Particle::id_t ParticleContainer::insert(shared_ptr<Particle>& p){
	id_t id=findFreeId();
	insertAt(p,id);
	return id;
}

#ifdef WOO_SUBDOMAINS
	void ParticleContainer::clearSubdomains(){ subDomains.clear(); }
	void ParticleContainer::setupSubdomains(){ subDomains.clear(); subDomains.resize(maxSubdomains); }
	// put given parts to 
	bool ParticleContainer::setParticleSubdomain(const shared_ptr<Particle>& b, int subDom){
		#ifdef WOO_OPENMP
			assert(b==parts[b->id]); // consistency check
			// subdomains not used
			if(subDomains.empty()){ b->subDomId=Particle::ID_NONE; return false; }
			// there are subdomains, but it was requested to add the parts to one that does not exists
			// it is an error condition, since traversal of subdomains would siletly skip this parts
			// if(subDom>=(int)subDomains.size()) throw std::invalid_argument(("ParticleContainer::setParticleSubdomain: adding #"+lexical_cast<string>(b->id)+" to non-existent sub-domain "+lexical_cast<string>(subDom)).c_str());
			assert(subDom<(int)subDomains.size());
			id_t localId=findFreeDomainLocalId(subDom);
			if(localId>=(id_t)subDomains[subDom].size()) subDomains[subDom].resize(localId+1);
			subDomains[subDom][localId]=b;
			b->subDomId=domNumLocalId2subDomId(subDom,localId);
			return true;
		#else
			// no subdomains should exist on OpenMP-less builds, and particles should not have subDomId set either
			assert(subDomains.empty()); assert(b->subDomId==Particle::ID_NONE); return false;
		#endif
	}

#endif /* WOO_SUBDOMAINS */

const shared_ptr<Particle>& ParticleContainer::safeGet(Particle::id_t id){
	if(!exists(id)) throw std::invalid_argument("No such particle: #"+lexical_cast<string>(id)+".");
	return (*this)[id];
}


bool ParticleContainer::pyRemove(Particle::id_t id){
	if(!exists(id)) return false;
	dem->removeParticle(id);
	return true;
}

py::list ParticleContainer::pyRemoveList(vector<id_t> ids){
	py::list ret;
	for(auto& id: ids) ret.append(pyRemove(id));
	return ret;
}



bool ParticleContainer::remove(Particle::id_t id){
	if(!exists(id)) return false;
	// this is perhaps not necessary
	boost::mutex::scoped_lock lock(*manipMutex);
	freeIds.push_back(id);
	#ifdef WOO_SUBDOMAINS
		#ifdef WOO_OPENMP
			const shared_ptr<Particle>& b=parts[id];
			if(b->subDomId!=Particle::ID_NONE){
				int subDom, localId;
				std::tie(subDom,localId)=subDomId2domNumLocalId(b->subDomId);
				if(subDom<(int)subDomains.size() && localId<(id_t)subDomains[subDom].size() && subDomains[subDom][localId]==b){
					subDomainsLowestFree[subDom]=min(subDomainsLowestFree[subDom],localId);
				} else {
					LOG_FATAL("Particle::subDomId inconsistency detected for parts #"<<id<<" while erasing (cross thumbs)!");
					if(subDom>=(int)subDomains.size()){ LOG_FATAL("\tsubDomain="<<subDom<<" (max "<<subDomains.size()-1<<")"); }
					else if(localId>=(id_t)subDomains[subDom].size()){ LOG_FATAL("\tsubDomain="<<subDom<<", localId="<<localId<<" (max "<<subDomains[subDom].size()-1<<")"); }
					else if(subDomains[subDom][localId]!=b) { LOG_FATAL("\tsubDomains="<<subDom<<", localId="<<localId<<"; erasing #"<<id<<", subDomain record points to #"<<subDomains[subDom][localId]->id<<"."); }
				}
			}
		#else
			assert(parts[id]->subDomId==Particle::ID_NONE); // subDomId should never be defined for OpenMP-less builds
		#endif 
	#endif /* WOO_SUBDOMAINS */
	parts[id]=shared_ptr<Particle>();
	// removing last element, shrink the size as much as possible
	// extending the vector when the space is allocated is rather efficient
	if((size_t)(id+1)==parts.size()){
		while(!parts[id--]);
		parts.resize(id+2);
	}
	return true;
}


/* python access */
ParticleContainer::pyIterator::pyIterator(ParticleContainer* _pc): pc(_pc), ix(-1){}
shared_ptr<Particle> ParticleContainer::pyIterator::next(){
	int sz=pc->size();
	while(ix<sz){ ix++; if(pc->exists(ix)) return (*pc)[ix]; }
	woo::StopIteration();
	throw; // never reached, but makes the compiler happier
}
ParticleContainer::pyIterator ParticleContainer::pyIterator::iter(){ return *this; }

py::list ParticleContainer::pyFreeIds(){
	py::list ret;
	for(id_t id: freeIds) ret.append(id);
	return ret;
}

Particle::id_t ParticleContainer::pyAppend(shared_ptr<Particle> p){
	if(p->id>=0) IndexError("Particle already has id "+lexical_cast<string>(p->id)+" set; appending such particle (for the second time) is not allowed.");
	return insert(p);
}

py::list ParticleContainer::pyAppendList(vector<shared_ptr<Particle>> pp){
	py::list ret;
	FOREACH(shared_ptr<Particle>& p, pp){ret.append(pyAppend(p));}
	return ret;
}

shared_ptr<Node> ParticleContainer::pyAppendClumped(vector<shared_ptr<Particle>> pp, shared_ptr<Node> n){
	std::set<void*> seen;
	vector<shared_ptr<Node>> nodes; nodes.reserve(pp.size());
	vector<Particle::id_t> memberIds;
	for(const auto& p:pp){
		memberIds.push_back(pyAppend(p));
		for(const auto& n: p->shape->nodes){
			if(seen.count((void*)n.get())!=0) continue;
			seen.insert((void*)n.get());
			nodes.push_back(n);
		}
	}
	shared_ptr<Node> clump=ClumpData::makeClump(nodes,n);
	auto& cd=clump->getData<DemData>().cast<ClumpData>();
	cd.clumpLinIx=dem->clumps.size();
	cd.memberIds=memberIds;
	dem->clumps.push_back(clump);
	return clump;
}

shared_ptr<Particle> ParticleContainer::pyGetItem(Particle::id_t id){
	if(id<0 && id>=-(int)size()) id+=size();
	if(exists(id)) return (*this)[id];
	woo::IndexError("No such particle: #"+lexical_cast<string>(id)+".");
	return shared_ptr<Particle>(); // make compiler happy
}

ParticleContainer::pyIterator ParticleContainer::pyIter(){ return ParticleContainer::pyIterator(this); }

void ParticleContainer::pyRemask(vector<id_t> ids, int mask, bool visible, bool removeContacts, bool removeOverlapping){
	for(id_t id: ids){
		if(!exists(id)) woo::IndexError("No such particle: #"+to_string(id)+".");
		const auto& p=(*this)[id];
		p->mask=mask;
		p->shape->setVisible(visible);
		if(removeContacts){
			list<id_t> ids2;
			for(const auto& c: p->contacts){
				id_t id2(c.first);
				assert(exists(id2));
				if(!Collider::mayCollide(p,(*this)[id2],dem)) ids2.push_back(id2);
			}
			for(auto id2: ids2){ assert(dem->contacts->find(id,id2)); dem->contacts->remove(dem->contacts->find(id,id2)); }
		}
	}
	// traverse all other particles, check bbox overlaps
	if(removeOverlapping){
		list<id_t> toRemove;
		for(const auto& p2: *this){
			//if(!p2->shape || !p2->shape->bound){  cerr<<"-- #"<<p2->id<<" has no shape/bound."<<endl; continue; }
			AlignedBox3r b2(p2->shape->bound->min,p2->shape->bound->max);
			for(id_t id: ids){
				const auto& p=(*this)[id];
				//if(!p->shape || !p->shape->bound){ cerr<<"@@ #"<<id<<" has no shape/bound."<<endl; continue; }
				AlignedBox3r b1(p->shape->bound->min,p->shape->bound->max);
				//cerr<<"distance ##"<<id<<"+"<<p2->id<<" is "<<b1.exteriorDistance(b2)<<endl;
				if(b1.exteriorDistance(b2)<=0 && Collider::mayCollide(p2,p,dem)) toRemove.push_back(p2->id);
			}
		}
		for(auto id: toRemove){
			// cerr<<"Removing #"<<id<<endl;
			if(exists(id)) dem->removeParticle(id);
		}
		dem->contacts->dirty=true;
	}
}

