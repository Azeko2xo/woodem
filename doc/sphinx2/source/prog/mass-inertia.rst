Mass and inertia computation
============================

:obj:`Mass <woo.dem.DemData.mass>` and :obj:`~woo.dem.DemData.inertia` are computed automatically from particle geometry. For the purposes of rotation integration, :obj:`~woo.dem.DemData.inertia` is diagonal tensor (and only the diagonal is actually stored) expressed in node-local coordinates (:obj:`~woo.core.Node.pos` and :obj:`woo.core.Node.ori`), therefore nodes might need to be rotated so that mass distribution is aligned with their local coordinate system.

Uninodal particles always have the node in their centroid, and they are oriented along the local axes (e.g. :obj:`~woo.dem.Capsule` is always elongated along the local :math:`+x`-axis).


Uninodal particles
------------------

Uninodal particles have a simplified interface for setting mass and inertia. ``Particle::updateMassInertia`` (callable from Python as :obj:`woo.dem.Particle.updateMassInertia`) calls in turn ``Shape::updateMassInertia`` passing particle's material :obj:`~woo.dem.Material.density` as parameter.

Multinodal particles
--------------------

The generic routine ``Shape::lumpMassInertia`` collects mass and inertia for given shape and node. 

This function is usually invoked throug the static :obj:`woo.dem.DemData.setOriMassInertia` function (which takes :obj:`woo.core.Node` as argument), which collets mass and inertia from all shapes referenced by the node, computes principal axes and aligns node orientation with principal axes; this raises and exception if the shape is orientation-dependent, such as capsule attached to a tetrahedron node.

Clumps
------

Clumps implement analytical and numerical (grid-based) computation of centroid, mass, inertia and local orientation.

The analytical method is suitable for clumps without overlaps, where first-momentum is used to compute centroid, second-order momentum and parallel axis theorem to compute inertia tensor eigenvalues and eigenvectors, which are principal axes.

The numerical method is similar, except that it samples the clump with regular grid, and computes each voxel (volume element, grid cell) contribution to mass, first- and second-order momenta. This is suitable for clumps where different shapes are overlapping, since each voxel is only counted once. The numerical routine has different (more efficient) implementation for sphere-only clumps (:obj:`woo.dem.SphereClumpGeom`; testing whether a point is inside is trivial) and another one for generic-shape clumps (:obj:`woo.dem.RawShapeClump`; the point is tested for being inside using the ``Shape::isInside`` virtual method).
