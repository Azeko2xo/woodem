#pragma once

#include<woo/lib/base/Types.hpp>
#include<woo/lib/base/Math.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<boost/python.hpp>

// attribute flags
namespace woo{
	#define ATTR_FLAGS_VALUES noSave=(1<<0), readonly=(1<<1), triggerPostLoad=(1<<2), hidden=(1<<3), noResize=(1<<4), noGui=(1<<5), pyByRef=(1<<6), static_=(1<<7), multiUnit=(1<<8), noDump=(1<<9), activeLabel=(1<<10), rgbColor=(1<<11), filename=(1<<12), existingFilename=(1<<13), dirname=(1<<14)
	// this will disappear later
	namespace Attr { enum flags { ATTR_FLAGS_VALUES }; }
	// prohibit copies, only references should be passed around

	// see http://stackoverflow.com/questions/10570589/template-member-function-specialized-on-pointer-to-data-member and http://ideone.com/95xIt for the logic behind
	// store data here, for uniform access from python
	// derived classes only hide compile-time options
	// and contain named-param accessors, to retain the template type when chained
	struct AttrTraitBase /*:boost::noncopyable*/{
		// keep in sync with py/wrapper/wooWrapper.cpp !
		enum class Flags { ATTR_FLAGS_VALUES };
		// do not access those directly; public for convenience when accessed from python
		int _flags;
		int _currUnit;
		string _doc, _name, _cxxType,_startGroup, _hideIf;
		vector<string> _unit;
		vector<pair<string,Real>> _prefUnit;
		vector<vector<pair<string,Real>>> _altUnits;
		// avoid throwing exceptions when not initialized, just return None
		AttrTraitBase(): _flags(0)              { _ini=_range=_choice=_bits=_buttons=[]()->py::object{ return py::object(); }; }
		AttrTraitBase(int flags): _flags(flags) { _ini=_range=_choice=_bits=_buttons=[]()->py::object{ return py::object(); }; }
		std::function<py::object()> _ini;
		std::function<py::object()> _range;
		std::function<py::object()> _choice;
		std::function<py::object()> _bits;
		std::function<py::object()> _buttons;
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
			ATTR_FLAG_DO(activeLabel,isActiveLabel)
			ATTR_FLAG_DO(rgbColor,isRgbColor)
			ATTR_FLAG_DO(filename,isFilename)
			ATTR_FLAG_DO(existingFilename,isExistingFilename)
			ATTR_FLAG_DO(dirname,isDirname)
		#undef ATTR_FLAG_DO
		py::object pyGetIni()const{ return _ini(); }
		py::object pyGetRange()const{ return _range(); }
		py::object pyGetChoice()const{ return _choice(); }
		py::object pyGetBits()const{ return _bits(); }
		py::object pyGetButtons(){ return _buttons(); }
		py::object pyUnit(){ return py::object(_unit); }
		py::object pyPrefUnit(){
			// return base unit if preferred not specified
			for(size_t i=0; i<_prefUnit.size(); i++) if(_prefUnit[i].first.empty()) _prefUnit[i]=pair<string,Real>(_unit[i],1.);
			return py::object(_prefUnit);
		}
		// define alternative units: name and factor, by which the base unit is multiplied to obtain this one
		py::object pyAltUnits(){ return py::object(_altUnits); }

