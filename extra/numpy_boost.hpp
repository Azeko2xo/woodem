/*
Copyright (c) 2008, Michael Droettboom
All rights reserved.

Licensed under the BSD license.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * The names of its contributors may not be used to endorse or
      promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __NUMPY_BOOST_HPP__
#define __NUMPY_BOOST_HPP__

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/ndarraytypes.h>
#include <boost/multi_array.hpp>
#include <boost/cstdint.hpp>
#include <complex>
#include <algorithm>

/* numpy_type_map<T>

   Provides a mapping from C++ datatypes to Numpy type
   numbers. */
namespace detail {
  template<class T>
  class numpy_type_map {
  };

#define MAKE_NUMPY_TYPE2CODE( ctype, nptype ) \
template<> \
struct numpy_type_map<ctype> {\
  static constexpr int typenum = nptype ; \
};

//template<>
//const int numpy_type_map<float>::typenum = NPY_FLOAT;
MAKE_NUMPY_TYPE2CODE( float, NPY_FLOAT );

MAKE_NUMPY_TYPE2CODE(std::complex<float> , NPY_CFLOAT)

MAKE_NUMPY_TYPE2CODE(double, NPY_DOUBLE)

MAKE_NUMPY_TYPE2CODE(std::complex<double> , NPY_CDOUBLE)

MAKE_NUMPY_TYPE2CODE(long double, NPY_LONGDOUBLE)

MAKE_NUMPY_TYPE2CODE(std::complex<long double> , NPY_CLONGDOUBLE)

MAKE_NUMPY_TYPE2CODE(boost::int8_t, NPY_INT8)

MAKE_NUMPY_TYPE2CODE(boost::uint8_t, NPY_UINT8)

MAKE_NUMPY_TYPE2CODE(boost::int16_t, NPY_INT16)

MAKE_NUMPY_TYPE2CODE(boost::uint16_t, NPY_UINT16)

MAKE_NUMPY_TYPE2CODE(boost::int32_t, NPY_INT32)

MAKE_NUMPY_TYPE2CODE(boost::uint32_t, NPY_UINT32)

MAKE_NUMPY_TYPE2CODE(boost::int64_t, NPY_INT64)

MAKE_NUMPY_TYPE2CODE(boost::uint64_t, NPY_UINT64)

}

class python_exception : public std::exception {

};

template <typename Array>
struct numpy_from_boost_array_proxy {
    Array const& source_arr;
    numpy_from_boost_array_proxy( Array const& arr):
        source_arr( arr ) {};
};

// A simple tagging mechanism to trigger construction
// from something implementing boost_array interface.
template <typename Array>
numpy_from_boost_array_proxy<Array> numpy_from_boost_array(
    Array const& arr )
{
    return numpy_from_boost_array_proxy<Array>(arr); 
}


/* An array that acts like a boost::array, but is backed by the memory
   of a Numpy array.  Provides nice C++ interface to a Numpy array
   without any copying of the data.
   
   It may be constructed one of two ways:

     1) With an existing Numpy array.  The boost::array will then
        have the data, dimensions and strides of the Numpy array.

     2) With a list of dimensions, in which case a new contiguous
        Numpy array will be created and the new boost::array will
        point to it.

     3) From another object implementing the concept boost::array.
        In this case, because no assumptions can be made, the data
        from the source object will be copied. In order to make this 
        process explicit, use the proxy numpy_from_boost_array
        
   */
template<class T, int NDims>
class numpy_boost : public boost::multi_array_ref<T, NDims>
{
public:
  typedef numpy_boost<T, NDims>            self_type;
  typedef boost::multi_array_ref<T, NDims> super;
  typedef typename super::size_type        size_type;
  typedef T*                               TPtr;

private:
  PyArrayObject* array;

