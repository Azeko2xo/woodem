
#include<woo/lib/base/CompUtils.hpp>
#include<woo/lib/base/Volumetric.hpp>
#include<woo/core/Master.hpp> // for WOO_PYTHON_MODULE

WOO_PYTHON_MODULE(comp);
BOOST_PYTHON_MODULE(comp){
	WOO_SET_DOCSTRING_OPTS;
	py::def("tetraVolume",&woo::Volumetric::tetraVolume);
	py::def("tetraInertia",&woo::Volumetric::tetraInertia);
	py::def("triangleArea",&woo::Volumetric::triangleArea);
	py::def("triangleInertia",&woo::Volumetric::triangleInertia);
	py::def("inertiaTensorTranslate",&woo::Volumetric::inertiaTensorTranslate);
	py::def("cart2cyl",&CompUtils::cart2cyl,"Convert cartesian coordinates to cylindrical; cylindrical coordinates are specified as :math:`(r,\\theta,z)`, the reference plane is the :math:`xy`-plane (see `at Wikipedia <http://en.wikipedia.org/wiki/Cylindrical_coordinates>`__).");
	py::def("cyl2cart",&CompUtils::cyl2cart,"Convert cylindrical coordinates to cartesian; cylindrical coordinates are specified as :math:`(r,\\theta,z)`, the reference plane is the :math:`xy`-plane (see `at Wikipedia <http://en.wikipedia.org/wiki/Cylindrical_coordinates>`__).");
};
