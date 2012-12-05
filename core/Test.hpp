#pragma once
#include<woo/lib/object/Object.hpp>

namespace woo{
	struct WooTestClass: public woo::Object{
		#define _UNIT_ATTR(unitName) ((Real,unitName,0.,AttrTrait<>().unitName ## Unit(),"Variable with " #unitName "unit."))
		WOO_CLASS_BASE_DOC_ATTRS(WooTestClass,Object,"This class serves to test various functionalities; it also includes all possible units, which thus get registered in woo",
			/* enumerate all units here */
			// ((Real,angle,0.,AttrTrait<>().angleUnit(),""))
			_UNIT_ATTR(angle)
			_UNIT_ATTR(time)
			_UNIT_ATTR(len)
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
		);
		#undef _UNIT_ATTR
	};
};
using woo::WooTestClass;
REGISTER_SERIALIZABLE(WooTestClass);

