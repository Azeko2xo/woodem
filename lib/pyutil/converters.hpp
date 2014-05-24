#pragma once 

#include<boost/python.hpp>
#include<boost/python/class.hpp>
#include<vector>
#include<list>

namespace py=boost::python;

namespace woo{
	/*** c++-list to python-list */
	template<typename containedType>
	struct custom_vector_from_seq{
		custom_vector_from_seq(){ py::converter::registry::push_back(&convertible,&construct,py::type_id<vector<containedType> >()); }
		static void* convertible(PyObject* obj_ptr){
			// the second condition is important, for some reason otherwise there were attempted conversions of Body to list which failed afterwards.
			if(!PySequence_Check(obj_ptr) || !PyObject_HasAttrString(obj_ptr,"__len__")) return 0;
			return obj_ptr;
		}
		static void construct(PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data){
			void* storage=((py::converter::rvalue_from_python_storage<vector<containedType> >*)(data))->storage.bytes;
			new (storage) vector<containedType>();
			vector<containedType>* v=(vector<containedType>*)(storage);
			int l=PySequence_Size(obj_ptr); if(l<0) abort(); /*std::cerr<<"l="<<l<<"; "<<typeid(containedType).name()<<std::endl;*/ v->reserve(l); for(int i=0; i<l; i++) { v->push_back(py::extract<containedType>(PySequence_GetItem(obj_ptr,i))); }
			data->convertible=storage;
		}
	};

	template<typename T>
	struct custom_vvector_to_list{
		static PyObject* convert(const vector<vector<T> >& vv){
			py::list ret; for(const vector<T>& v: vv){
				py::list ret2;
				for(const T& e: v) ret2.append(e);
				ret.append(ret2);
			}
			return py::incref(ret.ptr());
		}
	};

	template<typename containedType>
	struct custom_list_to_list{
		static PyObject* convert(const std::list<containedType>& v){
			py::list ret; for(const containedType& e: v) ret.append(e);
			return py::incref(ret.ptr());
		}
	};

	template<typename containedType>
	struct custom_vector_to_list{
		static PyObject* convert(const vector<containedType>& v){
			py::list ret; for(const containedType& e: v) ret.append(e);
			return py::incref(ret.ptr());
		}
	};

	/* pair-tuple converter */
	template<typename CxxPair>
	struct custom_CxxPair_to_PyTuple{
		static PyObject* convert(const CxxPair& t){ return py::incref(py::make_tuple(std::get<0>(t),std::get<1>(t)).ptr()); }
	};
	template<typename CxxPair>
	struct custom_CxxPair_from_PyTuple{
		custom_CxxPair_from_PyTuple(){ py::converter::registry::push_back(&convertible,&construct,py::type_id<CxxPair>()); }
		static void* convertible(PyObject* obj_ptr){
			if(!PySequence_Check(obj_ptr) || !PyObject_HasAttrString(obj_ptr,"__len__") || PySequence_Size(obj_ptr)!=2) return 0;
			return obj_ptr;
		}
		static void construct(PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data){
			void* storage=((py::converter::rvalue_from_python_storage<CxxPair>*)(data))->storage.bytes;
			new (storage) CxxPair();
			CxxPair* t=(CxxPair*)(storage);
			t->first=py::extract<typename CxxPair::first_type>(PySequence_GetItem(obj_ptr,0));
			t->second=py::extract<typename CxxPair::second_type>(PySequence_GetItem(obj_ptr,1));
			data->convertible=storage;
		}
	};


	template<typename T>
	void converters_cxxVector_pyList_2way(){
		custom_vector_from_seq<T>(); py::to_python_converter<vector<T>,custom_vector_to_list<T>>();
	};


#if 0
	// this defines getstate and setstate methods to support pickling on linear sequences (should work for std::list as well)
	template<class T>
	struct VectorPickle: py::pickle_suite{
		static py::list getstate(const T& tt){ py::list ret; for(const typename T::value_type& t: tt) ret.append(t); return ret; }
		static void setstate(T& tt, py::list state){ tt.clear(); for(int i=0;i<py::len(state);i++) tt.push_back(py::extract<typename T::value_type>(state[i])); }
	};
#endif



};


#if 0
template<typename T>
struct CxxVec_PySeq_Conversions_Visitor: public py::def_visitor<CxxVec_PySeq_Conversions_Visitor<T>>{
	template<class PyClass>
	void visit(PyClass& cl) const {
		cl.

	}





};
#endif
