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
	#include<yade/lib/pyutil/numpy.hpp>
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

#include<yade/lib/base/Types.hpp>

#include<yade/lib/base/Math.hpp>
#include<yade/lib/base/openmp-accu.hpp>

#include<yade/core/Field.hpp>
#include<yade/core/Scene.hpp>

#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/IntraForce.hpp>

#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/core/MatchMaker.hpp>

#if 0
#include<yade/core/Engine.hpp>
#include<yade/pkg/common/Dispatching.hpp>
#include<yade/pkg/dem/SpherePack.hpp>
#include<yade/pkg/common/KinematicEngines.hpp>
#endif


#ifdef YADE_OPENGL
	#include<yade/pkg/gl/Functors.hpp>
	#include<yade/pkg/gl/Renderer.hpp>
	#include<yade/pkg/gl/NodeGlRep.hpp>
#endif

#include <boost/python/suite/indexing/vector_indexing_suite.hpp>



using namespace py; // = boost::python


/* two-way se3 handling */
struct custom_se3_to_tuple{
	static PyObject* convert(const Se3r& se3){
		py::tuple ret=py::make_tuple(se3.position,se3.orientation);
		return incref(ret.ptr());
	}
};
struct custom_Se3r_from_seq{
	custom_Se3r_from_seq(){
		 converter::registry::push_back(&convertible,&construct,type_id<Se3r>());
	}
	static void* convertible(PyObject* obj_ptr){
		 if(!PySequence_Check(obj_ptr)) return 0;
		 if(PySequence_Size(obj_ptr)!=2 && PySequence_Size(obj_ptr)!=7) return 0;
		 return obj_ptr;
	}
	static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data){
		void* storage=((converter::rvalue_from_python_storage<Se3r>*)(data))->storage.bytes;
		new (storage) Se3r; Se3r* se3=(Se3r*)storage;
		if(PySequence_Size(obj_ptr)==2){ // from vector and quaternion
			se3->position=extract<Vector3r>(PySequence_GetItem(obj_ptr,0));
			se3->orientation=extract<Quaternionr>(PySequence_GetItem(obj_ptr,1));
		} else if(PySequence_Size(obj_ptr)==7){ // 3 vector components, 3 axis components, angle
			se3->position=Vector3r(extract<Real>(PySequence_GetItem(obj_ptr,0)),extract<Real>(PySequence_GetItem(obj_ptr,1)),extract<Real>(PySequence_GetItem(obj_ptr,2)));
			Vector3r axis=Vector3r(extract<Real>(PySequence_GetItem(obj_ptr,3)),extract<Real>(PySequence_GetItem(obj_ptr,4)),extract<Real>(PySequence_GetItem(obj_ptr,5)));
			Real angle=extract<Real>(PySequence_GetItem(obj_ptr,6));
			se3->orientation=Quaternionr(AngleAxisr(angle,axis));
		} else throw std::logic_error(__FILE__ ": First, the sequence size for Se3r object was 2 or 7, but now is not? (programming error, please report!");
		data->convertible=storage;
	}
};


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
	static PyObject* convert(const std::vector<std::vector<T> >& vv){
		py::list ret; FOREACH(const std::vector<T>& v, vv){
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
/*** c++-list to python-list */
template<typename containedType>
struct custom_vector_to_list{
	static PyObject* convert(const std::vector<containedType>& v){
		py::list ret; FOREACH(const containedType& e, v) ret.append(e);
		return incref(ret.ptr());
	}
};
template<typename containedType>
struct custom_vector_from_seq{
	custom_vector_from_seq(){ converter::registry::push_back(&convertible,&construct,type_id<std::vector<containedType> >()); }
	static void* convertible(PyObject* obj_ptr){
		// the second condition is important, for some reason otherwise there were attempted conversions of Body to list which failed afterwards.
		if(!PySequence_Check(obj_ptr) || !PyObject_HasAttrString(obj_ptr,"__len__")) return 0;
		return obj_ptr;
	}
	static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data){
		 void* storage=((converter::rvalue_from_python_storage<std::vector<containedType> >*)(data))->storage.bytes;
		 new (storage) std::vector<containedType>();
		 std::vector<containedType>* v=(std::vector<containedType>*)(storage);
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

#if 0
template<typename T>
std::string vectorRepr(const vector<T>& v){ std::string ret("["); for(size_t i=0; i<v.size(); i++) { if(i>0) ret+=","; ret+=lexical_cast<string>(v[i]); } return ret+"]"; }
template<>
std::string vectorRepr(const vector<std::string>& v){ std::string ret("["); for(size_t i=0; i<v.size(); i++) { if(i>0) ret+=","; ret+="'"+lexical_cast<string>(v[i])+"'"; } return ret+"]"; }

// is not picked up?
bool operator<(const Vector3r& a, const Vector3r& b){ return a[0]<b[0]; }
#endif


BOOST_PYTHON_MODULE(_customConverters){

	// syntax ??
	//   http://language-binding.net/pyplusplus/documentation/indexing_suite_v2.html.html#container_proxy
	//   http://www.mail-archive.com/cplusplus-sig@python.org/msg00862.html
	//class_<indexing::container_proxy<std::vector<string> >,bases<class_<std::vector<string> > > >("vecStr").def(indexing::container_suite<indexing::container_proxy<std::vector<string> > >());
	//class_<std::vector<string> >("vecStr").def(indexing::container_suite<std::vector<string> >());

	#if 0
		custom_vector_from_seq<string>(); class_<std::vector<string> >("vector_" "string").def(indexing::container_suite<std::vector<string> >()).def("__repr__",&vectorRepr<string>);
		custom_vector_from_seq<int>(); class_<std::vector<int> >("vector_" "int").def(indexing::container_suite<std::vector<int> >()).def("__repr__",&vectorRepr<int>);
		custom_vector_from_seq<Real>(); class_<std::vector<Real> >("vector_" "Real").def(indexing::container_suite<std::vector<Real> >()).def("__repr__",&vectorRepr<Real>);
		// this needs operator< for Vector3r (defined above, but not picked up for some reason)
		custom_vector_from_seq<Vector3r>(); class_<std::vector<Vector3r> >("vector_" "Vector3r").def(indexing::container_suite<std::vector<Vector3r> >()).def("__repr__",&vectorRepr<Vector3r>);
	#endif

	custom_Se3r_from_seq(); to_python_converter<Se3r,custom_se3_to_tuple>();

	custom_OpenMPAccumulator_from_float(); to_python_converter<OpenMPAccumulator<Real>, custom_OpenMPAccumulator_to_float>(); 
	custom_OpenMPAccumulator_from_int(); to_python_converter<OpenMPAccumulator<int>, custom_OpenMPAccumulator_to_int>(); 

	custom_ptrMatchMaker_from_float();

	// StrArrayMap (typedef for std::map<std::string,numpy_boost>) → python dictionary
	//custom_StrArrayMap_to_dict();
	// register from-python converter and to-python converter

	to_python_converter<std::vector<std::vector<std::string> >,custom_vvector_to_list<std::string> >();
	//to_python_converter<std::list<shared_ptr<Functor> >, custom_list_to_list<shared_ptr<Functor> > >();
	//to_python_converter<std::list<shared_ptr<Functor> >, custom_list_to_list<shared_ptr<Functor> > >();

	// don't return array of nodes as lists, each DemField.nodes[0] operation must create the list first,
	// pick the element, and throw it away; since node lists are typically long, create a custom class
	// using indexing suite (version 1; never found out how is the allegedly superior version 2
	// supposed to be used):
	// http://stackoverflow.com/questions/6157409/stdvector-to-boostpythonlist
	py::class_<std::vector<shared_ptr<Node> > >("NodeList").def(py::vector_indexing_suite<std::vector<shared_ptr<Node> >, /*NoProxy, shared_ptr provides proxy semantics already */true>());

	// register 2-way conversion between c++ vector and python homogeneous sequence (list/tuple) of corresponding type
	#define VECTOR_SEQ_CONV(Type) custom_vector_from_seq<Type>();  to_python_converter<std::vector<Type>, custom_vector_to_list<Type> >();
		VECTOR_SEQ_CONV(int);
		VECTOR_SEQ_CONV(bool);
		VECTOR_SEQ_CONV(Real);
		VECTOR_SEQ_CONV(Se3r);
		VECTOR_SEQ_CONV(Vector2r);
		VECTOR_SEQ_CONV(Vector2i);
		VECTOR_SEQ_CONV(Vector3r);
		VECTOR_SEQ_CONV(Vector3i);
		VECTOR_SEQ_CONV(Vector6r);
		VECTOR_SEQ_CONV(Vector6i);
		VECTOR_SEQ_CONV(VectorXr);
		VECTOR_SEQ_CONV(Matrix3r);
		VECTOR_SEQ_CONV(Quaternionr);
		VECTOR_SEQ_CONV(std::string);
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

		VECTOR_SEQ_CONV(shared_ptr<Engine>);
		VECTOR_SEQ_CONV(shared_ptr<Serializable>);
#if 0
		VECTOR_SEQ_CONV(shared_ptr<NodeData>);
		VECTOR_SEQ_CONV(shared_ptr<Body>);
		VECTOR_SEQ_CONV(shared_ptr<Material>);
		VECTOR_SEQ_CONV(shared_ptr<Interaction>);
		VECTOR_SEQ_CONV(shared_ptr<BoundFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<IGeomFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<IPhysFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<LawFunctor>);
		VECTOR_SEQ_CONV(shared_ptr<SpherePack>);
		VECTOR_SEQ_CONV(shared_ptr<KinematicEngine>);
#endif
		#ifdef YADE_OPENGL
			VECTOR_SEQ_CONV(shared_ptr<GlShapeFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlNodeFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlBoundFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlExtraDrawer>);
			VECTOR_SEQ_CONV(shared_ptr<GlCPhysFunctor>);
		#if 0
			VECTOR_SEQ_CONV(shared_ptr<GlStateFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlCGeomFunctor>);
			VECTOR_SEQ_CONV(shared_ptr<GlFieldFunctor>);
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





