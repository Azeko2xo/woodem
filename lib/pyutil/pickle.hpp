#pragma once

#include<woo/lib/base/Types.hpp>
#include<woo/lib/base/Math.hpp>
#include<boost/serialization/nvp.hpp>

namespace woo{
	class Pickler{
		static bool initialized;
		static py::object cPickle_dumps;
		static py::object cPickle_loads;
		static void ensureInitialized();
		public:
			static std::string dumps(py::object o);
			static py::object loads(const std::string&);
	};
}

namespace boost{ namespace serialization{
	template<class Archive>
	void serialize(Archive & ar, py::object& obj, const unsigned int version){
		std::string pickled;
		if(!Archive::is_loading::value) pickled=woo::Pickler::dumps(obj);
		ar & BOOST_SERIALIZATION_NVP(pickled);
		if(Archive::is_loading::value) obj=woo::Pickler::loads(pickled);
	};
	#define SERIALIZE_PY_TYPE(PY_TYPE) \
		template<class Archive> void serialize(Archive & ar, PY_TYPE& obj, const unsigned int version){ \
			std::string pickled; \
			if(!Archive::is_loading::value) pickled=woo::Pickler::dumps(py::object(obj)); \
			ar & BOOST_SERIALIZATION_NVP(pickled); \
			if(Archive::is_loading::value) obj=py::extract<PY_TYPE>(woo::Pickler::loads(pickled)); \
		};
	SERIALIZE_PY_TYPE(py::dict);
	SERIALIZE_PY_TYPE(py::tuple);
	SERIALIZE_PY_TYPE(py::list);
	// add other types here
}}
