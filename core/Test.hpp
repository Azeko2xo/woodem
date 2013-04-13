#pragma once
#include<woo/lib/object/Object.hpp>
#include<woo/lib/base/openmp-accu.hpp>
#include<woo/core/Engine.hpp>

namespace woo{
	struct WooTestClass: public woo::Object{
		#define _UNIT_ATTR(unitName) ((Real,unitName,0.,AttrTrait<>().unitName ## Unit(),"Variable with " #unitName "unit."))
		py::list aaccuGetRaw();
		void aaccuSetRaw(const vector<Real>& r);
		void aaccuWriteThreads(size_t ix, const vector<Real>& cycleData);
		enum { POSTLOAD_NONE=-1,POSTLOAD_CTOR=0, POSTLOAD_FOO=1, POSTLOAD_BAR=2, POSTLOAD_BAZ=3 };
		void postLoad(WooTestClass&, void* addr){
			if(addr==NULL){ postLoadStage=POSTLOAD_CTOR; return; }
			if(addr==(void*)&foo_incBaz){ baz++; postLoadStage=POSTLOAD_FOO; return; }
			if(addr==(void*)&bar_zeroBaz){ baz=0; postLoadStage=POSTLOAD_BAR; return; }
			if(addr==(void*)&baz){ postLoadStage=POSTLOAD_BAZ; return; }
		}
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

			((int,hiddenAttribute,0,AttrTrait<Attr::hidden>(),"Hidden data member (not accessible from python)"))
			((int,meaning42,42,AttrTrait<Attr::readonly>(),"Read-only data member"))
			((int,foo_incBaz,0,AttrTrait<Attr::triggerPostLoad>(),"Change this attribute to have baz incremented"))
			((int,bar_zeroBaz,0,AttrTrait<Attr::triggerPostLoad>(),"Change this attribute to have baz incremented"))
			((int,baz,0,AttrTrait<Attr::triggerPostLoad>(),"Value which is changed when assigning to foo_incBaz / bar_zeroBaz."))
			((int,postLoadStage,(int)POSTLOAD_NONE,AttrTrait<Attr::readonly>(),"Store the last stage from postLoad (to check it is called the right way)"))

			,/*ctor*/
			,/*py*/
				.add_property("aaccuRaw",&WooTestClass::aaccuGetRaw,&WooTestClass::aaccuSetRaw,"Access OpenMPArrayAccumulator data directly. Writing resizes and sets the 0th thread value, resetting all other ones.")
				.def("aaccuWriteThreads",&WooTestClass::aaccuWriteThreads,(py::arg("index"),py::arg("cycleData")),"Assign a single line in the array accumulator, assigning number from *cycleData* in parallel in each thread.")
				;
				_classObj.attr("postLoad_none")=(int)POSTLOAD_NONE;
				_classObj.attr("postLoad_ctor")=(int)POSTLOAD_CTOR;
				_classObj.attr("postLoad_foo")=(int)POSTLOAD_FOO;
				_classObj.attr("postLoad_bar")=(int)POSTLOAD_BAR;
				_classObj.attr("postLoad_baz")=(int)POSTLOAD_BAZ;

		);
		#undef _UNIT_ATTR
	};

	struct WooTestPeriodicEngine: public PeriodicEngine{
		void notifyDead(){ deadCounter++; }
		WOO_CLASS_BASE_DOC_ATTRS(WooTestPeriodicEngine,PeriodicEngine,"Test some PeriodicEngine features.",
			((int,deadCounter,0,,"Count how many times :obj:`dead` was assigned to."))
		);
	};
};
using woo::WooTestClass;
using woo::WooTestPeriodicEngine;
REGISTER_SERIALIZABLE(WooTestClass);
REGISTER_SERIALIZABLE(WooTestPeriodicEngine);

