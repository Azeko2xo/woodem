// 2009 © Václav Šmilauer <eudoxos@arcig.cz>


// this is not currently used, but can be enabled if needed
// probably breaks compilation for older (like <=1.35 or so)
// boost::python
#if 0
	#include<indexing_suite/container_suite.hpp>
	// #include<indexing_suite/container_proxy.hpp>
	#include<indexing_suite/vector.hpp>
#endif

#if 0
	#include<woo/lib/pyutil/numpy.hpp>
#endif

#include<boost/python.hpp>
#include<boost/python/class.hpp>
#include<boost/python/module.hpp>
#include<boost/foreach.hpp>
#ifndef FOREACH
	#define FOREACH BOOST_FOREACH
#endif

#include<vector>
#include<string>
#include<stdexcept>
#include<iostream>
#include<map>

#include<woo/lib/base/Types.hpp>

#include<woo/lib/base/Math.hpp>
#include<woo/lib/base/openmp-accu.hpp>

#include<woo/core/Field.hpp>
#include<woo/core/Scene.hpp>

#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/LawTester.hpp>

#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/core/MatchMaker.hpp>


#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Functors.hpp>
	#include<woo/pkg/gl/Renderer.hpp>
	#include<woo/pkg/gl/NodeGlRep.hpp>
#endif

#include <boost/python/suite/indexing/vector_indexing_suite.hpp>



using namespace py; // = boost::python

struct custom_OpenMPAccumulator_to_float{ static PyObject* convert(const OpenMPAccumulator<Real>& acc){ return incref(PyFloat_FromDouble(acc.get())); } };
struct custom_OpenMPAccumulator_from_float{
	custom_OpenMPAccumulator_from_float(){  converter::registry::push_back(&convertible,&construct,type_id<OpenMPAccumulator<Real> >()); }
	static void* convertible(PyObject* obj_ptr){ return PyFloat_Check(obj_ptr) ? obj_ptr : 0; }
	static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data){ void* storage=((converter::rvalue_from_python_storage<OpenMPAccumulator<Real> >*)(data))->storage.bytes; new (storage) OpenMPAccumulator<Real>; ((OpenMPAccumulator<Real>*)storage)->set(extract<Real>(obj_ptr)); data->convertible=storage; }
};
struct custom_OpenMPAccumulator_to_int  { static PyObject* convert(const OpenMPAccumulator<int>& acc){ return incref(PyInt_FromLong((long)acc.get())); } };
struct custom_OpenMPAccumulator_from_int{
	custom_OpenMPAccumulator_from_int(){  converter::registry::push_back(&convertible,&construct,type_id<OpenMPAccumulator<int> >()); }
	static void* convertible(PyObject* obj_ptr){ return PyInt_Check(obj_ptr) ? obj_ptr : 0; }
	static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data){ void* storage=((converter::rvalue_from_python_storage<OpenMPAccumulator<int> >*)(data))->storage.bytes; new (storage) OpenMPAccumulator<int>; ((OpenMPAccumulator<int>*)storage)->set(extract<int>(obj_ptr)); data->convertible=storage; }
};

template<typename T>
struct custom_vvector_to_list{
	static PyObject* convert(const vector<vector<T> >& vv){
		py::list ret; FOREACH(const vector<T>& v, vv){
			py::list ret2;
			FOREACH(const T& e, v) ret2.append(e);
			ret.append(ret2);
		}
		return incref(ret.ptr());
	}
};

template<typename containedType>
struct custom_list_to_list{
	static PyObject* convert(const std::list<containedType>& v){
		py::list ret; FOREACH(const containedType& e, v) ret.append(e);
		return incref(ret.ptr());
	}
};
/* pair-tuple converter */
template<typename CxxPair>
struct custom_CxxPair_to_PyTuple{
	static PyObject* convert(const CxxPair& t){ return incref(py::make_tuple(std::get<0>(t),std::get<1>(t)).ptr()); }
};
template<typename CxxPair>
struct custom_CxxPair_from_PyTuple{
	custom_CxxPair_from_PyTuple(){ converter::registry::push_back(&convertible,&construct,type_id<CxxPair>()); }
	static void* convertible(PyObject* obj_ptr){
		if(!PySequence_Check(obj_ptr) || !PyObject_HasAttrString(obj_ptr,"__len__") || PySequence_Size(obj_ptr)!=2) return 0;
		return obj_ptr;
	}
	static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data){
		void* storage=((converter::rvalue_from_python_storage<CxxPair>*)(data))->storage.bytes;
		new (storage) CxxPair();
		CxxPair* t=(CxxPair*)(storage);
		t->first=extract<typename CxxPair::first_type>(PySequence_GetItem(obj_ptr,0));
		t->second=extract<typename CxxPair::second_type>(PySequence_GetItem(obj_ptr,1));
		data->convertible=storage;
	}
};

