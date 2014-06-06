######################
Impose force/velocity
######################

Motion integration is performed by the :obj:`~woo.dem.Leapfrog` engine and described in detail in :ref:`theory-motion-integration`. Usually, each node moves according to force, which results in acceleration, velocity change and position change; nodes rotate according to torque, which results in angular acceleration, angular velocity change and orientation change.

It is sometimes useful to prescribe some other kind of motion -- e.g. force, velocity or trajectory.

.. note:: Motion integration is performed on :obj:`nodes <woo.core.Node>`, which are usually attached to :obj:`particles <woo.dem.Particle>`. There are also nodes *not* attached to any particle but used by some engines (e.g. :obj:`~woo.dem.Conveyor`); those nodes, if added `S.dem.nodes` (usually via :obj:`S.dem.nodesAppend <woo.DemField.nodesAppend>`), will move just the same. This will be shown below.

Blocking DoFs
==============

Blocking `degrees of freedom (DoFs) <http://en.wikipedia.org/wiki/Degrees_of_freedom_%28mechanics%29>`__ is done via the ``fixed`` parameter to routines creating particles (:obj:`Sphere.make <woo.dem.Sphere.make>` and friends). This parameter simply sets :obj:`DemData.blocked <woo.dem.DemData.blocked>`. :obj:`~woo.dem.Wall`, :obj:`~woo.dem.InfCylinder` and :obj:`~woo.dem.Facet` are by default ``fixed`` (they will not move, even if force is applied to them); in contrast to that, particles like :obj:`~woo.dem.Sphere` or :obj:`~woo.dem.Capsule` are by default not fixed.

:obj:`DemData.blocked <woo.dem.DemData.blocked>` is in itself more versatile; it can block forces/torques along individual axes; translations are noted as ``xyz`` (lowercase) and rotations ``XYZ`` (uppercase). For instance

* to prevent particle from rotating, set :obj:`~woo.dem.DemData.blocked` to ``XYZ``;
* for 2d simulation in the :math:`xy`-plane, set :obj:`~woo.dem.DemData.blocked` to ``zXY`` (prevent out-of-plane translation and only allow rotation along the axis perpendicular to :math:`xy` plane).

.. note:: :obj:`~woo.dem.DemData.blocked` does not make the node necessarily **unmovable**. Since forces/torques along that axis are ignored, it only means that :obj:`velocity <woo.dem.DemData.vel>` and :obj:`angular velocity <woo.dem.DemData.angVel>` will be **constant**. Those are initially zero, but if you set them to a non-zero value, the node will be moving.

.. ipython::

   Woo [1]: s=Sphere.make((0,0,1),.1)

   # see what is blocked on the sphere
   Woo [1]: s.blocked

   Woo [1]: s.blocked='XYZ'

   # did it really change?
   Woo [1]: s.blocked

   Woo [1]: w=Wall.make(0,axis=2)

   # see what is blocked on the wall by default
   Woo [1]: w.blocked

   # Particle.blocked is only a shorthand for particles with a single node
   # it really means this:
   Woo [1]: w.nodes[0].dem.blocked

   # DemData is what carries all DEM-related properties (velocity, mass, inertia and such)
   Woo [1]: w.nodes[0].dem

   # let's dump it to see what's inside there
   Woo [1]: w.nodes[0].dem.dumps(format='expr')

:obj:`DemData.impose`
=====================

A more versatile option is to use the :obj:`DemData.impose <woo.dem.DemData.impose>` to assign any object deriving from :obj:`woo.dem.Impose`. What is imposed in each step is either force or velocity, though those are not constrained to global coordinate axes, and are computed by some c++ routine. :obj:`~woo.dem.Impose` instances can be shared by many nodes.

This shows how the feed was made movable by imposing :obj:`woo.dem.HarmonicOscillation` on the :obj:`woo.dem.ConveyorFactory.node`.

.. youtube:: M23o4tuak6g

:obj:`woo.dem.InterpolatedMotion` was used in this example to move the bottle:

.. youtube:: D_pc3RU5IXc

.. literalinclude:: bottle.py

This example shows the :obj:`woo.dem.AlignedHarmonicOscillations` imposition, which is applied to all cylinders with different parameters:

.. youtube:: 6BvN0OuhgvI 

.. literalinclude:: impose-harmonic.py


