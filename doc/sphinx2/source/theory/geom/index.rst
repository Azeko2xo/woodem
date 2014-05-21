.. _contact_geometry:

=========
Geometry
=========

Woo splits each contact into its geometrical features and the :ref:`contact model <dem_contact_models>`.

The geometry is given by local coordinates (situated at the contact point and having the :math:`x`-axis along the contact normal) and is also responsible for computing relative velocity of contacting particles in local coordinates. THis information is stored in :obj:`woo.dem.L6Geom`.

The geometry computation (performed by various :obj:`woo.dem.CGeomFunctor` subclasses) is split in two parts:

* Algorithm for computing relative velocities (and other features) at the contact point, which is common for all particle shapes:

   .. toctree::
   
      l6geom.rst

* Intersection algorithms for various :obj:`shapes <woo.dem.Shape>`, such as :obj:`sphere <woo.dem.Sphere>` with :obj:`facet <woo.dem.Facet>`, :obj:`ellipsoid <woo.dem.Ellipsoid>` with :obj:`wall <woo.dem.Wall>` and so on.

   .. toctree::

      sphere.rst
      ellipsoid.rst


Particle shapes
----------------
The following particle shapes are supported:

1. Simple shapes (1-node): :obj:`~woo.dem.Sphere`, :obj:`~woo.dem.Capsule`, :obj:`~woo.dem.Ellipsoid`;
2. Multinodal shapes: :obj:`~woo.dem.Truss` (2 nodes), :obj:`~woo.dem.Facet` (3 nodes); :obj:`~woo.dem.FlexFacet` (same as :obj:`~woo.dem.Facet` geometrically, but adds deformability);
3. Infinite axis-aligned shapes: :obj:`~woo.dem.InfCylinder`, :obj:`~woo.dem.Wall`.

The following matrix shows which shapes can collide with each other:

.. include:: generated-cg2-table.rst

The ``g3g`` geometry is an algorithm used in Yade's `ScGeom <https://yade-dem.org/doc/yade.wrapper.html#yade.wrapper.ScGeom>`__ and many other codes including PFC3D; it was only included for comparison purposes and is not to be used otherwise.
