Timestep computation
=====================

:obj:`Timestep <woo.core.Scene.dt>` determines numerical stability and is an important parameter in simulations. It can be assigned explicitly, but it can also be left at its default ``nan`` value, and it will be initialized automatically.

In the DEM field, there are three constraints for critical timestep:

#. particle stiffnesses (:obj:`~woo.utils.pWaveDt` and similar) which essentially estimate future contact stiffnesses from a single particle;
#. current contact stiffnesses (:obj:`~woo.dem.DynDt`) sums up all stiffnesses of all contact of a node, and estimates eigenvalue, which is the proper stability criterion;
#. current intra-particles stiffnesses (FEM) which are linking nodes of multi-nodal particles (:obj:`~woo.dem.DynDt` provides the interface to query those, by using :obj:`~woo.dem.IntraForce`).

Initialization
--------------
When timestep is not a meaningful value at the beginning of the timestep (usually the very first one), all defined fields (via ``Field::critDt``) and engines (via ``Engine::critDt``) are queried for critical timestep:

#. :obj:`~woo.dem.DemField` determines critical timestep by calling :obj:`~woo.utils.pWaveDt` on its particles.
#. :obj:`~woo.dem.ConveyorInlet` reports minimum timestep based on the smallest particle to be generated, and assigned material.
#. :obj:`~woo.dem.RandomInlet` calls its :obj:`generators <woo.dem.ParticleGenerator>`, which, in conjunction with material information from the inlet, compute the critical timestep for particles to be generated.
#. :obj:`~woo.dem.DynDt` is called to determine timestep the same as otherwise (see below).

The critical timestep from all these sources is multipled by the :obj:`~woo.core.Scene.dtSafety` factor before being applied to :obj:`~woo.core.Scene.dt`.

:obj:`~woo.dem.DynDt`
--------------------

:obj:`~woo.dem.DynDt` uses both contact and intra-particle stiffnesses to determin critical timestep:

   #. if :obj:`engines <woo.core.Scene.engines>` contain :obj:`~woo.dem.IntraForce`, it will be used: for every multi-nodal particle, ``IntraForce::addIntraStiffness`` is called; this dispatches the call to shape-specific functor (if any), which in turn calls its own shape-specific ``IntraFunctor::addIntraStiffnesses``. This function is responsible for intializing internal stiffness matrices, if necessary, and reporting diagonal stiffness terms to the caller.

   #. for all nodes, traverse all particles attached to it, and add their normal and tangential stiffnesses.