/*** c++-list to python-list */
template<typename containedType>
struct custom_vector_to_list{
	static PyObject* convert(const vector<containedType>& v){
		py::list ret; FOREACH(const containedType& e, v) ret.append(e);
		return incref(ret.ptr());
	}
};
template<typename containedType>
struct custom_vector_from_seq{
	custom_vector_from_seq(){ converter::registry::push_back(&convertible,&construct,type_id<vector<containedType> >()); }
	static void* convertible(PyObject* obj_ptr){
		// the second condition is important, for some reason otherwise there were attempted conversions of Body to list which failed afterwards.
		if(!PySequence_Check(obj_ptr) || !PyObject_HasAttrString(obj_ptr,"__len__")) return 0;
		return obj_ptr;
	}
	static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data){
		 void* storage=((converter::rvalue_from_python_storage<vector<containedType> >*)(data))->storage.bytes;
		 new (storage) vector<containedType>();
		 vector<containedType>* v=(vector<containedType>*)(storage);
		 int l=PySequence_Size(obj_ptr); if(l<0) abort(); /*std::cerr<<"l="<<l<<"; "<<typeid(containedType).name()<<std::endl;*/ v->reserve(l); for(int i=0; i<l; i++) { v->push_back(extract<containedType>(PySequence_GetItem(obj_ptr,i))); }
		 data->convertible=storage;
	}
};


struct custom_ptrMatchMaker_from_float{
	custom_ptrMatchMaker_from_float(){ converter::registry::push_back(&convertible,&construct,type_id<shared_ptr<MatchMaker> >()); }
	static void* convertible(PyObject* obj_ptr){ if(!PyNumber_Check(obj_ptr)) { cerr<<"Not convertible to MatchMaker"<<endl; return 0; } return obj_ptr; }
	static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data){
		void* storage=((converter::rvalue_from_python_storage<shared_ptr<MatchMaker> >*)(data))->storage.bytes;
		new (storage) shared_ptr<MatchMaker>(new MatchMaker); // allocate the object at given address
		shared_ptr<MatchMaker>* mm=(shared_ptr<MatchMaker>*)(storage); // convert that address to our type
		(*mm)->algo="val"; (*mm)->val=PyFloat_AsDouble(obj_ptr); (*mm)->postLoad(**mm);
		data->convertible=storage;
	}
};



#if 0
template<typename numT, int dim>
struct custom_numpyBoost_to_py{
	static PyObject* convert(numpy_boost<numT, dim> nb){
		return nb.py_ptr(); // handles incref internally
	}
};
#endif

// this defines getstate and setstate methods to support pickling on linear sequences (should work for std::list as well)
template<class T>
struct VectorPickle: py::pickle_suite{
	static py::list getstate(const T& tt){ py::list ret; for(const typename T::value_type& t: tt) ret.append(t); return ret; }
	static void setstate(T& tt, py::list state){ tt.clear(); for(int i=0;i<py::len(state);i++) tt.push_back(py::extract<typename T::value_type>(state[i])); }
};


