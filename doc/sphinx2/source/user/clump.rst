.. _user-clumps:

*******
Clumps
*******

Clumps are rigid aggregates of particles particles. They move in such way that particle's arrangement is preserved. Clumps are not particles by themselves, they don't collide with particles: only the particles they are composed of do. Forces applied on particles are subsequently transferred to the clump, which moves as a single body, and particles' positions and velocities are then update so that the configuration remains the same.

Internally, clump is a :obj:`~woo.core.Node` with some special data attached (:obj:`~woo.dem.ClumpData`). :obj:`~woo.dem.ClumpData` derives from :obj:`~woo.dem.DemData` (which contains things like :obj:`~woo.dem.DemData.mass`, :obj:`~woo.dem.DemData.vel`, … needed for motion integration) and adds information about particles it is composed of (also their relative positions and orientations).

Dynamic properties
==================

Dynamic properties such as :obj:`~woo.dem.DemData.mass` and :obj:`~woo.dem.DemData.inertia` are computed automatically, either analytically or numerically (grid sampling).


.. _clumps-analytic:

Analytic
---------

This algorithm is useful for non-intersecting (or lightly intersecting) configurations since the volumes of overlap are computed twice. 

Dynamic properties are computed by looping over all nodes (which carry the mass of particles --- this means that clumped particles can be multi-nodal) of the clump. Using :math:`\vec{x}_i`, :math:`\quat{q}_i`, :math:`m_i`, :math:`\vec{I}_i` to respectively denote :obj:`position <woo.core.Node.pos>`, :obj:`orientation <woo.core.Node.ori>`, :obj:`mass <woo.dem.DemData.mass>` and :obj:`local principal inertia <woo.dem.DemData.inertia>`, we compute in global coordinates mass, static momentum and inertia tensor as:

.. math::
   :nowrap:

   \begin{align*}
      m_g&=\sum_i m_i, \\
      \vec{S}_g&=\sum_i m_i \vec{x}_i, \\
      [\tens{I}_g]&=\sum_i f_{it}(f_{ir}(\vec{I}_i,\quat{q}_i^*),m_i,-\vec{x}_i),
   \end{align*}

where :math:`f_{it}` is function translating tensor of inertia using the `parallel axis theorem <https://en.wikipedia.org/wiki/Parallel_axis_theorem#Moment_of_inertia_matrix>`__ (:math:`[\tens{I}_{gi}]=[\tens{I}_{li}]+m_i(\vec{x}_i^T\vec{x}_i \mat{I}_{3\times3}-\vec{x}_i\vec{x}_i^T)`, with :math:`\tens{I}_{li}` being local inertia tensor but in global orientation) and :math:`f_{ir}` is function rotation the tensor of inertia using rotation matrix :math:`\mat{T}` (which is computed from the :math:`\quat{q}^*` conjugated orientation, since we rotate backwards, from local to global) as :math:`\mat{T}^T[\tens{I}]\mat{T}`).

.. todo:: Formulas for computing principal axes position & orientation.

Grid sampling
-------------
This method is used when computing properties of :obj:`woo.dem.SphereClumpGeom`.

.. todo:: Describe the grid sampling algorithm.

Building clumps
===============

Individual
----------

Using :obj:`S.dem.addClumped <woo.dem.ParticleContainer.addClumped>` instead of :obj:`S.dem.add <woo.dem.ParticleContainer.add>`, list of particles is clumped together before being added to the simulation. Dynamic clump properties are :ref:`computed analytically <clumps-analytic>` in this case. This can be useful for importing mesh which is not static, but should behave as a rigid objects, as seen e.g. in the :ref:`bottle example <tutorial-interpolated>` with the bottle:

.. youtube:: jXL8qXi780M

.. literalinclude:: ../tutorial/bottle.py

Inlet
------

.. todo:: :obj:`woo.dem.PsdClumpGenerator`

Clump motion
============

Gravity
-------

Particles participating in a clump are exempt from direct gravity application, since gravity is applied on the clump node itself already.

Contact forces
--------------

Clumps move just like any other nodes; contrary to particles, they have no direct (contact) forces applied on them so those must be collected in the :obj:`integrator <woo.dem.Leapfrog>` (by calling :obj:`woo.dem.ClumpData.forceTorqueFromMembers`); it is a simple summation of forces but related to the clump node's position :math:`\vec{x}_c`:

.. math::
   :nowrap:

   \begin{align*}
      F_{\Sigma}&=\sum \vec{F}_i, \\
      T_{\Sigma}&=\sum \vec{T}_i+(\vec{x}_i-\vec{x}_c)\times\vec{F}_i.
   \end{align*}

These summary forces are used in :ref:`theory-motion-integration`.

.. note:: Forces on clumps are collected in the integrator for the purposes of motion integration and usually (when :obj:`woo.dem.Leapfrog.reset` is ``True``, which is the highly recommended default) immediately discarded again; that means that the summary force is not easily accessible for the user. The function :obj:`woo.dem.ClumpData.forceTorqueFromMembers` can be used to compute force and torque from members on-demand. That is useful e.g. for plotting acting force; note however that the value returned from :obj:`~woo.dem.ClumpData.forceTorqueFromMembers` does *not* include gravity acting on the clump as whole, as per notice above.

   .. ipython::
   
      Woo [1]: from woo.core import *; from woo.dem import *

      Woo [1]: S=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))],engines=DemField.minimalEngines(),dt=1e-5)

      Woo [1]: S.dem.par.add(Wall.make(0,axis=2,sense=1))              # ground

      Woo [1]: n=S.dem.par.addClumped([Sphere.make((0,0,0),.5),Sphere.make((0,0,1),.5),Sphere.make((0,1,0),.5)])  # clump of 3 spheres

      Woo [1]: n.pos                                                   # clump node is in the centroid

      Woo [1]: S.dem.par[0].mass, S.dem.par[1].mass, S.dem.par[2].mass # mass of individual particles
      
      Woo [1]: n.dem.mass                                              # clump mass is the sum

      Woo [1]: S.run(1000,True)                                        # run a few steps so that the clump reaches the ground

      Woo [1]: S.dem.par[0].f, S.dem.par[1].f, S.dem.par[2].f          # contact force on individual particles

      Woo [1]: ClumpData.forceTorqueFromMembers(n)                     # force and torque on the whole clump

