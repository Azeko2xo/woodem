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
		#if PY_MAJOR_VERSION >= 3
			// destructed at the end of the scope, when std::string copied the content already
			py::object s(cPickle_dumps(o,-1)); 
			PyObject* b=s.ptr();
			assert(PyBytes_Check(b));
			//cerr<<"[dumps:gil:length="<<PyBytes_Size(b)<<"]";
			// bytes may contain 0 (only in py3k apparently), make sure size is passed explicitly
			return std::string(PyBytes_AsString(b),PyBytes_Size(b)); // -1: use binary protocol
		#else
			return py::extract<string>(cPickle_dumps(o,-1))(); // -1: use binary protocol
		#endif
		;
	}
	py::object Pickler::loads(const std::string& s){
		ensureInitialized();
		GilLock pyLock;
		//cerr<<"[loads:gil]";
		#if PY_MAJOR_VERSION >= 3
			return cPickle_loads(py::handle<>(PyBytes_FromStringAndSize(s.data(),(Py_ssize_t)s.size())));
		#else
			return cPickle_loads(s);
		#endif
	}
}