BOOST_PYTHON_MODULE(_customConverters){

	// syntax ??
	//   http://language-binding.net/pyplusplus/documentation/indexing_suite_v2.html.html#container_proxy
	//   http://www.mail-archive.com/cplusplus-sig@python.org/msg00862.html
	//class_<indexing::container_proxy<vector<string> >,bases<class_<vector<string> > > >("vecStr").def(indexing::container_suite<indexing::container_proxy<vector<string> > >());
	//class_<vector<string> >("vecStr").def(indexing::container_suite<vector<string> >());

	#if 0
		custom_vector_from_seq<string>(); class_<vector<string> >("vector_" "string").def(indexing::container_suite<vector<string> >()).def("__repr__",&vectorRepr<string>);
		custom_vector_from_seq<int>(); class_<vector<int> >("vector_" "int").def(indexing::container_suite<vector<int> >()).def("__repr__",&vectorRepr<int>);
		custom_vector_from_seq<Real>(); class_<vector<Real> >("vector_" "Real").def(indexing::container_suite<vector<Real> >()).def("__repr__",&vectorRepr<Real>);
		// this needs operator< for Vector3r (defined above, but not picked up for some reason)
		custom_vector_from_seq<Vector3r>(); class_<vector<Vector3r> >("vector_" "Vector3r").def(indexing::container_suite<vector<Vector3r> >()).def("__repr__",&vectorRepr<Vector3r>);
	#endif

	custom_OpenMPAccumulator_from_float(); to_python_converter<OpenMPAccumulator<Real>, custom_OpenMPAccumulator_to_float>(); 
	custom_OpenMPAccumulator_from_int(); to_python_converter<OpenMPAccumulator<int>, custom_OpenMPAccumulator_to_int>(); 

	custom_ptrMatchMaker_from_float();

	// 2-way conversion for std::pair -- python 2-tuple
	#define PAIR_TUPLE_CONV(T) custom_CxxPair_from_PyTuple<T>(); to_python_converter<T,custom_CxxPair_to_PyTuple<T>>();
	typedef std::pair<int,string> pairIntString;
	typedef std::pair<string,Real> pairStringReal;
	typedef vector<std::pair<string,Real>> vecPairStringReal;
	PAIR_TUPLE_CONV(pairIntString);
	PAIR_TUPLE_CONV(pairStringReal);

	// StrArrayMap (typedef for std::map<string,numpy_boost>) → python dictionary
	//custom_StrArrayMap_to_dict();
	// register from-python converter and to-python converter

	to_python_converter<vector<vector<string> >,custom_vvector_to_list<string> >();
	//to_python_converter<std::list<shared_ptr<Functor> >, custom_list_to_list<shared_ptr<Functor> > >();
	//to_python_converter<std::list<shared_ptr<Functor> >, custom_list_to_list<shared_ptr<Functor> > >();

	// this somehow did not work really...
	//to_python_converter<vector<py::object>,custom_vector_to_list<py::object>>();

	// don't return array of nodes as lists, each DemField.nodes[0] operation must create the list first,
	// pick the element, and throw it away; since node lists are typically long, create a custom class
	// using indexing suite (version 1; never found out how is the allegedly superior version 2
	// supposed to be used):
	// http://stackoverflow.com/questions/6157409/stdvector-to-boostpythonlist

	py::class_<vector<shared_ptr<Node> > >("NodeList").def(py::vector_indexing_suite<vector<shared_ptr<Node> >, /*NoProxy, shared_ptr provides proxy semantics already */true>()).def_pickle(VectorPickle<vector<shared_ptr<Node>>>());

	// register 2-way conversion between c++ vector and python homogeneous sequence (list/tuple) of corresponding type
	#define VECTOR_SEQ_CONV(Type) custom_vector_from_seq<Type>();  to_python_converter<vector<Type>, custom_vector_to_list<Type> >();
		VECTOR_SEQ_CONV(int);
		VECTOR_SEQ_CONV(bool);
		VECTOR_SEQ_CONV(Real);
		#if 0
			VECTOR_SEQ_CONV(Se3r);
		#endif
		VECTOR_SEQ_CONV(Vector2r);
		VECTOR_SEQ_CONV(Vector2i);
		VECTOR_SEQ_CONV(Vector3r);
		VECTOR_SEQ_CONV(Vector3i);
		VECTOR_SEQ_CONV(Vector6r);
		VECTOR_SEQ_CONV(Vector6i);
		VECTOR_SEQ_CONV(VectorXr);
		VECTOR_SEQ_CONV(Matrix3r);
		VECTOR_SEQ_CONV(Quaternionr);
		VECTOR_SEQ_CONV(string);
		VECTOR_SEQ_CONV(pairIntString);
		VECTOR_SEQ_CONV(pairStringReal);
		VECTOR_SEQ_CONV(vecPairStringReal); // vector<vector<pair<string,Real>>>
		// VECTOR_SEQ_CONV(shared_ptr<Node>);
		custom_vector_from_seq<shared_ptr<Node> >(); // allow assignments vector<shared_ptr<Node> >=[list of nodes]
		VECTOR_SEQ_CONV(shared_ptr<NodeData>);
		VECTOR_SEQ_CONV(shared_ptr<ScalarRange>);
		VECTOR_SEQ_CONV(shared_ptr<Field>);
		VECTOR_SEQ_CONV(shared_ptr<Particle>);
		VECTOR_SEQ_CONV(shared_ptr<Contact>);
		VECTOR_SEQ_CONV(shared_ptr<Material>);

		VECTOR_SEQ_CONV(shared_ptr<BoundFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<CGeomFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<CPhysFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<LawFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<IntraFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<LawTesterStage>);

		VECTOR_SEQ_CONV(shared_ptr<Engine>);
		VECTOR_SEQ_CONV(shared_ptr<Object>);
		#ifdef WOO_OPENGL
			VECTOR_SEQ_CONV(shared_ptr<GlShapeFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlNodeFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlBoundFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlExtraDrawer>);
			VECTOR_SEQ_CONV(shared_ptr<GlCPhysFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlFieldFunctor>);
		#if 0
			VECTOR_SEQ_CONV(shared_ptr<GlStateFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlCGeomFunctor>);
		#endif
		#endif
	#undef VECTOR_SEQ_CONV

	#if 0
		import_array();
		to_python_converter<numpy_boost<Real,1>, custom_numpyBoost_to_py<Real,1> >();
		to_python_converter<numpy_boost<Real,2>, custom_numpyBoost_to_py<Real,2> >();
		to_python_converter<numpy_boost<int,1>, custom_numpyBoost_to_py<int,1> >();
		to_python_converter<numpy_boost<int,2>, custom_numpyBoost_to_py<int,2> >();
	#endif

}





