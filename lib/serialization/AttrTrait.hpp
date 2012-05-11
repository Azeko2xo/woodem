#include<yade/lib/base/Types.hpp>
#include<yade/lib/base/Math.hpp>
#include<boost/python.hpp>

// attribute flags
namespace yade{
	#define ATTR_FLAGS_VALUES noSave=1, readonly=2, triggerPostLoad=4, hidden=8, noResize=16, noGui=32, pyByRef=64, static_=12
	// this will disappear later
	namespace Attr { enum flags { ATTR_FLAGS_VALUES }; }
	struct AttrTrait{
		// do not access those directly; public for convenience when accessed from python
			int _flags;
			string _doc, _name, _cxxType, _unit, _prefUnit;
			map<string,Real> _altUnits;
			//boost::any _ini;
			std::function<py::object()> _ini;
			std::function<py::object()> _range;
			std::function<py::object()> _choice;
			// keep in sync with py/wrapper/yadeWrapper.cpp !
			enum class Flags { ATTR_FLAGS_VALUES };
			void resetFactories(){ _ini=_range=_choice=[]()->py::object{ return py::object(); }; }
			AttrTrait(): _flags(0) { resetFactories(); }
			AttrTrait(int f): _flags(f) { resetFactories(); }  // construct from flags, for compat
			AttrTrait(const AttrTrait& b): _flags(b._flags), _doc(b._doc), _name(b._name), _cxxType(b._cxxType), _unit(b._unit), _prefUnit(b._prefUnit), _ini(b._ini), _range(b._range), _choice(b._choice){ _altUnits.insert(b._altUnits.begin(),b._altUnits.end()); }
			AttrTrait& operator=(const AttrTrait& b){
				if(this==&b) return *this;
				_flags=b._flags; _doc=b._doc; _name=b._name; _cxxType=b._cxxType; _unit=b._unit; _prefUnit=b._prefUnit; _altUnits.insert(b._altUnits.begin(),b._altUnits.end()); _ini=b._ini; _range=b._range; _choice=b._choice;
				return *this;
			}

			#define ATTR_FLAG(flag,isFlag) AttrTrait& flag(bool val=true){ if(val) _flags|=(int)Flags::flag; else _flags&=~((int)Flags::flag); return *this; } bool isFlag() const { return _flags&(int)Flags::flag; }
				ATTR_FLAG(noSave,isNoSave)
				ATTR_FLAG(readonly,isReadonly)
				ATTR_FLAG(triggerPostLoad,isTriggerPostLoad)
				ATTR_FLAG(hidden,isHidden)
				ATTR_FLAG(noResize,isNoResize)
				ATTR_FLAG(noGui,isNoGui)
				ATTR_FLAG(pyByRef,isPyByRef)
				ATTR_FLAG(static_,isStatic)
			#undef ATTR_FLAG
			int pyGetFlags() const{ return _flags; }

			AttrTrait& name(const string& s){ _name=s; return *this; }
			AttrTrait& doc(const string& s){ _doc=s; return *this; }
			AttrTrait& cxxType(const string& s){ _cxxType=s; return *this; }
			AttrTrait& unit(const string& s){ _unit=s; return *this; }
			AttrTrait& prefUnit(const string& s){ _prefUnit=s; return *this; }
			AttrTrait& altUnits(const map<string,Real>& m){ _altUnits=m; return *this; }

