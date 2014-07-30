#include<woo/lib/pyutil/pickle.hpp>
#include<woo/lib/pyutil/gil.hpp>

namespace woo{
	py::object Pickler::cPickle_dumps;
	py::object Pickler::cPickle_loads;
	bool Pickler::initialized=false;

	void Pickler::ensureInitialized(){
		if(initialized) return;
		GilLock pyLock;
		//cerr<<"[Pickler::ensureInitialized:gil]";
		#if PY_MAJOR_VERSION >= 3
			py::object cPickle=py::import("pickle");
		#else
			py::object cPickle=py::import("cPickle");
		#endif
		cPickle_dumps=cPickle.attr("dumps");
		cPickle_loads=cPickle.attr("loads");
		initialized=true;
	}

	std::string Pickler::dumps(py::object o){
		ensureInitialized();
		GilLock pyLock;
		//cerr<<"[dumps:gil]";
		return py::extract<string>(cPickle_dumps(o,-1))(); // -1: use binary protocol
	}
	py::object Pickler::loads(const std::string& s){
		ensureInitialized();
		GilLock pyLock;
		//cerr<<"[loads:gil]";
		return cPickle_loads(s);
	}
}
