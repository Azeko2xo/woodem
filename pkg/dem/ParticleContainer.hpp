// 2010 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include<yade/lib/serialization/Serializable.hpp>
#include<yade/core/Field.hpp>

#include<boost/foreach.hpp>
#ifndef FOREACH
#  define FOREACH BOOST_FOREACH
#endif

#include<boost/iterator/filter_iterator.hpp>

#ifdef YADE_SUBDOMAINS
	#ifdef YADE_OPENMP
		#define YADE_PARALLEL_FOREACH_PARTICLE_BEGIN(b,particles) \
			const int _numSubdomains=particles->numSubdomains(); \
			_Pragma("omp parallel for schedule(static,1)") \
			for(int _subDomain=0; _subDomain<_numSubdomains; _subDomain++) \
			FOREACH(b, particles->getSubdomain(_subDomain)){
		#define YADE_PARALLEL_FOREACH_PARTICLE_END() }
	#else
		#define YADE_PARALLEL_FOREACH_PARTICLE_BEGIN(b,particles) \
			assert(particles->numSubdomains()==1); assert(particles->getSubdomain(0).size()==particles->size()); \
			FOREACH(b,*(particles)){
		#define YADE_PARALLEL_FOREACH_PARTICLE_END() }
	#endif
#else
	#if YADE_OPENMP
		#define YADE_PARALLEL_FOREACH_PARTICLE_BEGIN(b_,particles) const id_t _sz(particles->size()); _Pragma("omp parallel for") for(id_t _id=0; _id<_sz; _id++){ b_((*particles)[_id]);
		#define YADE_PARALLEL_FOREACH_PARTICLE_END() }
	#else
		#define YADE_PARALLEL_FOREACH_PARTICLE_BEGIN(b,particles) FOREACH(b,*(particles)){
		#define YADE_PARALLEL_FOREACH_PARTICLE_END() }
	#endif
#endif

namespace py=boost::python;

class Particle;
class DemField;

/*
Container of particles implemented as flat std::vector. It handles parts removal and
intelligently reallocates free ids for newly added ones.
*/
struct ParticleContainer: public Serializable{
	DemField* dem; // backptr to DemField, set by DemField::postLoad; do not modify!
	typedef int id_t;

	boost::mutex* manipMutex; // to synchronize with rendering, and between threads

	private:
		typedef std::vector<shared_ptr<Particle> > ContainerT;
		// ContainerT parts;
		id_t lowestFree;
		id_t findFreeId();
		#ifdef YADE_SUBDOMAINS
			id_t findFreeDomainLocalId(int subDom);
			// subDomain data
			std::vector<std::vector<shared_ptr<Particle> > > subDomains;
		#endif
	public:
		~ParticleContainer(){ delete manipMutex; }

		struct IsExisting{
			bool operator()(shared_ptr<Particle>& p){ return (bool)p;} 
			bool operator()(const shared_ptr<Particle>& p){ return (bool)p;} 
		};
		// friend class InteractionContainer;  // accesses the parts vector directly

		//typedef ContainerT::iterator iterator;
		//typedef ContainerT::const_iterator const_iterator;
		typedef boost::filter_iterator<IsExisting,ContainerT::iterator> iterator;
		typedef boost::filter_iterator<IsExisting,ContainerT::const_iterator> const_iterator;

		id_t insert(shared_ptr<Particle>&);
		void insertAt(shared_ptr<Particle>& p, id_t id);

	
		// mimick some STL api
		void clear();
		iterator begin() { return iterator(parts.begin(),parts.end()); }
		iterator end() { return iterator(parts.end(),parts.end()); }
		const_iterator begin() const { return const_iterator(parts.begin(),parts.end()); }
		const_iterator end() const { return const_iterator(parts.end(),parts.end()); }

		size_t size() const { return parts.size(); }
		shared_ptr<Particle>& operator[](id_t id){ return parts[id];}
		const shared_ptr<Particle>& operator[](id_t id) const { return parts[id]; }
		const shared_ptr<Particle>& safeGet(id_t id);

		bool exists(id_t id) const { return (id>=0) && ((size_t)id<parts.size()) && ((bool)parts[id]); }
		bool remove(id_t id);
		

		#ifdef YADE_SUBDOMAINS
			/* subdomain support
				Only meaningful when using OpenMP, but we keep the interface for OpenMP-less build as well.
				Loop over all particles can be achieved as shown below; this works for all possible scenarios:

				1. OpenMP-less build
				2. OpenMP build, but no sub-domains defined (the loop will be then sequential, over the global domain)
				3. OpenMP build, with sub-domains; each thread will handle one sub-domain
				
				#ifdef YADE_OPENMP
					#pragma omp parallel for schedule(static)
				#endif
				for(int subDom=0; subDom<scene->particles->numSubdomains(); subDom++){
					FOREACH(const shared_ptr<Particle>& b, scene->particles->getSubdomain(subDom)){
						if(!b) continue;
					}
				}

				For convenience, this is all wrapped in YADE_PARALLEL_FOREACH_BODY macro, which then works like this:

				YADE_PARALLEL_FOREACH_BODY(const shared_ptr<Particle>& b,scene->particles){
					if(!b) continue

				}
			
			*/
			// initialized by the ctor, is omp_get_max_threads() for OpenMP builds and 1 for OpenMP-less builds
			const int maxSubdomains;
			private:
				// lowest free id for each sub-domain
				std::vector<id_t> subDomainsLowestFree;
			public:
			// return subdomain with given number; if no subDomains are defined (which is always the case for OpenMP-less builds), return the whole domain
			const std::vector<shared_ptr<Particle> >& getSubdomain(int subDom){ if(subDomains.empty()){ assert(subDom==0); return parts; } assert((size_t)subDom<subDomains.size()); return subDomains[subDom]; }
			// return the number of actually defined subdomains; if no subdomains were defined, return 1, which has the meaning of the whole domain
			int numSubdomains(){ return std::max((size_t)1,subDomains.size()); }
			// convert global subId to subDomain number and domain-local id
			std::tuple<int,id_t> subDomId2domNumLocalId(id_t subDomId) const { return std::make_tuple(subDomId % maxSubdomains, subDomId / maxSubdomains); }
			// convert subDomain number and subDomain-local id to global subId
			id_t domNumLocalId2subDomId(int domNum,id_t localId) const { return localId*maxSubdomains+domNum; }

			// delete all subdomain data
			void clearSubdomains();
			// setup empty subdomains
			void setupSubdomains();
			// add parts to a sub-domain; return false if there are no subdomains, true otherwise
			bool setParticleSubdomain(const shared_ptr<Particle>&, int subDom);
		#endif /* YADE_SUBDOMAINS */

		// python access
		class pyIterator{
			const ParticleContainer* pc; int ix;
		public:
			pyIterator(ParticleContainer*);
			pyIterator iter();
			shared_ptr<Particle> next();
		};
		id_t pyAppend(shared_ptr<Particle>);
		shared_ptr<Node> pyAppendClumped(vector<shared_ptr<Particle>>);
		py::list pyAppendList(vector<shared_ptr<Particle>>);
		bool pyRemove(id_t id);
		shared_ptr<Particle> pyGetItem(id_t id);
		size_t pyLen();
		pyIterator pyIter();
	

		YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(ParticleContainer,Serializable,"Storage for DEM particles",
			((ContainerT/* = std::vector<shared_ptr<Particle> > */,parts,,Attr::hidden,"Actual particle storage"))
			,/* init */ ((lowestFree,0))
				#ifdef YADE_SUBDOMAINS
					#ifdef YADE_OPENMP
						((maxSubdomains,omp_get_max_threads()))
					#else
						((maxSubdomains,1))
					#endif
					((subDomainsLowestFree,vector<id_t>(maxSubdomains,0)))
				#endif /* YADE_SUBDOMAINS */
			,/* ctor */
				manipMutex=new boost::mutex;
			,/*py*/
			.def("append",&ParticleContainer::pyAppend) /* wrapper chacks if the id is not already assigned */
			.def("append",&ParticleContainer::pyAppendList)
			.def("appendClumped",&ParticleContainer::pyAppendClumped,"Add particles as rigid aggregate. Add resulting clump node (which is *not* a particle) to O.dem.clumps, subject to integration.")
			.def("remove",&ParticleContainer::remove) /* no wrapping needed */
			.def("exists",&ParticleContainer::exists)
			.def("__getitem__",&ParticleContainer::pyGetItem)
			.def("__len__",&ParticleContainer::size)
			.def("clear",&ParticleContainer::clear)
			.def("__iter__",&ParticleContainer::pyIter)
			// define nested iterator class here; ugly: abuses _classObj from the macro definition (implementation detail)
			; boost::python::scope foo(_classObj);
			boost::python::class_<ParticleContainer::pyIterator>("iterator",py::init<pyIterator>()).def("__iter__",&pyIterator::iter).def("next",&pyIterator::next);
		);
		DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(ParticleContainer);
