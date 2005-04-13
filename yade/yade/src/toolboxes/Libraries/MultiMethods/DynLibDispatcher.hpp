/***************************************************************************
 *   Copyright (C) 2005 by Janek Kozicki                                   *
 *   cosurgi@berlios.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DYNLIB_DISPATCHER_HPP
#define DYNLIB_DISPATCHER_HPP

#include "Indexable.hpp"
#include "ClassFactory.hpp"
#include "Serializable.hpp"
#include "MultiMethodsExceptions.hpp"
#include "Functor.hpp"
#include "Typelist.hpp"
#include "TypeManip.hpp"
#include "NullType.hpp"

#include <vector>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>

///
/// base classes involved in multiple dispatch must be derived from Indexable
///


///////////////////////////////////////////////////////////////////////////////////////////////////
/// base template for all dispatchers								///
///////////////////////////////////////////////////////////////////////////////////////////////////

template 
<
	class BaseClass,	//	a typelist with base classess involved in the dispatch (or single class, for 1D )
				// 		FIXME : should use shared_ptr references, like this: DynLibDispatcher< TYPELIST_2( shared_ptr<ActionParameter>& , shared_ptr<Body>& ) , ....
	class Executor,		//	class which gives multivirtual function
	class ResultType,	//	type returned by multivirtual function
	class TList,		//	typelist of arguments passed to multivirtual function
				//	WARNING: first arguments must be boost::shared_ptr<BaseClass>, for details see FunctorWrapper
				
	bool autoSymmetry=true	//	true -	the function called is always the same,
				//		only order of arguments is rearranged
				//		to make correct function call, 
				//		only go() is called
				//		
				//	false -	the function called is always different.
				//		arguments order is not rearranged
				//		go(), and goReverse() are called, respectively
				//
>
class DynLibDispatcher
{
	private:
		// this template recursively defines a type for callBacks matrix, with required number of dimensions
		template<class T > struct Matrix
		{
			typedef Loki::NullType ResultIterator;
			typedef Loki::NullType ResultIteratorInt;
		};
		template<class Head >
		struct Matrix< Loki::Typelist< Head, Loki::NullType > >
		{
			typedef typename std::vector< boost::shared_ptr< Executor > > 		Result;
			typedef typename std::vector< int > 					ResultInt;
			typedef typename std::vector< boost::shared_ptr< Executor > >::iterator ResultIterator;
			typedef typename std::vector< int >::iterator 				ResultIteratorInt;
		};
		template<class Head, class Tail >
		struct Matrix< Loki::Typelist< Head, Tail > >
		{
			// recursive typedef to get matrix of required dimensions
			typedef typename Matrix< Tail >::Result 		InsideType;
			typedef typename Matrix< Tail >::ResultInt 		InsideTypeInt;
			typedef typename std::vector< InsideType > 		Result;
			typedef typename std::vector< InsideTypeInt > 		ResultInt;
			typedef typename std::vector< InsideType >::iterator 	ResultIterator;
			typedef typename std::vector< InsideTypeInt >::iterator ResultIteratorInt;
		};
		
		// this template helps declaring iterators for each dimension of callBacks matrix
		template<class T > struct GetTail
		{
			typedef Loki::NullType Result;
		};
		template<class Head, class Tail>
		struct GetTail< Loki::Typelist< Head , Tail > >
		{
			typedef Tail Result;
		};
		template<class Head >
		struct GetTail< Loki::Typelist< Head , Loki::NullType > >
		{
			typedef Loki::NullType Result;
		};
		template <typename G, typename T, typename U>
		struct Select
		{
			typedef T Result;
		};
		template <typename T, typename U>
		struct Select<Loki::NullType, T, U>
		{
			typedef U Result;
		};
    

		typedef typename Loki::TL::Append<  Loki::NullType , BaseClass >::Result BaseClassList;
		typedef typename Loki::TL::TypeAtNonStrict<BaseClassList , 0>::Result	BaseClass1;  // 1D
		typedef typename Loki::TL::TypeAtNonStrict<BaseClassList , 1>::Result	BaseClass2;  // 2D
		typedef typename Loki::TL::TypeAtNonStrict<BaseClassList , 2>::Result	BaseClass3;  // 3D
		
		typedef typename GetTail< BaseClassList >::Result			Tail2; // 2D
		typedef typename GetTail< Tail2 >::Result				Tail3; // 3D
		typedef typename GetTail< Tail3 >::Result				Tail4; // 4D ...
		
		typedef typename Matrix< BaseClassList >::ResultIterator 		Iterator2; // outer iterator 2D
		typedef typename Matrix< Tail2 >::ResultIterator			Iterator3; // inner iterator 3D
		typedef typename Matrix< Tail3 >::ResultIterator			Iterator4; // more inner iterator 4D
		
		typedef typename Matrix< BaseClassList >::ResultIteratorInt		IteratorInfo2;
		typedef typename Matrix< Tail2 >::ResultIteratorInt			IteratorInfo3;
		typedef typename Matrix< Tail3 >::ResultIteratorInt			IteratorInfo4;
		
		typedef typename Matrix< BaseClassList >::Result MatrixType;
		typedef typename Matrix< BaseClassList >::ResultInt MatrixIntType;
		MatrixType callBacks;		// multidimensional matrix that stores functors ( 1D, 2D, 3D, 4D, ....)
		MatrixIntType callBacksInfo;	// multidimensional matrix for extra information about functors in the matrix
						//                       currently used to remember if it is reversed functor
						
		// ParmNReal is defined to avoid ambigious function call for different dimensions of multimethod
		typedef Loki::FunctorImpl<ResultType, TList > Impl;
		typedef TList ParmList;
		typedef 	 Loki::NullType Parm1; 						// it's always at least 1D
		typedef typename Impl::Parm2 Parm2Real;
		typedef typename Select< Tail2 , Loki::NullType , Parm2Real >::Result Parm2; 	// 2D
		typedef typename Impl::Parm3 Parm3Real;
		typedef typename Select< Tail3 , Loki::NullType , Parm3Real >::Result Parm3; 	// 3D - to have 3D just working, without symmetry handling - change this line to be: typedef typename Impl::Parm3 Parm3;
		typedef typename Impl::Parm4 Parm4Real;
		typedef typename Select< Tail4 , Loki::NullType , Parm4Real >::Result Parm4;	// 4D - same as above
		typedef typename Impl::Parm5 Parm5;
		typedef typename Impl::Parm6 Parm6;
		typedef typename Impl::Parm7 Parm7;
		typedef typename Impl::Parm8 Parm8;
		typedef typename Impl::Parm9 Parm9;
		typedef typename Impl::Parm10 Parm10;
		typedef typename Impl::Parm11 Parm11;
		typedef typename Impl::Parm12 Parm12;
		typedef typename Impl::Parm13 Parm13;
		typedef typename Impl::Parm14 Parm14;
		typedef typename Impl::Parm15 Parm15;
	
	// Serialization stuff.. - FIXME - maybe this should be done in separate class...
		bool deserializing;
	protected:
		std::vector<std::vector<std::string> >	functorNames;
		std::list<boost::shared_ptr<Executor> > functorArguments;
		typedef typename std::list<boost::shared_ptr<Executor> >::iterator executorListIterator;
			
// 			virtual void registerAttributes()
// 			{
// 				REGISTER_ATTRIBUTE(functorNames);
// 				if(functors.size() != 0)
// 					REGISTER_ATTRIBUTE(functors);
// 			}
			
 	public:
		DynLibDispatcher()
		{
			// FIXME - static_assert( typeid(BaseClass1) == typeid(Parm1) ); // 1D
			// FIXME - static_assert( typeid(BaseClass2) == typeid(Parm2) ); // 2D
			callBacks.clear();
			callBacksInfo.clear();
			functorNames.clear();
			functorArguments.clear();
			deserializing = false;
		};
		
		shared_ptr<Executor> makeExecutor(string libName)
		{
			boost::shared_ptr<Executor> executor;
			try
			{
				executor = boost::dynamic_pointer_cast<Executor>(ClassFactory::instance().createShared(libName));
				if(! executor )
				{
					//static bool first=true;
					//if(first)
					//{
					//	cerr << "WARNING: DynLibDispatcher::makeExecutor, dynamic_pointer_cast on library: " << libName << " failed - trying static_pointer_cast instead. Check that you have a virtual desctructor.\n";
					//	first = false;
					//}
					executor = boost::static_pointer_cast<Executor>(ClassFactory::instance().createShared(libName));
				}
			}
			catch (FactoryCantCreate& fe)
			{
				std::string error = string(fe.what()) 
						+ " -- " + MultiMethodsExceptions::NotExistingClass + libName;	
				throw MultiMethodsNotExistingClass(error.c_str());
			}	
			assert(executor);
			
			return executor;
		}
		
		void storeFunctorArguments(boost::shared_ptr<Executor>& ex)
		{
			if(! ex) return;
			bool dupe = false;
			executorListIterator it    = functorArguments.begin();
			executorListIterator itEnd = functorArguments.end();
			for( ; it != itEnd ; ++it )
				if( (*it)->getClassName() == ex->getClassName() )
					dupe = true;
			
			if(! dupe) functorArguments.push_back(ex);
		}
		
		boost::shared_ptr<Executor> findFunctorArguments(std::string libName)
		{
			executorListIterator it    = functorArguments.begin();
			executorListIterator itEnd = functorArguments.end();
			for( ; it != itEnd ; ++it )
				if( (*it)->getClassName() == libName )
					return *it;
			
			return boost::shared_ptr<Executor>();
		}

////////////////////////////////////////////////////////////////////////////////
// add multivirtual function to 1D
////////////////////////////////////////////////////////////////////////////////
		void postProcessDispatcher1D(bool d)
		{
			if(d)
			{
				deserializing = true;
				for(unsigned int i=0;i<functorNames.size();i++)
					add(functorNames[i][0],functorNames[i][1],findFunctorArguments(functorNames[i][1]));
				deserializing = false;
			}
		}
		
		void storeFunctorName(	  const std::string& baseClassName
					, const std::string& libName
					, boost::shared_ptr<Executor>& ex)
		{
			std::vector<std::string> v;
			v.push_back(baseClassName);
			v.push_back(libName);
			functorNames.push_back(v);
			storeFunctorArguments(ex);
		}
		
		void add(	  std::string baseClassName
				, std::string libName
				, boost::shared_ptr<Executor> ex = boost::shared_ptr<Executor>()
			)
		{
			// create base class, to access its index. (we can't access static variable, because
			// the class might not exist in memory at all, and we have to load dynamic library,
			// so that a static variable is created and accessible)
			boost::shared_ptr<BaseClass1> baseClass = 
				boost::dynamic_pointer_cast<BaseClass1>(ClassFactory::instance().createShared(baseClassName));
			// this is a strange tweak without which it won't work.
			boost::shared_ptr<Indexable> base = boost::dynamic_pointer_cast<Indexable>(baseClass);
		
			assert(base);
			int& index = base->getClassIndex();
 			assert (index != -1);
			
			int maxCurrentIndex = base->getMaxCurrentlyUsedClassIndex();
			callBacks.resize( maxCurrentIndex+1 );	// make sure that there is a place for new Functor

			boost::shared_ptr<Executor> executor = ex ? ex : makeExecutor(libName);	// create the requested functor
			callBacks[index] = executor;
			
			if(!deserializing) storeFunctorName(baseClassName,libName,ex);
			
			#ifdef DEBUG
				cerr <<" New class added to DynLibDispatcher 1D: " << libName << endl;
			#endif
		};
		
		bool locateMultivirtualFunctor1D(int& index, boost::shared_ptr<BaseClass1>& base)
		{
			index = base->getClassIndex();
			assert( index >= 0 && (unsigned int)( index ) < callBacks.size());
			if( callBacks[index] )
				return true;
			
			int depth=1;
			int index_tmp = base->getBaseClassIndex(depth);
			while(1)
				if(index_tmp == -1)
					return false;
				else
					if(callBacks[index_tmp])
					{
						callBacksInfo[index] = callBacksInfo[index_tmp];
						callBacks    [index] = callBacks    [index_tmp];
						return true;
					}
					else
						index_tmp = base->getBaseClassIndex(++depth);
		
			return false; // FIXME - this line should be not needed
		}

		
////////////////////////////////////////////////////////////////////////////////
// add multivirtual function to 2D
////////////////////////////////////////////////////////////////////////////////
		void postProcessDispatcher2D(bool d)
		{
			if(d)
			{
				deserializing = true;
				for(unsigned int i=0;i<functorNames.size();i++)
					add(functorNames[i][0],functorNames[i][1],functorNames[i][2],findFunctorArguments(functorNames[i][2]));
				deserializing = false;
			}
		}
		
		void storeFunctorName(	  const std::string& baseClassName1
					, const std::string& baseClassName2
					, const std::string& libName
					, boost::shared_ptr<Executor>& ex) // 2D
		{
			std::vector<std::string> v;
			v.push_back(baseClassName1);
			v.push_back(baseClassName2);
			v.push_back(libName);
			functorNames.push_back(v);
			storeFunctorArguments(ex);
		}
		
		void add(	  std::string baseClassName1
				, std::string baseClassName2
				, std::string libName
				, boost::shared_ptr<Executor> ex = boost::shared_ptr<Executor>()
			)
		{
			boost::shared_ptr<BaseClass1> baseClass1 =
				boost::dynamic_pointer_cast<BaseClass1>(ClassFactory::instance().createShared(baseClassName1));
			boost::shared_ptr<BaseClass2> baseClass2 =
				boost::dynamic_pointer_cast<BaseClass2>(ClassFactory::instance().createShared(baseClassName2));
			boost::shared_ptr<Indexable> base1 = boost::dynamic_pointer_cast<Indexable>(baseClass1);
			boost::shared_ptr<Indexable> base2 = boost::dynamic_pointer_cast<Indexable>(baseClass2);
			
			assert(base1);
			assert(base2);

 			int& index1 = base1->getClassIndex();
			assert (index1 != -1);
 			
			int& index2 = base2->getClassIndex();
 			assert(index2 != -1);
	
			if( typeid(BaseClass1) == typeid(BaseClass2) )
				assert(base1->getMaxCurrentlyUsedClassIndex() == base2->getMaxCurrentlyUsedClassIndex());
	
			int maxCurrentIndex1 = base1->getMaxCurrentlyUsedClassIndex();
			int maxCurrentIndex2 = base2->getMaxCurrentlyUsedClassIndex();
			
			callBacks.resize( maxCurrentIndex1+1 );		// resizing callBacks table
			callBacksInfo.resize( maxCurrentIndex1+1 );
			for( Iterator2 ci = callBacks.begin() ; ci != callBacks.end() ; ++ci )
				ci->resize(maxCurrentIndex2+1);
			for( IteratorInfo2 cii = callBacksInfo.begin() ; cii != callBacksInfo.end() ; ++cii )
				cii->resize(maxCurrentIndex2+1);

			boost::shared_ptr<Executor> executor = ex ? ex : makeExecutor(libName);	// create the requested functor
			
			if( typeid(BaseClass1) == typeid(BaseClass2) ) // both base classes are the same
			{
				callBacks	[index2][index1] = executor;
				callBacks	[index1][index2] = executor;
				
				string order		= baseClassName1 + " " + baseClassName2;
				string reverseOrder	= baseClassName2 + " " + baseClassName1;
				
				if( autoSymmetry || executor->checkOrder() == order ) // if you want autoSymmetry, you don't have to DEFINE_FUNCTOR_ORDER_2D
				{
					callBacksInfo	[index2][index1] = 1; // this is reversed call
					callBacksInfo	[index1][index2] = 0;
				}
				else if( executor->checkOrder() == reverseOrder )
				{
					callBacksInfo	[index2][index1] = 0;
					callBacksInfo	[index1][index2] = 1; // this is reversed call
				} else
				{
					std::string err = MultiMethodsExceptions::UndefinedOrder + libName;
					throw MultiMethodsUndefinedOrder(err.c_str());
				}
			}
			else // classes are different, no symmetry possible
			{
				callBacks	[index1][index2] = executor;
				callBacksInfo	[index1][index2] = 0;
			}
			
			if(!deserializing) storeFunctorName(baseClassName1,baseClassName2,libName,ex);
			
			#ifdef DEBUG
				cerr <<" New class added to MultiMethodsManager 2D: " << libName << endl;
			#endif
		}
		
		bool locateMultivirtualFunctor2D(int& index1, int& index2, boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2)
		{
			index1 = base1->getClassIndex();
			index2 = base2->getClassIndex();
			assert( index1 >= 0 && (unsigned int)( index1 ) < callBacks.size() &&
				index2 >= 0 && (unsigned int)( index2 ) < callBacks[index1].size() );
				
			if(callBacks[index1][index2])
				return true;

			int depth1=1;
			int depth2=1;
			int index1_tmp = base1->getBaseClassIndex(depth1);
			int index2_tmp = base2->getBaseClassIndex(depth2);
			
			if( index1_tmp == -1 )
				while(1)
					if(index2_tmp == -1)
						return false;
					else
						if(callBacks[index1    ][index2_tmp])
						{
							callBacksInfo[index1][index2] = callBacksInfo[index1][index2_tmp];
							callBacks    [index1][index2] = callBacks    [index1][index2_tmp];
//							index2 = index2_tmp;
							return true;
						}
						else
							index2_tmp = base2->getBaseClassIndex(++depth2);
			else
			if( index2_tmp == -1 )
				while(1)
					if(index1_tmp == -1)
						return false;
					else
						if(callBacks[index1_tmp][index2    ])
						{
							callBacksInfo[index1][index2] = callBacksInfo[index1_tmp][index2];
							callBacks    [index1][index2] = callBacks    [index1_tmp][index2];
//							index1 = index1_tmp;
							return true;
						}
						else
							index1_tmp = base1->getBaseClassIndex(++depth1);
			else
			if( index1_tmp != -1 && index2_tmp != -1 )
				cerr << "DynLibDispatcher: ambigious dispatch, could not determine which multivirtual function should be called\n";
			
			return false;
		};

		
////////////////////////////////////////////////////////////////////////////////
// add multivirtual function to 3D
////////////////////////////////////////////////////////////////////////////////

//  to be written when needed.... - not too difficult
		void postProcessDispatcher3D(bool d)
		{
		}
		
		void storeFunctorName(const std::string& str1,const std::string& str2,const std::string& str3,const std::string& str4,boost::shared_ptr<Executor>& ex)
		{
		}
		
		void add(std::string baseClassName1 , std::string baseClassName2 , std::string baseClassName3 , std::string libName)
		{
		}
		
		bool locateMultivirtualFunctor3D(int& index1, int& index2, int& index3,
			boost::shared_ptr<BaseClass1>& base1, boost::shared_ptr<BaseClass2>& base2, boost::shared_ptr<BaseClass3>& base3 )
		{
			return false;
		}
		
////////////////////////////////////////////////////////////////////////////////
// calling multivirtual function, 1D
////////////////////////////////////////////////////////////////////////////////

		ResultType operator() (boost::shared_ptr<BaseClass1>& base)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8, Parm9 p9)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8, p9);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8, Parm9 p9,
									Parm10 p10)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8, p9, p10);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8, Parm9 p9,
									Parm10 p10, Parm11 p11)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8, Parm9 p9,
									Parm10 p10, Parm11 p11, Parm12 p12)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8, Parm9 p9,
									Parm10 p10, Parm11 p11, Parm12 p12, Parm13 p13)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8, Parm9 p9,
									Parm10 p10, Parm11 p11, Parm12 p12, Parm13 p13, Parm14 p14)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
			else	return ResultType();
		}
	
		ResultType operator() (boost::shared_ptr<BaseClass1>& base, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7, Parm8 p8, Parm9 p9,
									Parm10 p10, Parm11 p11, Parm12 p12, Parm13 p13, Parm14 p14, Parm15 p15)
		{
			int index;
			if( locateMultivirtualFunctor1D(index,base) )
				return (callBacks[index])->go(base, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15);
			else	return ResultType();
		}
	
////////////////////////////////////////////////////////////////////////////////
// calling multivirtual function, 2D, 
// symmetry handling in private struct
////////////////////////////////////////////////////////////////////////////////
	private:
		template< bool useSymmetry, class BaseClassTrait1, class BaseClassTrait2, class ParmTrait3, class ParmTrait4, class ParmTrait5, class ParmTrait6,
				class ParmTrait7, class ParmTrait8, class ParmTrait9, class ParmTrait10, class ParmTrait11, class ParmTrait12, class ParmTrait13,
				class ParmTrait14, class ParmTrait15 >
		struct InvocationTraits
		{
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2 )
			{	
				return ex->goReverse	(base1, base2);	
			}	
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3)
			{
				return ex->goReverse	(base1, base2, p3);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4)
			{
				return ex->goReverse	(base1, base2, p3, p4);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5)
			{
				return ex->goReverse	(base1, base2, p3, p4, p5);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8, p9);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12, ParmTrait13 p13)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12, ParmTrait13 p13, ParmTrait14 p14)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex, boost::shared_ptr<BaseClassTrait1> base1, boost::shared_ptr<BaseClassTrait2> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12, ParmTrait13 p13, ParmTrait14 p14, ParmTrait15 p15)
			{	
				return ex->goReverse	(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15);
			}
		};
		template< class BaseClassTrait, class ParmTrait3, class ParmTrait4, class ParmTrait5, class ParmTrait6
					, class ParmTrait7, class ParmTrait8, class ParmTrait9, class ParmTrait10, class ParmTrait11, class ParmTrait12
					, class ParmTrait13, class ParmTrait14, class ParmTrait15 >
		struct InvocationTraits< true , BaseClassTrait, BaseClassTrait, ParmTrait3, ParmTrait4, ParmTrait5, ParmTrait6
					, ParmTrait7, ParmTrait8, ParmTrait9, ParmTrait10, ParmTrait11, ParmTrait12
					, ParmTrait13, ParmTrait14, ParmTrait15 >
		{
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2 )
			{
				return ex->go		(base2, base1 );
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3)
			{
				return ex->go		(base2, base1, p3);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4)
			{
				return ex->go		(base2, base1, p3, p4);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5)
			{	
				return ex->go		(base2, base1, p3, p4, p5);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8, p9);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8, p9, p10);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8, p9, p10, p11);
			}	
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12, ParmTrait13 p13)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12, ParmTrait13 p13, ParmTrait14 p14)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
			}
			static ResultType doDispatch( boost::shared_ptr<Executor>& ex , boost::shared_ptr<BaseClassTrait> base1, boost::shared_ptr<BaseClassTrait> base2, ParmTrait3 p3,
						ParmTrait4 p4, ParmTrait5 p5, ParmTrait6 p6, ParmTrait7 p7, ParmTrait8 p8, ParmTrait9 p9, ParmTrait10 p10, ParmTrait11 p11,
						ParmTrait12 p12, ParmTrait13 p13, ParmTrait14 p14, ParmTrait15 p15)
			{	
				return ex->go		(base2, base1, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15);
			}
		};

////////////////////////////////////////////////////////////////////////////////
// calling multivirtual function, 2D, public interface
////////////////////////////////////////////////////////////////////////////////
	public:
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2 );
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3);
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4 );
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5 );
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6 );
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7 );
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8);
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8, Parm9 p9)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8, p9 );
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8, p9 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8, Parm9 p9, Parm10 p10)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8, p9, p10);
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8, Parm9 p9, Parm10 p10, Parm11 p11)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8, Parm9 p9, Parm10 p10, Parm11 p11, Parm12 p12)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12 );
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8, Parm9 p9, Parm10 p10, Parm11 p11, Parm12 p12, Parm13 p13)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8, Parm9 p9, Parm10 p10, Parm11 p11, Parm12 p12, Parm13 p13, Parm14 p14)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14 );
			}
			else	return ResultType();
		}
		
		ResultType operator() (boost::shared_ptr<BaseClass1>& base1,boost::shared_ptr<BaseClass2>& base2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6,
						Parm7 p7, Parm8 p8, Parm9 p9, Parm10 p10, Parm11 p11, Parm12 p12, Parm13 p13, Parm14 p14, Parm15 p15)
		{
			int index1, index2;
			if( locateMultivirtualFunctor2D(index1,index2,base1,base2) )
			{
				if(callBacksInfo[index1][index2])	// reversed
				{
					typedef InvocationTraits<autoSymmetry, BaseClass1, BaseClass2, Parm3, Parm4, Parm5, Parm6,
						Parm7, Parm8, Parm9, Parm10, Parm11, Parm12, Parm13, Parm14, Parm15 > CallTraits;
					return CallTraits::doDispatch( callBacks[index1][index2] , base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15);
				}
				else
					return (callBacks[index1][index2] )->go			(base1, base2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15 );
			}
			else	return ResultType();
		}
		
////////////////////////////////////////////////////////////////////////////////
// calling multivirtual function, 3D
////////////////////////////////////////////////////////////////////////////////

// to be continued ...

};

#endif // DYNLIB_DISPATCHER_HPP

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

