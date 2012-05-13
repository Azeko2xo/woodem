#include<yade/lib/base/Types.hpp>
#include<yade/lib/base/Math.hpp>
#include<boost/python.hpp>

// attribute flags
namespace yade{
	#define ATTR_FLAGS_VALUES noSave=1, readonly=2, triggerPostLoad=4, hidden=8, noResize=16, noGui=32, pyByRef=64, static_=128
	// this will disappear later
	namespace Attr { enum flags { ATTR_FLAGS_VALUES }; }
	// prohibit copies, only references should be passed around

	// see http://stackoverflow.com/questions/10570589/template-member-function-specialized-on-pointer-to-data-member and http://ideone.com/95xIt for the logic behind
	// store data here, for uniform access from python
	// derived classes only hide compile-time options
	// and contain named-param accessors, to retain the template type when chained
	struct AttrTraitBase /*:boost::noncopyable*/{
		// keep in sync with py/wrapper/yadeWrapper.cpp !
		enum class Flags { ATTR_FLAGS_VALUES };
		// do not access those directly; public for convenience when accessed from python
		int _flags;
		string _doc, _name, _cxxType, _unit, _prefUnit, _startGroup;
		vector<pair<string,Real>> _altUnits;
		// avoid throwing exceptions when not initialized, just return None
		AttrTraitBase(): _flags(0) { _ini=_range=_choice=[]()->py::object{ return py::object(); }; }
		std::function<py::object()> _ini;
		std::function<py::object()> _range;
		std::function<py::object()> _choice;
		// getters (setters below, to return the same reference type)
		#define ATTR_FLAG_DO(flag,isFlag) bool isFlag() const { return _flags&(int)Flags::flag; }
			ATTR_FLAG_DO(noSave,isNoSave)
			ATTR_FLAG_DO(readonly,isReadonly)
			ATTR_FLAG_DO(triggerPostLoad,isTriggerPostLoad)
			ATTR_FLAG_DO(hidden,isHidden)
			ATTR_FLAG_DO(noResize,isNoResize)
			ATTR_FLAG_DO(noGui,isNoGui)
			ATTR_FLAG_DO(pyByRef,isPyByRef)
			ATTR_FLAG_DO(static_,isStatic)
		#undef ATTR_FLAG_DO
		py::object pyGetIni()const{ return _ini(); }
		py::object pyGetRange()const{ return _range(); }
		py::object pyGetChoice()const{ return _choice(); }
		// define alternative units: name and factor, by which the base unit is multiplied to obtain this one
		py::object pyAltUnits(){ return py::object(_altUnits); }

