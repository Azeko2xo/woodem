#include<yade/lib/object/ClassFactory.hpp>
// make core classes known to the class factory
#if 0
#include<yade/core/Body.hpp>
#include<yade/core/BodyContainer.hpp>
#include<yade/core/Bound.hpp>
#include<yade/core/FileGenerator.hpp>
#include<yade/core/Material.hpp>
#include<yade/core/Shape.hpp>
#include<yade/core/State.hpp>
#endif

#include<yade/core/Cell.hpp>
#include<yade/core/Dispatcher.hpp>
#include<yade/core/EnergyTracker.hpp>
#include<yade/core/Engine.hpp>
#include<yade/core/Functor.hpp>
#include<yade/core/DisplayParameters.hpp>
//#include<yade/core/Interaction.hpp>
//#include<yade/core/InteractionContainer.hpp>

#include<boost/version.hpp>

YAD3_CLASS_IMPLEMENTATION(Cell_CLASS_DESCRIPTOR)

YADE_PLUGIN(core,(Cell)(Dispatcher)(EnergyTracker)(Engine)(Functor)(GlobalEngine)(ParallelEngine)(PartialEngine)(PeriodicEngine)(PyRunner)(DisplayParameters));
#include<yade/core/Field.hpp>
YADE_PLUGIN(core,(Constraint)(Node)(NodeData)(Field));
#ifdef YADE_OPENGL
YADE_PLUGIN(gl,(NodeGlRep)(ScalarRange));
//NodeGlRep::~NodeGlRep(){}
#endif

EnergyTracker::~EnergyTracker(){} // vtable

//BOOST_CLASS_EXPORT(OpenMPArrayAccumulator<Real>);
//BOOST_CLASS_EXPORT(OpenMPAccumulator<Real>);
