// 2010 ©  Václav Šmilauer <eudoxos@arcig.cz>

#pragma once

#include<locale>
#include<boost/archive/codecvt_null.hpp>
#include<boost/iostreams/filtering_stream.hpp>
#include<boost/iostreams/filter/bzip2.hpp>
#include<boost/iostreams/filter/gzip.hpp>
#include<boost/iostreams/device/file.hpp>
#include<boost/algorithm/string.hpp>
#include<boost/version.hpp>
#if BOOST_VERSION<104800
	#error You need boost >= 1.48 for boost/math/special_functions/nonfinite_num_facets.hpp . Woo does not provide its own copy since rev. 3048, you might need to revert back, or upgrade your boost installation. [boost version should have been checked by the configure script already?]
#else
	#include<boost/math/special_functions/nonfinite_num_facets.hpp>
#endif

// this used to be feature given to scons
#ifndef WOO_XMLSERIALIZATION
	#define WOO_XMLSERIALIZATION
#endif


namespace woo{
/* Utility template functions for (de)serializing objects using boost::serialization from/to streams or files.

	Includes boost::math::nonfinite_num_{put,get} for gracefully handling nan's and inf's.
*/
struct ObjectIO{
	// tell whether given filename looks like XML
	static bool isXmlFilename(const std::string f){
		return boost::algorithm::ends_with(f,".xml") || boost::algorithm::ends_with(f,".xml.bz2") || boost::algorithm::ends_with(f,".xml.gz");
	}
	// save to given stream and archive format
	template<class T, class oarchive>
	static void save(std::ostream& ofs, const string& objectTag, T& object){
		std::locale default_locale(std::locale::classic(), new boost::archive::codecvt_null<char>);
		std::locale locale2(default_locale, new boost::math::nonfinite_num_put<char>);
		ofs.imbue(locale2);
		oarchive oa(ofs,boost::archive::no_codecvt);
		oa << boost::serialization::make_nvp(objectTag.c_str(),object);
		ofs.flush();
	}
	// load from given stream and archive format
	template<class T, class iarchive>
	static void load(std::istream& ifs, const string& objectTag, T& object){
		std::locale default_locale(std::locale::classic(), new boost::archive::codecvt_null<char>);
		std::locale locale2(default_locale, new boost::math::nonfinite_num_get<char>);
		ifs.imbue(locale2);
		iarchive ia(ifs,boost::archive::no_codecvt);
		ia >> boost::serialization::make_nvp(objectTag.c_str(),object);
	}
	// save to given file, guessing compression and XML/binary from extension
	template<class T>
	static void save(const string fileName, const string& objectTag, T& object){
		boost::iostreams::filtering_ostream out;
		if(boost::algorithm::ends_with(fileName,".bz2")) out.push(boost::iostreams::bzip2_compressor());
		if(boost::algorithm::ends_with(fileName,".gz")) out.push(boost::iostreams::gzip_compressor());
		boost::iostreams::file_sink outSink(fileName,std::ios_base::out|std::ios_base::binary);
		if(!outSink.is_open()) throw std::runtime_error("Error opening file "+fileName+" for writing.");
		out.push(outSink);
		if(!out.good()) throw std::logic_error("boost::iostreams::filtering_ostream.good() failed (but "+fileName+" is open for writing)?");
		if(isXmlFilename(fileName)){
			#ifdef WOO_XMLSERIALIZATION
				save<T,boost::archive::xml_oarchive>(out,objectTag,object);
			#else
				throw std::runtime_error("Serialization to XML is not supported in this build of Yade (enable the 'xmlserialization' feature).");
			#endif
		}
		else save<T,boost::archive::binary_oarchive>(out,objectTag,object);
	}
	// load from given file, guessing compression and XML/binary from extension
	template<class T>
	static void load(const string& fileName, const string& objectTag, T& object){
		boost::iostreams::filtering_istream in;
		if(boost::algorithm::ends_with(fileName,".bz2")) in.push(boost::iostreams::bzip2_decompressor());
		if(boost::algorithm::ends_with(fileName,".gz")) in.push(boost::iostreams::gzip_decompressor());
		boost::iostreams::file_source inSource(fileName,std::ios_base::in|std::ios_base::binary);
		if(!inSource.is_open()) throw std::runtime_error("Error opening file "+fileName+" for reading.");
		in.push(inSource);
		if(!in.good()) throw std::logic_error("boost::iostreams::filtering_istream::good() failed (but "+fileName+" is open for reading)?");
		if(isXmlFilename(fileName)){
			#ifdef WOO_XMLSERIALIZATION
				load<T,boost::archive::xml_iarchive>(in,objectTag,object);
			#else
				throw std::runtime_error("De-serialization from XML is not supported in this build of Yade (enable the 'xmlserialization' feature).");
			#endif
		}
		else load<T,boost::archive::binary_iarchive>(in,objectTag,object);
	}
};

}
