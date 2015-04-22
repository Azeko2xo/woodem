.. _user-shapepack:

**********
ShapePack
**********

:obj:`~woo.dem.ShapePack` is an object which stores purely geometrical configuration of a number of particles; contrary to :ret:`tutorial-spherepack` (which is less universal, but has many sphere-specific optimizaitons), it can contain also other shapes than just spheres.

The API of both :obj:`~woo.dem.ShapePack` and :obj:`~_woo._packSphere.SpherePack` is deliberately almost identical.

File format
============

:obj:`~woo.dem.ShapePack` is useful as intermediary storage of particle arrangement (e.g. beteween different simulations, or as cache for routines such as :obj:`~woo.pack.makeBandFeedPack`).


.. ipython::

   Woo [1]: sp=woo.dem.ShapePack()     # create new object

   Woo [1]: sp.fromDem(S, S.dem)       # get data from the current simulation

   Woo [1]: sp.saveTxt('export-shapepack.txt') # save data to file

   Woo [1]: sp2=woo.dem.ShapePack(loadFrom='export-shapepack.txt')  # load at construction time

   Woo [1]: sp3=woo.dem.ShapePack()

   Woo [1]: sp3.loadTxt('export-shapepack.txt')                     # load with an existing object


The resulting file contains one particle per line::

   0 Capsule 0.0120227 0.976311 0.163282 0.105552 -0.0138765 -0.0322094 -0.0335685
   1 Capsule 0.849475 0.849563 0.0607862 0.0953496 0.0315324 0.0102243 0.000629567
   2 Capsule -0.372623 -0.103693 0.0763046 0.141139 0.0639594 0.00390455 0.000197987
   3 Capsule 0.884581 0.32526 0.0991963 0.17232 0.0543704 0.0431827 -0.00213683
   4 Capsule 0.610379 0.250056 0.257899 0.117386 0.0160657 -0.0521275 0.012216
   5 Capsule -1.07945 -0.0630431 0.0648866 0.122897 -0.0513651 0.0231532 -0.000282427
   6 Capsule 0.629565 1.33939 0.202427 0.099201 0.0177916 0.0208184 -0.0254291
   7 Capsule -0.115262 -1.0465 0.089268 0.170938 0.0703223 0.0447295 0.00343105
   8 Capsule -0.331438 0.762446 0.121542 0.114695 0.0252811 -0.0259674 -0.020808
   9 Capsule 0.383944 0.935563 0.397593 0.154123 0.0513785 0.0153721 -0.00217204
   10 Sphere -0.192145 0.137062 0.592456 0.0682023
   10 Sphere -0.186653 0.215515 0.615108 0.109124
   10 Sphere -0.181162 0.293969 0.63776 0.0682023
   11 Sphere 0.23314 -0.535194 0.543624 0.0788098
   11 Sphere 0.146563 -0.499811 0.529616 0.126096
   11 Sphere 0.0599855 -0.464427 0.515607 0.0788098
   12 Sphere -0.355005 0.980683 0.353488 0.110661
   12 Sphere -0.345734 0.89573 0.392399 0.110661
   13 Sphere -0.751995 0.926802 0.246687 0.101449
   13 Sphere -0.828549 0.924203 0.207408 0.101449
   14 Sphere -0.700782 -0.661376 0.236885 0.0769197
   15 Sphere -0.671113 -0.742401 0.269667 0.123072
   16 Sphere -0.641443 -0.823426 0.30245 0.0769197
   17 Sphere 0.0668563 0.138768 0.724237 0.120268
   18 Sphere 0.167893 0.12529 0.729157 0.120268

The format has the following items:

#. first colum is particle ID (within this :obj:`~woo.dem.ShapePack` object); if several consecutive particles have the same ID, they are clumped together;
#. second column is :obj:`~woo.dem.Particle.shape` type;
#. columns 3--5 are bounding sphere center coordinates (x, y, z);
#. column 6 is bounding sphere radius;
#. remaining columns are shape-specific.

Shape-specific columns
----------------------

.. note:: :obj:`~woo.dem.ShapePack`'s text format is an implementation detail. Although efforts will be made to not change existing formats, there is no strong guarantee that it will never change. With this in mind, read on.

Details on shape-specific columns can be always found by looking at the implementation of respective :obj:`woo.dem.Shape`'s ``asRaw`` and ``setFromRaw`` methods.

* :obj:`~woo.dem.Sphere` has no shape-specific columns, it is identical with the bounding sphere.
* :obj:`~woo.dem.Capsule` has 3 additional columns representing the half-shaft vector (half of :obj:`~woo.dem.Capsule.shaft` pointing in the direction of the local :math:`z`-axis) in global 3d space.
* :obj:`~woo.dem.Ellipsoid` has 6 additional columns storing two 3-vectors:
  #. rotation vector (the :obj:`ori <woo.core.Node.ori>` quaternion converted to angle :math:`\alpha` and normalized axis :math:`\vec{a}`, then compute :math:`\alpha \vec{a}`)
  #. :obj:`~woo.dem.Ellipsoid.semiAxes`

Multi-nodal shapes can also be stored in the :obj:`~woo.dem.ShapePack`, though this has been an experimental feature only so far. Their extra columns have the following meaning:

* :obj:`~woo.dem.Facet` (and also geometrically identical :obj:`~woo.fem.Membrane`) store 3 nodal coordinates (9 extra columns) in global space.
* :obj:`~woo.fem.Tetra` (and also geometrically identical :obj:`~woo.fem.Tet4`) store 4 nodal coordinates (12 extra columns) in global space.

Multinodal shapes feature a special (little tested) way to share nodes, which is often the case with triangulated solids: if the x-coordinate of a node is ``NaN`` (not-a-number), then the :math:`z`-coordinate encodes the index of an already-existing node to re-use (nodes are counted within groups with identical IDs).