		static void pyRegisterClass(){
			py::class_<AttrTraitBase,boost::noncopyable>("AttrTrait",py::no_init) // this is intentional; no need for templates in python
				.add_property("noSave",&AttrTraitBase::isNoSave)
				.add_property("readonly",&AttrTraitBase::isReadonly)
				.add_property("triggerPostLoad",&AttrTraitBase::isTriggerPostLoad)
				.add_property("hidden",&AttrTraitBase::isHidden)
				.add_property("noResize",&AttrTraitBase::isNoResize)
				.add_property("noGui",&AttrTraitBase::isNoGui)
				.add_property("pyByRef",&AttrTraitBase::isPyByRef)
				.add_property("static",&AttrTraitBase::isStatic)
				.def_readonly("_flags",&AttrTraitBase::_flags)
				// non-flag attributes
				.def_readonly("doc",&AttrTraitBase::_doc)
				.def_readonly("cxxType",&AttrTraitBase::_cxxType)
				.def_readonly("name",&AttrTraitBase::_name)
				.def_readonly("unit",&AttrTraitBase::_unit)
				.def_readonly("prefUnit",&AttrTraitBase::_prefUnit)
				.def_readonly("startGroup",&AttrTraitBase::_startGroup)
				.add_property("altUnits",&AttrTraitBase::pyAltUnits)
				.add_property("ini",&AttrTraitBase::pyGetIni)
				.add_property("range",&AttrTraitBase::pyGetRange)
				.add_property("choice",&AttrTraitBase::pyGetChoice)
			;
		}
	};
	template<int _compileFlags=0>
	struct AttrTrait: public AttrTraitBase {
		enum { compileFlags=_compileFlags };
		AttrTrait(){};
		AttrTrait(int f){ _flags=f; } // for compatibility
		// setters
		#define ATTR_FLAG_DO(flag,isFlag) AttrTrait& flag(bool val=true){ if(val) _flags|=(int)Flags::flag; else _flags&=~((int)Flags::flag); return *this; } bool isFlag() const { return _flags&(int)Flags::flag; }
			ATTR_FLAG_DO(noSave,isNoSave)
			ATTR_FLAG_DO(readonly,isReadonly)
			ATTR_FLAG_DO(triggerPostLoad,isTriggerPostLoad)
			ATTR_FLAG_DO(hidden,isHidden)
			ATTR_FLAG_DO(noResize,isNoResize)
			ATTR_FLAG_DO(noGui,isNoGui)
			ATTR_FLAG_DO(pyByRef,isPyByRef)
			ATTR_FLAG_DO(static_,isStatic)
		#undef ATTR_FLAG_DO
		AttrTrait& name(const string& s){ _name=s; return *this; }
		AttrTrait& doc(const string& s){ _doc=s; return *this; }
		AttrTrait& cxxType(const string& s){ _cxxType=s; return *this; }
		AttrTrait& unit(const string& s){ _unit=s; return *this; }
		AttrTrait& prefUnit(const string& s){ _prefUnit=s; return *this; }
		AttrTrait& altUnits(const vector<pair<string,Real>>& m){ _altUnits=m; return *this; }
		AttrTrait& startGroup(const string& s){ _startGroup=s; return *this; }

		template<typename T> AttrTrait& ini(const T& t){ _ini=std::function<py::object()>([=]()->py::object{ return py::object(t); }); return *this; }
		AttrTrait& ini(){ _ini=std::function<py::object()>([]()->py::object{ return py::object(); }); return *this; }

		AttrTrait& range(const Vector2i& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		AttrTrait& range(const Vector2r& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }

		AttrTrait& choice(const vector<int>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		AttrTrait& choice(const vector<pair<int,string>>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
			// { py::dict ret; for(const auto& a: _altUnits) ret[a.first]=a.second; return ret; }
		// shorthands for common units
		AttrTrait& angleUnit(){ unit("rad"); altUnits({{"deg",180/Mathr::PI}}); return *this; }
		AttrTrait& lenUnit(){ unit("m"); altUnits({{"mm",1e3}}); return *this; }
		AttrTrait& velUnit(){ unit("m/s"); altUnits({{"km/h",1e-3*3600}}); return *this; }
		AttrTrait& accelUnit(){ unit("m/s²"); return *this; }
		AttrTrait& massUnit(){ unit("kg"); altUnits({{"g",1e3},{"t",1e-3}}); return *this; }
		AttrTrait& angVelUnit(){ unit("rad/s"); altUnits({{"rot/s",1./Mathr::PI}}); return *this; }
		AttrTrait& pressureUnit(){ unit("Pa"); altUnits({{"kPa",1e-3},{"MPa",1e-6},{"GPa",1e-9}}); return *this; }
		AttrTrait& stiffnessUnit(){ return pressureUnit(); }
		AttrTrait& massFlowRateUnit(){ unit("kg/s"); altUnits({{"t/year",1e-3*(24*3600*365)}}); return *this; }
		AttrTrait& densityUnit(){ unit("kg/m³"); altUnits({{"t/m³",1e-3},{"g/cm³",1e-3}}); return *this; }


	};
	#undef ATTR_FLAGS_VALUES
};