		string pyStr(){ return "<AttrTrait '"+_name+"', flags="+to_string(_flags)+" @ '"+lexical_cast<string>(this)+">"; }

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
				.add_property("activeLabel",&AttrTraitBase::isActiveLabel)
				.add_property("rgbColor",&AttrTraitBase::isRgbColor)
				.add_property("filename",&AttrTraitBase::isFilename)
				.add_property("existingFilename",&AttrTraitBase::isExistingFilename)
				.add_property("dirname",&AttrTraitBase::isDirname)
				.def_readonly("_flags",&AttrTraitBase::_flags)
				// non-flag attributes
				.def_readonly("doc",&AttrTraitBase::_doc)
				.def_readonly("cxxType",&AttrTraitBase::_cxxType)
				.def_readonly("name",&AttrTraitBase::_name)
				.def_readonly("unit",&AttrTraitBase::pyUnit)
				.def_readonly("prefUnit",&AttrTraitBase::pyPrefUnit)
				.add_property("altUnits",&AttrTraitBase::pyAltUnits)
				.def_readonly("startGroup",&AttrTraitBase::_startGroup)
				.add_property("hideIf",&AttrTraitBase::_hideIf)
				.add_property("ini",&AttrTraitBase::pyGetIni)
				.add_property("range",&AttrTraitBase::pyGetRange)
				.add_property("choice",&AttrTraitBase::pyGetChoice)
				.add_property("bits",&AttrTraitBase::pyGetBits)
				.add_property("buttons",&AttrTraitBase::pyGetButtons)
				.def("__str__",&AttrTraitBase::pyStr)
				.def("__repr__",&AttrTraitBase::pyStr)
			;
		}
	};
	template<int _compileFlags=0>
	struct AttrTrait: public AttrTraitBase {
		enum { compileFlags=_compileFlags };
		AttrTrait(): AttrTraitBase(_compileFlags) { };
		AttrTrait(int f){ _flags=f; } // for compatibility
		// setters
		/*
			Some flags may only be set as template argument (since they select templated code),
			not after trait construction. Their setters are not provided below. Those are:
			* noSave
			* triggerPostLoad
		*/
		#define ATTR_FLAG_DO(flag,isFlag) AttrTrait& flag(bool val=true){ if(val) _flags|=(int)Flags::flag; else _flags&=~((int)Flags::flag); return *this; } bool isFlag() const { return _flags&(int)Flags::flag; }
			// REMOVE later
			ATTR_FLAG_DO(readonly,isReadonly)
			ATTR_FLAG_DO(hidden,isHidden)
			ATTR_FLAG_DO(pyByRef,isPyByRef)

			// dynamically modifiable without harm
			ATTR_FLAG_DO(noResize,isNoResize)
			ATTR_FLAG_DO(noGui,isNoGui)
			ATTR_FLAG_DO(static_,isStatic)
			ATTR_FLAG_DO(multiUnit,isMultiUnit)
			ATTR_FLAG_DO(noDump,isNoDump)
			ATTR_FLAG_DO(activeLabel,isActiveLabel)
			ATTR_FLAG_DO(rgbColor,isRgbColor)
			ATTR_FLAG_DO(filename,isFilename)
			ATTR_FLAG_DO(existingFilename,isExistingFilename)
			ATTR_FLAG_DO(dirname,isDirname)
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
		AttrTrait& altUnits(const vector<pair<string,Real>>& m, bool clearPrevious=false){
			if(_unit.empty() && !isMultiUnit()){
				cerr<<"ERROR: Set AttrTrait.unit() before AttrTrait.altUnits()."<<endl;
				abort();
			}
			auto& alt(_altUnits[_unit.size()-1]);
			if(clearPrevious) alt=m;
			else alt.insert(alt.end(),m.begin(),m.end());
			return *this;
		}
		AttrTrait& startGroup(const string& s){ _startGroup=s; return *this; }
		AttrTrait& hideIf(const string& h){ _hideIf=h; return *this; }

		template<typename T> AttrTrait& ini(const T t){ _ini=std::function<py::object()>([=]()->py::object{ return py::object(t); }); return *this; }
		AttrTrait& ini(){ _ini=std::function<py::object()>([]()->py::object{ return py::object(); }); return *this; }

		AttrTrait& range(const Vector2i& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		AttrTrait& range(const Vector2r& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }

		// choice from integer values
		AttrTrait& choice(const vector<int>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		// choice from integer values, represented by descriptions
		AttrTrait& choice(const vector<pair<int,string>>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		AttrTrait& choice(const vector<string>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		// bitmask where each bit is represented by a string (given from the left)
		AttrTrait& bits(const vector<string>& t){ _bits=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
		// bitmask where each bit-group (size given by the int) is represented by given string (if 1, it is a simple bool, otherwise strings should be 2**i in number, so that e.g. 2 bits have 4 corresponding values which are bits 00, 01, 10, 11 respectively
		// AttrTrait& bits(const vector<pair<int,vector<string>>>& t){ _bits=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }

		AttrTrait& buttons(const vector<string>& b, bool showBefore=true){ _buttons=std::function<py::object()>([=]()->py::object{ return py::make_tuple(py::object(b),showBefore);}); return *this; }

		// shorthands for common units
		// each new unit MUST be added as an attribute to core/Test.hpp
		// so that it is know to python!!
		AttrTrait& angleUnit(){ unit("rad"); altUnits({{"deg",180/M_PI}}); return *this; }
		AttrTrait& timeUnit(){ unit("s"); altUnits({{"ms",1e3},{"μs",1e6},{"day",1./(60*60*24)},{"year",1./(60*60*24*365)}}); return *this; }
		AttrTrait& lenUnit(){ unit("m"); altUnits({{"mm",1e3}}); return *this; }
		AttrTrait& areaUnit(){ unit("m²"); altUnits({{"mm²",1e6}}); return *this; }
		AttrTrait& volUnit(){ unit("m³"); altUnits({{"mm³",1e9}}); return *this; }
		AttrTrait& velUnit(){ unit("m/s"); altUnits({{"km/h",1e-3*3600}}); return *this; }
		AttrTrait& accelUnit(){ unit("m/s²"); return *this; }
		AttrTrait& massUnit(){ unit("kg"); altUnits({{"g",1e3},{"t",1e-3}}); return *this; }
		AttrTrait& angVelUnit(){ unit("rad/s"); altUnits({{"rot/s",1/(2*M_PI)},{"rot/min",60.*(1./(2*M_PI))}}); return *this; }
		AttrTrait& angMomUnit(){ unit("N·m·s"); return *this; }
		AttrTrait& inertiaUnit(){ unit("kg·m²"); return *this; }
		AttrTrait& forceUnit(){ unit("N"); altUnits({{"kN",1e-3},{"MN",1e-6}}); return *this; }
		AttrTrait& torqueUnit(){ unit("N·m"); altUnits({{"kN·m",1e-3},{"MN·m",1e-6}}); return *this; }
		AttrTrait& pressureUnit(){ unit("Pa"); altUnits({{"kPa",1e-3},{"MPa",1e-6},{"GPa",1e-9}}); return *this; }
		AttrTrait& stiffnessUnit(){ return pressureUnit(); }
		AttrTrait& massFlowRateUnit(){ unit("kg/s"); altUnits({{"t/h",1e-3*3600},{"t/y",1e-3*(24*3600*365)},{"Mt/y",1e-6*1e-3*(24*3600*365)}}); return *this; }
		AttrTrait& densityUnit(){ unit("kg/m³"); altUnits({{"t/m³",1e-3},{"g/cm³",1e-3}}); return *this; }
		AttrTrait& fractionUnit(){ unit("-"); altUnits({{"%",1e2},{"‰",1e3},{"ppm",1e6}}); return *this; }

		AttrTrait& colormapChoice(){ vector<pair<int,string>> pairs({{-1,"[default] (-1)"}}); for(size_t i=0; i<CompUtils::colormaps.size(); i++) pairs.push_back({i,CompUtils::colormaps[i].name+" ("+to_string(i)+")"}); return choice(pairs); }
	};
	#undef ATTR_FLAGS_VALUES

	// overloaded for seamless expansion of macros
	// empty value returns default traits
	inline AttrTrait<0> makeAttrTrait(){ return AttrTrait<0>(); }
	// passing an AttrTrait object returns itself
	template<int flags>
	AttrTrait<flags> makeAttrTrait(const AttrTrait<flags>& at){ return at; }

	struct ClassTrait{
		string pyStr(){ return "<ClassTrait '"+_name+"' @ "+lexical_cast<string>(this)+">"; }
		string _doc;
		string _name;
		string title;
		string intro;
		vector<string> docOther;

		string getDoc() const { return _doc; }
		ClassTrait& doc(const string __doc){ _doc=__doc; return *this; }
		ClassTrait& name(const string& __name){ _name=__name; return *this; }
		ClassTrait& section(const string& _title, const string& _intro, const vector<string>& _docOther){ title=_title; intro=_intro; docOther=_docOther; return *this; }
		static void pyRegisterClass(){
			py::class_<ClassTrait,shared_ptr<ClassTrait>>("ClassTrait",py::no_init)
				.def_readonly("title",&ClassTrait::title)
				.def_readonly("intro",&ClassTrait::intro)
				// custom converters are needed for vector<string>
				.add_property("docOther",py::make_getter(&ClassTrait::docOther,py::return_value_policy<py::return_by_value>())/*,py::make_setter(&ClassTrait::docOther,py::return_value_policy<py::return_by_value>())*/)
				.def_readonly("doc",&ClassTrait::_doc)
				.def_readonly("name",&ClassTrait::_name)
				.def("__str__",&ClassTrait::pyStr)
				.def("__repr__",&ClassTrait::pyStr)
			;
		}
	};
	inline ClassTrait makeClassTrait(){ return ClassTrait(); }
	inline ClassTrait makeClassTrait(const string& _doc){ return ClassTrait().doc(_doc); }
	inline ClassTrait makeClassTrait(ClassTrait& c){ return c; }
};