			template<typename T>
			AttrTrait& ini(const T& t){ _ini=std::function<py::object()>([=]()->py::object{ return py::object(t); }); return *this; }
			AttrTrait& ini(){ _ini=std::function<py::object()>([]()->py::object{ return py::object(); }); return *this; }
			//AttrTrait& range(const Vector2i& r){ _range=r; return *this; }
			//AttrTrait& range(const Vector2r& r){ _range=r; return *this; }
			//template<typename T>
			AttrTrait& range(const Vector2i& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
			AttrTrait& range(const Vector2r& t){ _range=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }

			AttrTrait& choice(const vector<int>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }
			AttrTrait& choice(const vector<pair<int,string>>& t){ _choice=std::function<py::object()>([=]()->py::object{ return py::object(t);} ); return *this; }

			//AttrTrait& range(const T& t){ _range=std::function<py::object()>([&]()->py::object{ return py::object(t);} ); return *this; }
			//AttrTrait& choice(const vector<int>& c){ _choice=c; return *this; }
			//AttrTrait& choice(const vector<std::tuple<string,int>>& c){ _choice=c; return *this; }
			py::object pyGetIni()const{ return _ini(); }
			py::object pyGetRange()const{ return _range(); }
			py::object pyGetChoice()const{ return _choice(); }
			
			// define alternative units: name and factor, by which the base unit is multiplied to obtain this one
			py::dict pyAltUnits(){ py::dict ret; for(const auto& a: _altUnits) ret[a.first]=a.second; return ret; }
			// shorthands for common units
			AttrTrait& angleUnit(){ unit("rad"); altUnits({{"deg",180/Mathr::PI}}); return *this; }
			AttrTrait& lenUnit(){ unit("m"); altUnits({{"mm",1e3}}); return *this; }
			AttrTrait& velUnit(){ unit("m/s"); altUnits({{"km/h",1e-3*3600}}); return *this; }
			AttrTrait& accelUnit(){ unit("m/s²"); return *this; }
			AttrTrait& massUnit(){ unit("kg"); altUnits({{"g",1e3},{"t",1e-3}}); return *this; }
			AttrTrait& angVelUnit(){ unit("rad/s"); altUnits({{"rot/s",1./Mathr::PI}}); return *this; }
			AttrTrait& pressureUnit(){ unit("Pa"); altUnits({{"kPa",1e-3},{"MPa",1e-6},{"GPa",1e-9}}); return *this; }
			AttrTrait& stiffnessUnit(){ return pressureUnit(); }
			AttrTrait& massFlowUnit(){ unit("kg/s"); altUnits({{"t/year",1e-3*(24*3600*365)}}); return *this; }


			static void pyRegisterClass(){
				py::class_<AttrTrait>("AttrTrait")
					.add_property("noSave",&AttrTrait::isNoSave)
					.add_property("readonly",&AttrTrait::isReadonly)
					.add_property("triggerPostLoad",&AttrTrait::isTriggerPostLoad)
					.add_property("hidden",&AttrTrait::isHidden)
					.add_property("noResize",&AttrTrait::isNoResize)
					.add_property("noGui",&AttrTrait::isNoGui)
					.add_property("pyByRef",&AttrTrait::isPyByRef)
					.add_property("static",&AttrTrait::isStatic)
					.add_property("_flags",&AttrTrait::pyGetFlags)
					// non-flag attributes
					.def_readonly("doc",&AttrTrait::_doc)
					.def_readonly("cxxType",&AttrTrait::_cxxType)
					.def_readonly("name",&AttrTrait::_name)
					.def_readonly("unit",&AttrTrait::_unit)
					.def_readonly("prefUnit",&AttrTrait::_prefUnit)
					.add_property("altUnits",&AttrTrait::pyAltUnits)
					.add_property("ini",&AttrTrait::pyGetIni)
					.add_property("range",&AttrTrait::pyGetRange)
					.add_property("choice",&AttrTrait::pyGetChoice)
				;
			}
	};
	#undef ATTR_FLAGS_VALUES
	#if 0
		/* make templated child class for compile-time flags */
		template<bool noSave=false>
		struct AttrTrait: public AttrTraitBase{
			AttrTrait(){ }
			static void pyRegisterClass(const string& suffix){
				py::class_<decltype(*this),py::bases<AttrTraitBase>>(("AttrTrait"+suffix).c_str());
			}
		};
	#endif
};

