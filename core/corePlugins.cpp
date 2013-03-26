// #include<woo/lib/object/ClassFactory.hpp>
// make core classes known to the class factory
#if 0
#include<woo/core/Body.hpp>
#include<woo/core/BodyContainer.hpp>
#include<woo/core/Bound.hpp>
#include<woo/core/FileGenerator.hpp>
#include<woo/core/Material.hpp>
#include<woo/core/Shape.hpp>
#include<woo/core/State.hpp>
#endif

#include<woo/core/Cell.hpp>
#include<woo/core/Dispatcher.hpp>
#include<woo/core/EnergyTracker.hpp>
#include<woo/core/Engine.hpp>
#include<woo/core/Functor.hpp>
#include<woo/core/DisplayParameters.hpp>
#include<woo/core/Plot.hpp>
#include<woo/core/Test.hpp>
//#include<woo/core/Interaction.hpp>
//#include<woo/core/InteractionContainer.hpp>

#include<boost/version.hpp>

YAD3_CLASS_IMPLEMENTATION(Cell_CLASS_DESCRIPTOR)

WOO_PLUGIN(core,(Cell)(Dispatcher)(EnergyTracker)(Engine)(Functor)(GlobalEngine)(ParallelEngine)(PartialEngine)(PeriodicEngine)(Plot)(PyRunner)(DisplayParameters)(WooTestClass)(WooTestPeriodicEngine));
#include<woo/core/Field.hpp>
WOO_PLUGIN(core,(Constraint)(Node)(NodeData)(Field));
#ifdef WOO_OPENGL
WOO_PLUGIN(gl,(NodeGlRep)(ScalarRange));
//NodeGlRep::~NodeGlRep(){}
#endif

EnergyTracker::~EnergyTracker(){} // vtable

//BOOST_CLASS_EXPORT(OpenMPArrayAccumulator<Real>);
//BOOST_CLASS_EXPORT(OpenMPAccumulator<Real>);