  void init_from_array(PyArrayObject* a) throw() {
    /* Upon calling init_from_array, a should already have been
       incref'd for ownership by this object. */

    /* Store a reference to the Numpy array so we can DECREF it in the
       destructor. */
    array = a;

    /* Point the boost::array at the Numpy array data.

       We don't need to worry about free'ing this pointer, because it
       will always point to memory allocated as part of the data of a
       Numpy array.  That memory is managed by Python reference
       counting. */
    super::base_ = (TPtr)PyArray_DATA(a);

    /* Set the storage order.
       
       It would seem like we would want to choose C or Fortran
       ordering here based on the flags in the Numpy array.  However,
       those flags are purely informational, the actually information
       about storage order is recorded in the strides. */
    super::storage_ = boost::c_storage_order();

    /* Copy the dimensions from the Numpy array to the boost::array. */
    boost::detail::multi_array::copy_n(PyArray_DIMS(a), NDims, super::extent_list_.begin());

    /* Copy the strides from the Numpy array to the boost::array.

       Numpy strides are in bytes.  boost::array strides are in
       elements, so we need to divide. */
    for (size_t i = 0; i < NDims; ++i) {
      super::stride_list_[i] = PyArray_STRIDE(a, i) / sizeof(T);
    }

    /* index_base_list_ stores the bases of the indices in each
       dimension.  Since we want C-style and Numpy-style zero-based
       indexing, just fill it with zeros. */
    std::fill_n(super::index_base_list_.begin(), NDims, 0);

    /* We don't want any additional offsets.  If they exist, Numpy has
       already handled that for us when calculating the data pointer
       and strides. */
    super::origin_offset_ = 0;
    super::directional_offset_ = 0;

    /* Calculate the number of elements.  This has nothing to do with
       memory layout. */
    super::num_elements_ = std::accumulate(super::extent_list_.begin(),
                                           super::extent_list_.end(),
                                           size_type(1),
                                           std::multiplies<size_type>());
  }

public:
  /* Construct from an existing Numpy array */
  numpy_boost(PyObject* obj) throw (python_exception) :
    super(NULL, std::vector<typename super::index>(NDims, 0)),
    array(NULL)
  {
    PyArrayObject* a;

    a = (PyArrayObject*)PyArray_FromObject(
        obj, detail::numpy_type_map<T>::typenum, NDims, NDims);
    if (a == NULL) {
      throw python_exception();
    }

    init_from_array(a);
  }

  /* Construct from something which already has the data */
  template <typename Array>
  numpy_boost( numpy_from_boost_array_proxy<Array> const& prx ) throw (python_exception):
    super(NULL, std::vector<typename super::index>(NDims, 0)),
    array(NULL)
  {
        static_assert(Array::dimensionality==NDims,"IncorrectDimensions");
        npy_intp shape[NDims];
        PyArrayObject* a;

        std::copy_n(prx.source_arr.shape(), NDims, shape);

        a = (PyArrayObject*)PyArray_SimpleNew(
            NDims, shape, detail::numpy_type_map<T>::typenum);
        if (a == NULL) {
          throw python_exception();
        }
        std::copy_n( prx.source_arr.origin(), prx.source_arr.num_elements(), (T*)PyArray_DATA(a) );

        init_from_array(a);
  }

  numpy_boost(self_type &&other) /*throw()*/ :
    super(NULL, std::vector<typename super::index>(NDims, 0)),
    array(other.array)
  {
    // Correct way would be to move super, and then just update 
    // the array member...however, boost::multi_array does not 
    // support moving yet, so, let's stay in safe grounds.
    init_from_array(other.array);
    other.array = 0;
  }

  /* Copy constructor... ensure clone */
  numpy_boost(const self_type &other) throw() :
    super(NULL, std::vector<typename super::index>(NDims, 0)),
    array(NULL)
  {
    PyObject* cloned = PyArray_FromAny(
        other.array,
        NULL, // dtype = NULL, obtain from array
        0, 0, // just use whatever depth is already there
        NPY_C_CONTIGUOUS|NPY_ENSURECOPY,
        NULL );
    //Py_INCREF(other.array);
    init_from_array(cloned);
  }


  /* Construct a new array based on the given dimensions */
  template<class ExtentsList>
  explicit numpy_boost(const ExtentsList& extents) throw (python_exception) :
    super(NULL, std::vector<typename super::index>(NDims, 0)),
    array(NULL)
  {
    npy_intp shape[NDims];
    PyArrayObject* a;

    boost::detail::multi_array::copy_n(extents, NDims, shape);

    a = (PyArrayObject*)PyArray_SimpleNew(
        NDims, shape, detail::numpy_type_map<T>::typenum);
    if (a == NULL) {
      throw python_exception();
    }

    init_from_array(a);
  }

  /* Destructor */
  ~numpy_boost() {
    // Be aware that this object might be 
    // ready for disposal.
    if ( array != 0 )
    {
        /* Dereference the numpy array. */
        Py_XDECREF(array);
    }
  }

  /* Assignment operator */
  void operator=(const self_type &other) throw() {
    Py_INCREF(other.array);
    Py_DECREF(array);
    init_from_array(other.array);
  }

  /* Return the underlying Numpy array object.  [Borrowed
     reference] */
  PyObject*
  py_ptr() throw() {
    return (PyObject*)array;
  }
};

#endif
