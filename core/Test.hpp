#pragma once
#include<woo/lib/object/Object.hpp>
#include<woo/lib/base/openmp-accu.hpp>

namespace woo{
	struct WooTestClass: public woo::Object{
		#define _UNIT_ATTR(unitName) ((Real,unitName,0.,AttrTrait<>().unitName ## Unit(),"Variable with " #unitName "unit."))
		py::list aaccuGetRaw();
		void aaccuSetRaw(const vector<Real>& r);
		void aaccuWriteThreads(size_t ix, const vector<Real>& cycleData);
		WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(WooTestClass,Object,"This class serves to test various functionalities; it also includes all possible units, which thus get registered in woo",
			/* enumerate all units here */
			// ((Real,angle,0.,AttrTrait<>().angleUnit(),""))
			_UNIT_ATTR(angle)
			_UNIT_ATTR(time)
			_UNIT_ATTR(len)
			((Real,length_with_inches,0.,AttrTrait<>().lenUnit().altUnits({{"in",1/0.0254}}),"Variable with length, but also showing inches in the UI"))
			_UNIT_ATTR(area)
			_UNIT_ATTR(vol)
			_UNIT_ATTR(vel)
			_UNIT_ATTR(accel)
			_UNIT_ATTR(mass)
			_UNIT_ATTR(angVel)
			_UNIT_ATTR(angMom)
			_UNIT_ATTR(inertia)
			_UNIT_ATTR(force)
			_UNIT_ATTR(torque)
			_UNIT_ATTR(pressure)
			_UNIT_ATTR(stiffness)
			_UNIT_ATTR(massFlowRate)
			_UNIT_ATTR(density)
			_UNIT_ATTR(fraction)
			((OpenMPArrayAccumulator<Real>,aaccu,,AttrTrait<>().noGui(),"Test openmp array accumulator"))

			,/*ctor*/
			,/*py*/
				.add_property("aaccuRaw",&WooTestClass::aaccuGetRaw,&WooTestClass::aaccuSetRaw,"Access OpenMPArrayAccumulator data directly. Writing resizes and sets the 0th thread value, resetting all other ones.")
				.def("aaccuWriteThreads",&WooTestClass::aaccuWriteThreads,(py::arg("index"),py::arg("cycleData")),"Assign a single line in the array accumulator, assigning number from *cycleData* in parallel in each thread.")
		);
		#undef _UNIT_ATTR
	};
};
using woo::WooTestClass;
REGISTER_SERIALIZABLE(WooTestClass);

