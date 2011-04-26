#include<yade/lib/factory/ClassFactory.hpp>
// make core classes known to the class factory
#include<yade/core/Body.hpp>
#include<yade/core/BodyContainer.hpp>
#include<yade/core/Bound.hpp>
#include<yade/core/Cell.hpp>
#include<yade/core/Dispatcher.hpp>
#include<yade/core/EnergyTracker.hpp>
#include<yade/core/Engine.hpp>
#include<yade/core/FileGenerator.hpp>
#include<yade/core/Functor.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/core/InteractionContainer.hpp>
#include<yade/core/Material.hpp>
#include<yade/core/Shape.hpp>
#include<yade/core/State.hpp>

#include<boost/version.hpp>

// these two are not accessible from python directly (though they should be in the future, perhaps)

#if BOOST_VERSION>=104200
	BOOST_CLASS_EXPORT_IMPLEMENT(BodyContainer);
	BOOST_CLASS_EXPORT_IMPLEMENT(InteractionContainer);
#else
	BOOST_CLASS_EXPORT(BodyContainer);
	BOOST_CLASS_EXPORT(InteractionContainer);
#endif

YADE_PLUGIN(core,(Body)(Bound)(Cell)(Dispatcher)(EnergyTracker)(Engine)(FileGenerator)(Functor)(GlobalEngine)(Interaction)(IGeom)(IPhys)(Material)(PartialEngine)(Shape)(State));


#include<yade/core/Field.hpp>
YADE_PLUGIN(core,(Constraint)(Node)(NodeData)(Field));

EnergyTracker::~EnergyTracker(){} // vtable

//BOOST_CLASS_EXPORT(OpenMPArrayAccumulator<Real>);
//BOOST_CLASS_EXPORT(OpenMPAccumulator<Real>);
