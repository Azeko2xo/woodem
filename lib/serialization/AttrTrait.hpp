#include<yade/lib/base/Types.hpp>
#include<yade/lib/base/Math.hpp>
#include<boost/python.hpp>

// attribute flags
namespace yade{
	#define ATTR_FLAGS_VALUES noSave=1, readonly=2, triggerPostLoad=4, hidden=8, noResize=16, noGui=32, pyByRef=64, static_=128, multiUnit=256, noDump=512
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
		int _currUnit;
		string _doc, _name, _cxxType,_startGroup;
		vector<string> _unit;
		vector<pair<string,Real>> _prefUnit;
		vector<vector<pair<string,Real>>> _altUnits;
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
			ATTR_FLAG_DO(multiUnit,isMultiUnit)
			ATTR_FLAG_DO(noDump,isNoDump)
		#undef ATTR_FLAG_DO
		py::object pyGetIni()const{ return _ini(); }
		py::object pyGetRange()const{ return _range(); }
		py::object pyGetChoice()const{ return _choice(); }
		py::object pyUnit(){ return py::object(_unit); }
		py::object pyPrefUnit(){
			// return base unit if preferred not specified
			for(size_t i=0; i<_prefUnit.size(); i++) if(_prefUnit[i].first.empty()) _prefUnit[i]=pair<string,Real>(_unit[i],1.);
			return py::object(_prefUnit);
		}
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
				.add_property("multiUnit",&AttrTraitBase::isMultiUnit)
				.add_property("noDump",&AttrTraitBase::isNoDump)
				.def_readonly("_flags",&AttrTraitBase::_flags)
				// non-flag attributes
				.def_readonly("doc",&AttrTraitBase::_doc)
				.def_readonly("cxxType",&AttrTraitBase::_cxxType)
				.def_readonly("name",&AttrTraitBase::_name)
				.def_readonly("unit",&AttrTraitBase::pyUnit)
				.def_readonly("prefUnit",&AttrTraitBase::pyPrefUnit)
				.add_property("altUnits",&AttrTraitBase::pyAltUnits)
				.def_readonly("startGroup",&AttrTraitBase::_startGroup)
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
			ATTR_FLAG_DO(multiUnit,isMultiUnit)
			ATTR_FLAG_DO(noDump,isNoDump)
		#undef ATTR_FLAG_DO
		AttrTrait& name(const string& s){ _name=s; return *this; }
		AttrTrait& doc(const string& s){ _doc=s; return *this; }
		AttrTrait& cxxType(const string& s){ _cxxType=s; return *this; }
		AttrTrait& unit(const string& s){
			if(!_unit.empty() && !isMultiUnit()){
				cerr<<"ERROR: AttrTrait must be declared .multiUnit() before additional units are specified."<<endl;
				abort();
			}
			_unit.push_back(s); _altUnits.resize(_unit.size()); _prefUnit.resize(_unit.size());
			return *this;
		}
		AttrTrait& prefUnit(const string& s){
			if(_unit.empty() && !isMultiUnit()){
				cerr<<"ERROR: Set AttrTrait.unit() before AttrTrait.prefUnit()."<<endl;;
				abort();
			}
			Real mult;
			if(s==_unit.back()) mult=1.;
			else{
				/*find multiplier*/
				auto I=std::find_if(_altUnits.back().begin(),_altUnits.back().end(),[&](const pair<string,Real>& a)->bool{ return a.first==s; });
				if(I!=_altUnits.back().end()){ mult=I->second; }
				else throw std::runtime_error("AttrTrait for "+_name+": prefUnit(\""+s+"\") does not match any .altUnits()");
			}
			_prefUnit[_unit.size()-1]={s,mult};
			return *this;
		}
		AttrTrait& altUnits(const vector<pair<string,Real>>& m){
			if(_unit.empty() && !isMultiUnit()){
				cerr<<"ERROR: Set AttrTrait.unit() before AttrTrait.altUnits()."<<endl;
				abort();
			}
			_altUnits[_unit.size()-1]=m; return *this;
		}
		AttrTrait& startGroup(const string& s){ _startGroup=s; return *this; }

		template<typename T> AttrTrait& ini(const T& t){ _ini=std::function<py::object()>([=]()->py::object{ return py::object(t); }); return *this; }
		AttrTrait& ini(){ _ini=std::function<py::object()>([]()->py::object{ return py::object(); }); return *this; }

		AttrTrait& range(const Vector2i& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		AttrTrait& range(const Vector2r& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }

		AttrTrait& choice(const vector<int>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		AttrTrait& choice(const vector<pair<int,string>>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		// shorthands for common units
		AttrTrait& angleUnit(){ unit("rad"); altUnits({{"deg",180/Mathr::PI}}); return *this; }
		AttrTrait& lenUnit(){ unit("m"); altUnits({{"mm",1e3}}); return *this; }
		AttrTrait& areaUnit(){ unit("m²"); altUnits({{"mm²",1e6}}); return *this; }
		AttrTrait& volUnit(){ unit("m³"); altUnits({{"mm³",1e9}}); return *this; }
		AttrTrait& velUnit(){ unit("m/s"); altUnits({{"km/h",1e-3*3600}}); return *this; }
		AttrTrait& accelUnit(){ unit("m/s²"); return *this; }
		AttrTrait& massUnit(){ unit("kg"); altUnits({{"g",1e3},{"t",1e-3}}); return *this; }
		AttrTrait& angVelUnit(){ unit("rad/s"); altUnits({{"rot/s",1./Mathr::PI}}); return *this; }
		AttrTrait& angMomUnit(){ unit("N·m·s"); return *this; }
		AttrTrait& inertiaUnit(){ unit("kg·m²"); return *this; }
		AttrTrait& forceUnit(){ unit("N"); altUnits({{"kN",1e-3},{"MN",1e-6}}); return *this; }
		AttrTrait& torqueUnit(){ unit("N·m"); altUnits({{"kN·m",1e-3},{"MN·m",1e-6}}); return *this; }
		AttrTrait& pressureUnit(){ unit("Pa"); altUnits({{"kPa",1e-3},{"MPa",1e-6},{"GPa",1e-9}}); return *this; }
		AttrTrait& stiffnessUnit(){ return pressureUnit(); }
		AttrTrait& massFlowRateUnit(){ unit("kg/s"); altUnits({{"t/year",1e-3*(24*3600*365)}}); return *this; }
		AttrTrait& densityUnit(){ unit("kg/m³"); altUnits({{"t/m³",1e-3},{"g/cm³",1e-3}}); return *this; }
		AttrTrait& fractionUnit(){ unit("-"); altUnits({{"%",1e2},{"‰",1e3},{"ppm",1e6}}); return *this; }


	};
	#undef ATTR_FLAGS_VALUES
};
