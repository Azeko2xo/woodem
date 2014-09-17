#pragma once
#include<woo/lib/object/Object.hpp>
#include<woo/lib/base/openmp-accu.hpp>
#include<woo/core/Engine.hpp>

namespace woo{
	struct WooTestClass: public woo::Object{
		// must be #defined in the .cpp file as well as the macro gets expanded there
		// so, don't #undef it below
		#define _WOO_UNIT_ATTR(unitName) ((Real,unitName,0.,AttrTrait<>().unitName ## Unit(),"Variable with " #unitName " unit."))
		py::list aaccuGetRaw();
		void aaccuSetRaw(const vector<Real>& r);
		void aaccuWriteThreads(size_t ix, const vector<Real>& cycleData);
		enum { POSTLOAD_NONE=-1,POSTLOAD_CTOR=0, POSTLOAD_FOO=1, POSTLOAD_BAR=2, POSTLOAD_BAZ=3 };
		enum { NAMED_MINUS_ONE=-1, NAMED_ZERO=0, NAMED_ONE=1, NAMED_TWO=2 };
		void postLoad(WooTestClass&, void* addr);
		typedef boost::multi_array<Real,3> boost_multi_array_real_3;
		py::object arr3d_py_get();
		void arr3d_set(const Vector3i& shape, const VectorXr& data);
		#define woo_core_WooTestClass__CLASS_BASE_DOC_ATTRS_CTOR_PY \
			WooTestClass,Object,"This class serves to test various functionalities; it also includes all possible units, which thus get registered in woo", \
			/* enumerate all units here: */ \
			/* ((Real,angle,0.,AttrTrait<>().angleUnit(),"")) */ \
			_WOO_UNIT_ATTR(angle) \
			_WOO_UNIT_ATTR(time) \
			_WOO_UNIT_ATTR(len) \
			((Real,length_with_inches,0.,AttrTrait<>().lenUnit().altUnits({{"in",1/0.0254}}),"Variable with length, but also showing inches in the UI")) \
			_WOO_UNIT_ATTR(area) \
			_WOO_UNIT_ATTR(vol) \
			_WOO_UNIT_ATTR(vel) \
			_WOO_UNIT_ATTR(accel) \
			_WOO_UNIT_ATTR(mass) \
			_WOO_UNIT_ATTR(angVel) \
			_WOO_UNIT_ATTR(angMom) \
			_WOO_UNIT_ATTR(inertia) \
			_WOO_UNIT_ATTR(force) \
			_WOO_UNIT_ATTR(torque) \
			_WOO_UNIT_ATTR(pressure) \
			_WOO_UNIT_ATTR(stiffness) \
			_WOO_UNIT_ATTR(massRate) \
			_WOO_UNIT_ATTR(density) \
			_WOO_UNIT_ATTR(fraction) \
			_WOO_UNIT_ATTR(surfEnergy) \
			((OpenMPArrayAccumulator<Real>,aaccu,,AttrTrait<>().noGui(),"Test openmp array accumulator")) \
			((int,noSaveAttr,0,AttrTrait<Attr::noSave>(),"Attribute which is not saved")) \
			((int,hiddenAttr,0,AttrTrait<Attr::hidden>(),"Hidden data member (not accessible from python)")) \
			((int,meaning42,42,AttrTrait<Attr::readonly>(),"Read-only data member")) \
			((int,foo_incBaz,0,AttrTrait<Attr::triggerPostLoad>(),"Change this attribute to have baz incremented")) \
			((int,bar_zeroBaz,0,AttrTrait<Attr::triggerPostLoad>(),"Change this attribute to have baz incremented")) \
			((int,baz,0,AttrTrait<Attr::triggerPostLoad>(),"Value which is changed when assigning to foo_incBaz / bar_zeroBaz.")) \
			((int,postLoadStage,(int)POSTLOAD_NONE,AttrTrait<Attr::readonly>(),"Store the last stage from postLoad (to check it is called the right way)")) \
			((MatrixXr,matX,,,"MatriXr object, for testing serialization of arrays.")) \
			((boost_multi_array_real_3,arr3d,boost_multi_array_real_3(boost::extents[0][0][0]),AttrTrait<Attr::hidden>(),"boost::multi_array<Real,3> object for testing serialization of multi_array.")) \
			((int,namedEnum,NAMED_MINUS_ONE,AttrTrait<Attr::namedEnum>().namedEnum({{NAMED_MINUS_ONE,{"minus one","_1","neg1"}},{NAMED_ZERO,{"zero","nothing","NULL"}},{NAMED_ONE,{"one","single"}},{NAMED_TWO,{"two","double","2"}}}),"Named enumeration.")) \
			((int,bits,0,AttrTrait<>().bits({"bit0","bit1","bit2","bit3","bit4"}),"Test writable bits of writable flags var.")) \
			((int,bitsRw,0,AttrTrait<Attr::readonly>().bits({"bit0rw","bit1rw","bit2rw","bit3rw","bit4rw"},/*rw*/true),"Test writable bits of read-only flags var.")) \
			((int,bitsRo,3,AttrTrait<Attr::readonly>().bits({"bit0ro","bit1ro","bit2ro","bit3ro","bit4ro"}),"Test read-only bits of read-only flags var.")) \
			,/*ctor*/ \
			,/*py*/ \
				.add_property("aaccuRaw",&WooTestClass::aaccuGetRaw,&WooTestClass::aaccuSetRaw,"Access OpenMPArrayAccumulator data directly. Writing resizes and sets the 0th thread value, resetting all other ones.") \
				.def("aaccuWriteThreads",&WooTestClass::aaccuWriteThreads,(py::arg("index"),py::arg("cycleData")),"Assign a single line in the array accumulator, assigning number from *cycleData* in parallel in each thread.") \
				.def("arr3d_set",&WooTestClass::arr3d_set,(py::arg("shape"),py::arg("data")),"Set arr3d to have *shape* and fill it with *data* (must have the corresponding number of elements).") \
				.add_property("arr3d",&WooTestClass::arr3d_py_get) \
				; \
				_classObj.attr("postLoad_none")=(int)POSTLOAD_NONE; \
				_classObj.attr("postLoad_ctor")=(int)POSTLOAD_CTOR; \
				_classObj.attr("postLoad_foo")=(int)POSTLOAD_FOO; \
				_classObj.attr("postLoad_bar")=(int)POSTLOAD_BAR; \
				_classObj.attr("postLoad_baz")=(int)POSTLOAD_BAZ;
	
		WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_core_WooTestClass__CLASS_BASE_DOC_ATTRS_CTOR_PY);
	};

	struct WooTestClassStatic: public woo::Object {
		static void postLoadStatic(void* attr){
			if(attr==&trigger) numTriggered++;
		}
		WOO_CLASS_BASE_DOC_STATICATTRS(WooTestClassStatic,Object,"Class for testing static attributes access.",
			((int,namedEnum,-1,AttrTrait<Attr::namedEnum>().namedEnum({{-1,{"minus one"}},{0,{"zero","NULL"}},{1,{"one"}}}),"Test named enumeration"))
			((int,readonly,2,AttrTrait<Attr::readonly>(),"Test readonly access"))
			((int,hidden,0,AttrTrait<Attr::hidden>(),"Test hidden"))
			((int,noSave,0,AttrTrait<Attr::noSave>(),"test noSave"))
			((int,numTriggered,0,,"Counter for testing :obj:`trigger`."))
			((int,trigger,0,AttrTrait<Attr::triggerPostLoad>(),"Test triggerPostLoad"))
		);
	};



	struct WooTestPeriodicEngine: public PeriodicEngine{
		void notifyDead(){ deadCounter++; }
		#define woo_core_WooTestPeriodicEngine__CLASS_BASE_DOC_ATTRS \
			WooTestPeriodicEngine,PeriodicEngine,"Test some PeriodicEngine features.", \
			((int,deadCounter,0,,"Count how many times :obj:`dead` was assigned to."))
		WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_core_WooTestPeriodicEngine__CLASS_BASE_DOC_ATTRS);
	};
};
using woo::WooTestClass;
using woo::WooTestClassStatic;
using woo::WooTestPeriodicEngine;
WOO_REGISTER_OBJECT(WooTestClass);
WOO_REGISTER_OBJECT(WooTestClassStatic);
WOO_REGISTER_OBJECT(WooTestPeriodicEngine);

