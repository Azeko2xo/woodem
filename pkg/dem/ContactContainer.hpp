#pragma once
#include<woo/pkg/dem/ParticleContainer.hpp>

#include<boost/iterator/filter_iterator.hpp>


#ifdef WOO_OPENMP
	#include<omp.h>
#endif


struct DemField;
struct Contact;
struct Scene;

struct ContactContainer: public Object{
	DECLARE_LOGGER;
	/* internal data */
		// created in the ctor
		#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
			boost::mutex* manipMutex; // to synchronize with rendering, and between threads
		#endif
		DemField* dem; // backptr to DemField, set by DemField::postLoad; do not modify!
		ParticleContainer* particles;

		~ContactContainer(){
			#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
				delete manipMutex;
			#endif
		}

	/* basic functionality */
		bool add(const shared_ptr<Contact>& c, bool threadSafe=false);
		// copy of shared_ptr, so that the argument does not get deleted while being manipulated with
		bool remove(shared_ptr<Contact> c, bool threadSafe=false);

		shared_ptr<Contact> nullContactPtr; // ref to this will be returned if find finds nothing; such result may not be written to, but we want to avoid returning shared_ptr, since each find would change refcount
		const shared_ptr<Contact>& find(ParticleContainer::id_t idA, ParticleContainer::id_t idB) const;
		bool exists(ParticleContainer::id_t idA, ParticleContainer::id_t idB) const;
		shared_ptr<Contact>& operator[](size_t ix){return linView[ix];}
		const shared_ptr<Contact>& operator[](size_t ix) const { return linView[ix];}
		void clear();
		void removeNonReal();

	/* iteration */
		struct IsReal{
			bool operator()(shared_ptr<Contact>& p);
			bool operator()(const shared_ptr<Contact>& c);
		};
		typedef vector<shared_ptr<Contact> > ContainerT;
		typedef boost::filter_iterator<IsReal,ContainerT::iterator> iterator;
		typedef boost::filter_iterator<IsReal,ContainerT::const_iterator> const_iterator;

		iterator begin() { return iterator(linView.begin(),linView.end()); }
		iterator end() { return iterator(linView.end(),linView.end()); }
		const_iterator begin() const { return const_iterator(linView.begin(),linView.end()); }
		const_iterator end() const { return const_iterator(linView.end(),linView.end()); }
		size_t size() const { return linView.size(); }

	/* Handling pending contacts */
		struct PendingContact{
			shared_ptr<Contact> contact;
			bool force;
			template<class ArchiveT> void serialize(ArchiveT & ar, unsigned int version){ ar & BOOST_SERIALIZATION_NVP(contact); ar & BOOST_SERIALIZATION_NVP(force); }
		};

		// Ask for removing given contact (from the constitutive law); this resets the interaction (to the initial=potential state) and collider should traverse pending to decide whether to delete the interaction completely or keep it potential
		void requestRemoval(const shared_ptr<Contact>& c, bool force=false);

		int removeAllPending();
		void clearPending();
		int countReal() const;

		template<class T> int removePending(const T& t, Scene* scene){
			int ret=0;
			#ifdef WOO_OPENMP
				// shadow the this->pending by the local variable, to share the code
				FOREACH(list<PendingContact>& pending, threadsPending){
			#endif
					FOREACH(const PendingContact& p, pending){
						ret++;
						if(p.force || t.shouldBeRemoved(p.contact,scene)) remove(p.contact);
					}
					pending.clear();
			#ifdef WOO_OPENMP
				}
			#endif
			return ret;
		}

	/* python access */
		class pyIterator{
			const ContactContainer* cc; int ix;
		public:
			pyIterator(ContactContainer*);
			pyIterator iter();
			shared_ptr<Contact> next();
		};
		shared_ptr<Contact> pyByIds(const Vector2i& ids); // ParticleContainer::id_t id1, ParticleContainer::id_t id2);
		shared_ptr<Contact> pyNth(int n);
		pyIterator pyIter();


	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ContactContainer,Object,"Linear view on all contacts in the DEM field",
		((ContainerT,linView,,AttrTrait<Attr::hidden>(),"Linear storage of references; managed by accessor methods, do not modify directly!"))
		((bool,dirty,false,AttrTrait<Attr::hidden>(),"Flag for notifying the collider that persistent data should be invalidated"))
		((int,stepColliderLastRun,-1,AttrTrait<Attr::readonly>(),"Step number when a collider was last run; set by the collider, if it wants contacts that were not encoutered in that step to be deleted by ContactLoop (such as SpatialQuickSortCollider). Other colliders (such as InsertionSortCollider) set it it -1, which is the default."))
		#ifdef WOO_OPENMP
			((std::vector<std::list<PendingContact>>,threadsPending,std::vector<std::list<PendingContact>>(omp_get_max_threads()),AttrTrait<Attr::hidden>(),"Contacts which might be deleted by the collider in the next step (separate for each thread, for safe lock-free writes)"))
		#else
			((std::list<PendingContact>,pending,,AttrTrait<Attr::hidden>(),"Contact which might be deleted by the collider in the next step."))
		#endif

		, /*ctor*/
			#if defined(WOO_OPENMP) || defined(WOO_OPENGL)
				manipMutex=new boost::mutex;
			#endif
		, /* py */
		.def("clear",&ContactContainer::clear)
		.def("__len__",&ContactContainer::size)
		.def("__getitem__",&ContactContainer::pyByIds)
		.def("__getitem__",&ContactContainer::pyNth)
		.def("remove",&ContactContainer::requestRemoval,(py::arg("contact"),py::arg("force")=false))
		.def("removeNonReal",&ContactContainer::removeNonReal)
		.def("countReal",&ContactContainer::countReal)
		.def("exists",&ContactContainer::exists)
		.def("__iter__",&ContactContainer::pyIter)
		// define nested iterator class here; ugly, same as in ParticleContainer
		; boost::python::scope foo(_classObj);
		boost::python::class_<ContactContainer::pyIterator>("iterator",py::init<pyIterator>()).def("__iter__",&pyIterator::iter).def("next",&pyIterator::next);


	);
};
WOO_REGISTER_OBJECT(ContactContainer);
