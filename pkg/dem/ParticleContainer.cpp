// 2010 © Václav Šmilauer <eudoxos@arcig.cz>

#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/Clump.hpp>

#ifdef YADE_OPENMP
	#include<omp.h>
#endif

YADE_PLUGIN(dem,(ParticleContainer));


CREATE_LOGGER(ParticleContainer);
 
void ParticleContainer::clear(){
	parts.clear(); lowestFree=0;
	#ifdef YADE_SUBDOMAINS
		subDomains.clear();
	#endif
}

Particle::id_t ParticleContainer::findFreeId(){
	id_t max=parts.size();
	for(; lowestFree<max; lowestFree++){
		if(!(bool)parts[lowestFree]) return lowestFree;
	}
	return parts.size();
}

#ifdef YADE_SUBDOMAINS
	Particle::id_t ParticleContainer::findFreeDomainLocalId(int subDom){
		#ifdef YADE_OPENMP
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
	if((size_t)id>=parts.size()) parts.resize(id+1);
	// can be an empty shared_ptr, check needed
	if(p) p->id=id;
	parts[id]=p;
	#ifdef YADE_SUBDOMAINS
		setParticleSubdomain(p,0); // add it to subdomain #0; only effective if subdomains are set up
	#endif
}

Particle::id_t ParticleContainer::insert(shared_ptr<Particle>& p){
	id_t id=findFreeId();
	insertAt(p,id);
	return id;
}

#ifdef YADE_SUBDOMAINS
	void ParticleContainer::clearSubdomains(){ subDomains.clear(); }
	void ParticleContainer::setupSubdomains(){ subDomains.clear(); subDomains.resize(maxSubdomains); }
	// put given parts to 
	bool ParticleContainer::setParticleSubdomain(const shared_ptr<Particle>& b, int subDom){
		#ifdef YADE_OPENMP
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

#endif /* YADE_SUBDOMAINS */

const shared_ptr<Particle>& ParticleContainer::safeGet(Particle::id_t id){
	if(!exists(id)) throw std::invalid_argument("No such particle: #"+lexical_cast<string>(id)+".");
	return (*this)[id];
}



bool ParticleContainer::remove(Particle::id_t id){
	if(!exists(id)) return false;
	lowestFree=min(lowestFree,id);
	#ifdef YADE_SUBDOMAINS
		#ifdef YADE_OPENMP
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
	#endif /* YADE_SUBDOMAINS */
	parts[id]=shared_ptr<Particle>();
	return true;
}


/* python access */
ParticleContainer::pyIterator::pyIterator(ParticleContainer* _pc): pc(_pc), ix(-1){}
shared_ptr<Particle> ParticleContainer::pyIterator::next(){
	int sz=pc->size();
	while(ix<sz){ ix++; if(pc->exists(ix)) return (*pc)[ix]; }
	yade::StopIteration();
	throw; // never reached, but makes the compiler happier
}
ParticleContainer::pyIterator ParticleContainer::pyIterator::iter(){ return *this; }

Particle::id_t ParticleContainer::pyAppend(shared_ptr<Particle> p){
	if(p->id>=0) IndexError("Particle already has id "+lexical_cast<string>(p->id)+" set; appending such particle (for the second time) is not allowed.");
	return insert(p);
}

py::list ParticleContainer::pyAppendList(vector<shared_ptr<Particle>> pp){
	py::list ret;
	FOREACH(shared_ptr<Particle>& p, pp){ret.append(pyAppend(p));}
	return ret;
}

shared_ptr<Node> ParticleContainer::pyAppendClumped(vector<shared_ptr<Particle>> pp){
	std::set<void*> seen;
	vector<shared_ptr<Node>> nodes; nodes.reserve(pp.size());
	for(const auto& p:pp){
		pyAppend(p);
		for(const auto& n: p->shape->nodes){
			if(seen.count((void*)n.get())!=0) continue;
			seen.insert((void*)n.get());
			nodes.push_back(n);
		}
	}
	shared_ptr<Node> clump=ClumpData::makeClump(nodes);
	dem->clumps.push_back(clump);
	return clump;
}

shared_ptr<Particle> ParticleContainer::pyGetItem(Particle::id_t id){
	if(id<0 && id>=-(int)size()) id+=size();
	if(exists(id)) return (*this)[id];
	yade::IndexError("No such particle: #"+lexical_cast<string>(id)+".");
	return shared_ptr<Particle>(); // make compiler happy
}

ParticleContainer::pyIterator ParticleContainer::pyIter(){ return ParticleContainer::pyIterator(this); }
