.. _dem_contact_models:

*******************
DEM contact models
*******************

In DEM, particles are represented as rigid :obj:`geometry <woo.dem.Shape>` and any deformation of particles due to collisions with other ones are accounted for by contact models. Modeling contact consists (roughly speaking) of 3 tasks:

#. Decide whether particles geometrically interact, and if so, then:

   #. compute/update :obj:`local contact coordinates <woo.dem.CGeom.node>`, defined by contact point, contact normal (local :math:`x`-axis) and orientation of the perpendicular plane (local :math:`y`- and :math:`z`-axes);
   #. compute relative velocities of particles in the local system (:obj:`woo.dem.L6Geom.vel` and :obj:`woo.dem.L6Geom.angVel`)

#. Combine :obj:`material parameters <woo.dem.Material>` and compute :obj:`non-geometrical properties <woo.dem.CPhys>` of the contact, such as stiffness or friction parameters. This is usually done only once, when the contact is created (:obj:`woo.dem.ContactLoop.updatePhys`).

#. Compute effects of the contact on particles -- :obj:`force <woo.dem.CPhys.force>`, :obj:`torque <woo.dem.CPhys.torque>`) etc.

The first task is covered in the :ref:`contact_geometry` chapter while the last is covered in the following ones. The second task, computing contact properties from material parameters, is an implementation detail at this point not dealt with here.

..  This is the responsibility of :obj:`contact law functors <woo.dem.LawFunctor>`.
    This task is performed by :obj:`CPhys functors <woo.dem.CPhysFunctor`, 

.. Each task is performed by a "functor" with 2 inputs and some output. The last task uses outputs from the first two to compute forces & torques.
      =========================== ======================= ===================== ====================
      functor                     first input             second input          creates/updates
      --------------------------- ----------------------- --------------------- --------------------
      :obj:`woo.dem.CGeomFunctor` :obj:`woo.dem.Shape`    :obj:`woo.dem.Shape`  :obj:`woo.dem.CGeom`
      :obj:`woo.dem.CPhysFunctor` :obj:`woo.dem.Material` :obj:`woo.dem.Shape`  :obj:`woo.dem.CPhys`
      :obj:`woo.dem.LawFunctor`   :obj:`woo.dem.CGeom`    :obj:`woo.dem.CPhys`  force, torque, …
      =========================== ======================= ===================== ====================

.. toctree::
	:maxdepth: 3
	
	linear.rst
	hertzian.rst
	adhesive.rst

