// this file has the .cxx extension since we compile it separately, not
// matching the py/*.cpp glob in the build system
#include<woo/core/Master.hpp>
#include<boost/algorithm/string/predicate.hpp>
#include<boost/preprocessor.hpp>

static const string version(BOOST_PP_STRINGIZE(WOO_VERSION));
static const string revision(BOOST_PP_STRINGIZE(WOO_REVISION));

static string prettyVersion(bool lead=true){
	if(!version.empty()) return (lead?"ver. ":"")+version;
	if(boost::starts_with(revision,"bzr")) return (lead?"rev. ":"")+revision.substr(3);
	return version+"/"+revision;
}

WOO_PYTHON_MODULE(config)
BOOST_PYTHON_MODULE(config){
	py::list features;
		#ifdef WOO_LOG4CXX
			features.append("log4cxx");
		#endif
		#ifdef WOO_OPENMP
			features.append("openmp");
		#endif
		#ifdef WOO_OPENGL
			features.append("opengl");
		#endif
		#ifdef WOO_CGAL
			features.append("cgal");
		#endif
		#ifdef WOO_OPENCL
			features.append("opencl");
		#endif
		#ifdef WOO_GTS
			features.append("gts");
		#endif
		#ifdef WOO_VTK
			features.append("vtk");
		#endif
		#ifdef WOO_GL2PS
			features.append("gl2ps");
		#endif
		#ifdef WOO_QT4
			features.append("qt4");
		#endif
		#ifdef WOO_CLDEM
			features.append("cldem");
		#endif
		#ifdef WOO_NOXML
			features.append("noxml");
		#endif

	py::scope().attr("features")=features;

	py::scope().attr("debug")=
	#ifdef WOO_DEBUG
		true;
	#else
		false;
	#endif

	py::scope().attr("prefix")=BOOST_PP_STRINGIZE(WOO_PREFIX);
	py::scope().attr("suffix")=BOOST_PP_STRINGIZE(WOO_SUFFIX);

	py::scope().attr("revision")=BOOST_PP_STRINGIZE(WOO_REVISION);
	py::scope().attr("version")=BOOST_PP_STRINGIZE(WOO_VERSION);
	py::def("prettyVersion",&prettyVersion,(py::arg("lead")=true));

	py::scope().attr("sourceRoot")=BOOST_PP_STRINGIZE(WOO_SOURCE_ROOT);
	py::scope().attr("buildRoot")=BOOST_PP_STRINGIZE(WOO_BUILD_ROOT);
	py::scope().attr("flavor")=BOOST_PP_STRINGIZE(WOO_FLAVOR);
	py::scope().attr("buildDate")=__DATE__;

};
