#######
Meshes
#######

.. admonition:: Overview

   This chapter explains how to create mesh by importing or by paramteric construction.

STL import
===========

`STL (STereoLitography) file format <http://en.wikipedia.org/wiki/STL_%28file_format%29>`__ is an interchange format for triangulated sorfaces in 3d. It is used as a possible export format in many CAD systems. There is a single function for their import, :obj:`woo.utils.importSTL <_utils2.importSTL>` which return list of facet particles, which can be added to the simulation instantly::

   S.dem.par.append(woo.utils.importSTL('your-model.stl'),mat=woo.utils.defaultMaterial())

.. figure:: fig/ship-mesh.png
   :align: center

   Mesh imported from the STL file.

The :obj:`woo.utils.importSTL <_utils2.importSTL>` function has options for scaling, shifting, rotating and assigning colors -- check its documentation for details.


Parametric surfaces
====================

`GNU Triangulated Surface library <http://gts.sourceforce.net>`__ is a library for constructing and manipulating triangulated surfaces. There are a few convenience functions in Woo for converting lists of points into a GTS surface, which can be again converted to list of facets easily, and added to the simulation.

For example, let's create V-shaped conveyor bed with flad bottom. Let's work in local coordinates first, so that :math:`x`-axis is the conveyor axis, :math:`y` is horizontal and :math:`z` is vertical. The dimensions of the bed will be:

* ``botWd``, flat bottom width
* ``sideWd``, elevated side width
* ``sideHt``, elevated side height

The bed will start at :math:`x=0` and will go up to :math:`x=x_1` (``x1``) with ``xDiv`` segments:

.. ipython::

   @suppress
   Woo [1]: S=Scene(fields=[DemField()])

   # set parameters
   Woo [1]: botWd=.3; sideWd=.2; sideHt=.1; x1=2; xDiv=5

   Woo [1]: import numpy

   # x points where polylines will be defined
   Woo [1]: xx=numpy.linspace(0,x1,num=xDiv)

   Woo [1]: print xx

   # for each x, create the polyline with 4 points
   Woo [1]: pts=[[(x,-.5*botWd-sideWd,sideHt),(x,-.5*botWd,0),(x,.5*botWd,0),(x,.5*botWd+sideWd,sideHt)] for x in xx]

   Woo [1]: print pts

   # convert list of polylines to a gts surface (all must have the same number of points)
   Woo [1]: surf=woo.pack.sweptPolylines2gtsSurface(pts)

   Woo [1]: print surf

   # convert surface to facets and add them to the scene
   Woo [1]: S.dem.par.append(woo.pack.gtsSurface2Facets(surf))

If one wants to convert from local to global coordinates, define local coordinates via :obj:`~woo.core.Node` and pass it as the ``localCoords`` parameter to :obj:`woo.pack.sweptPolylines2gtsSurface`:

.. ipython::

   @suppress
   Woo [1]: import math

   Woo [1]: node=Node(pos=(.2,.2,.2),ori=((0,0,1),math.pi/3.))

   Woo [1]: surf=woo.pack.sweptPolylines2gtsSurface(pts,localCoords=node)

or use the node object manually to convert all points:

.. ipython::

   Woo [1]: pts=[[node.loc2glob(p) for p in pp] for pp in pts]

   Woo [1]: surf=woo.pack.sweptPolylines2gtsSurface(pts)

Behind the scenes, :obj:`~woo.core.Node.loc2glob` does nothing else than transforming the local point :math:`p'` using the local origin position :math:`O` and orientation :math:`q` as

.. math:: p=q(p'+O)q^*

or, in Python, ``n.ori*(p+n.pos)``.

.. note:: Quaternions are constructed from the `Axis-angle <http://en.wikipedia.org/wiki/Axis_angle>`__ representation of rotations; angles are always given in radians. Thus e.g. ``Quaternion((0,0,1),math.pi/3.)`` rotates around the :math:`z`-axis by :math:`\frac{\pi}{3}\equiv60°`.

In summary, the conveyor code looks like this, in a compact way:

.. literalinclude:: mesh-bed.py

.. figure:: fig/conveyor-mesh.png
   :align: center

   Conveyor bed mesh created parametrically by the script above.

