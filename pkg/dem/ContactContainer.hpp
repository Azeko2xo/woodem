#pragma once
#include<yade/pkg/dem/ParticleContainer.hpp>

#include<boost/iterator/filter_iterator.hpp>


#ifdef YADE_OPENMP
	#include<omp.h>
#endif


class DemField;
class Contact;
class Scene;

struct ContactContainer: public Serializable{
	/* internal data */
		// created in the ctor
		#if defined(YADE_OPENMP) || defined(YADE_OPENGL)
			boost::mutex* manipMutex; // to synchronize with rendering, and between threads
		#endif
		DemField* dem; // backptr to DemField, set by DemField::postLoad; do not modify!
		ParticleContainer* particles;

		~ContactContainer(){ delete manipMutex; }

	/* basic functionality */
		bool add(const shared_ptr<Contact>& c, bool threadSafe=false);
		bool remove(const shared_ptr<Contact>& c, bool threadSafe=false);
		shared_ptr<Contact> find(ParticleContainer::id_t idA, ParticleContainer::id_t idB);
		bool exists(ParticleContainer::id_t idA, ParticleContainer::id_t idB);
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
		struct PendingContact{ shared_ptr<Contact> contact; bool force; };
		#ifdef YADE_OPENMP
			std::vector<std::list<PendingContact> > threadsPending;
		#endif
		std::list<PendingContact> pending;

		// Ask for removing given contact (from the constitutive law); this resets the interaction (to the initial=potential state) and collider should traverse pending to decide whether to delete the interaction completely or keep it potential
		void requestRemoval(const shared_ptr<Contact>& c, bool force=false);

		int removeAllPending();
		void clearPending();
		int countReal() const;

		template<class T> int removePending(const T& t, Scene* scene){
			int ret=0;
			#ifdef YADE_OPENMP
				// shadow the this->pending by the local variable, to share the code
				FOREACH(list<PendingContact>& pending, threadsPending){
			#endif
					FOREACH(const PendingContact& p, pending){
						ret++;
						if(p.force || t.shouldBeRemoved(p.contact,scene)) remove(p.contact);
					}
					pending.clear();
			#ifdef YADE_OPENMP
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


	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(ContactContainer,Serializable,"Linear view on all contacts in the DEM field",
		((ContainerT,linView,,Attr::hidden,"Linear storage of references; managed by accessor methods, do not modify directly!"))
		((bool,dirty,false,Attr::hidden,"Flag for notifying the collider that persistent data should be invalidated"))
		((int,stepColliderLastRun,-1,Attr::readonly,"Step number when a collider was last run; set by the collider, if it wants contacts that were not encoutered in that step to be deleted by ContactLoop (such as SpatialQuickSortCollider). Other colliders (such as InsertionSortCollider) set it it -1, which is the default."))

		, /*ctor*/
			#ifdef YADE_OPENMP
				threadsPending.resize(omp_get_max_threads());
			#endif
			manipMutex=new boost::mutex;
		, /* py */
		.def("clear",&ContactContainer::clear)
		.def("__len__",&ContactContainer::size)
		.def("__getitem__",&ContactContainer::pyByIds)
		.def("__getitem__",&ContactContainer::pyNth)
		.def("remove",&ContactContainer::requestRemoval,(py::arg("contact"),py::arg("force")=false))
		.def("removeNonReal",&ContactContainer::removeNonReal)
		.def("countReal",&ContactContainer::countReal)
		.def("__iter__",&ContactContainer::pyIter)
		// define nested iterator class here; ugly, same as in ParticleContainer
		; boost::python::scope foo(_classObj);
		boost::python::class_<ContactContainer::pyIterator>("iterator",py::init<pyIterator>()).def("__iter__",&pyIterator::iter).def("next",&pyIterator::next);


	);
};
REGISTER_SERIALIZABLE(ContactContainer);

