// #include<woo/lib/object/ClassFactory.hpp>
// make core classes known to the class factory

// #include<woo/core/Cell.hpp>
#include<woo/core/Dispatcher.hpp>
#include<woo/core/EnergyTracker.hpp>
//#include<woo/core/Engine.hpp>
#include<woo/core/Functor.hpp>
#include<woo/core/DisplayParameters.hpp>
//#include<woo/core/Plot.hpp>
#include<woo/core/Test.hpp>

#include<boost/version.hpp>

// YAD3_CLASS_IMPLEMENTATION(Cell_CLASS_DESCRIPTOR)

WOO_PLUGIN(core,/*(Cell)(Dispatcher)(EnergyTracker)(Engine)(Functor)(ParallelEngine)(PeriodicEngine)(Plot)(PyRunner)*/(DisplayParameters)(WooTestClass)(WooTestPeriodicEngine));
#include<woo/core/Field.hpp>
WOO_PLUGIN(core,(Constraint)(Node)(NodeData)(Field));
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(NodeGlRep)(ScalarRange));
#endif

//BOOST_CLASS_EXPORT(OpenMPArrayAccumulator<Real>);
//BOOST_CLASS_EXPORT(OpenMPAccumulator<Real>);
