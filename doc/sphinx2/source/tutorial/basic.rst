###################
Simulation basics
###################

Scene
=======

The whole simulation is contained in a single :obj:`woo.core.Scene` object, which contains the rest; a scene can be can be loaded from and saved to a file, just like any other object. Scene defines things like timestep (:obj:`~woo.core.Scene.dt`) or periodic boundary conditions (:obj:`~woo.core.Scene.cell`).

It also contains several :obj:`fields <woo.core.Field>`; each field is dedicated to a one simulation method, and the fields can be semi-independent. As we go for a DEM simulation, the important field for us is a :obj:`~woo.dem.DemField`; that is the one containing particles and contacts (we deal with that later); we can also define gravity acceleration for this field by setting :obj:`DemField.gravity <woo.dem.DemField.gravity>`.

There can be an unlimited number of scenes, each of them independent of others. Only one can be controlled by the UI (and shown in the 3d view); it is the one assigned to :obj:`woo.master.scene <woo.core.Master>`, the master scene object.

New scene with the DEM field can be initialized in this way::

   import woo, woo.core, woo.dem   # import modules we use
   S=woo.core.Scene()              # create the scene object
   f=woo.dem.DemField()            # create the field object
   f.gravity=(0,0,-9.81)           # set gravity acceleration
   S.fields=[f]                     # add field to the scene
   woo.master.scene=S              # this will be the master scene now

Since all objects can take *keyword arguments* when constructed, and assignments can be chained in Python, this can be written in a much more compact way::

   S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField(gravity=(0,0,-9.81))])

The DEM field can be accessed as ``S.fields[0]``, but this is not very convenient. There is a shorthand defined, :obj:`S.dem <woo.core.Scene.dem>`, which always returns the (first) DEM field attached to the scene object.

Particles
==========

Particles are held in the :obj:`DemField.par <woo.dem.DemField.particles>` container (shorthand for :obj:`DemField.particles <woo.dem.DemField.particles`); new particles can be added using the :obj:`append <woo.dem.DemField.append>` method.

Particles are not simple objects; they hold together :obj:`material <woo.dem.Material>` and :obj:`shape <woo.dem.Shape>` (the geometry -- such as :obj:`~woo.dem.Sphere`, :obj:`~woo.dem.Capsule`, :obj:`~woo.dem.Wall`, ...) and other thigs. . :obj:`~woo.dem.Shape` is attached to one (for mononodal particles, like :obj:`spheres <woo.dem.Sphere>`) or several (for multinodal particles, like :obj:`facets <woo.dem.Facet>`) nodes with some :obj:`position <woo.core.Node.pos>` and :obj:`orientation <woo.core.Node.ori>`, each node holds :obj:`~woo.dem.DemData` containing :obj:`~woo.dem.DemData.mass`, :obj:`~woo.dem.DemData.inertia`, :obj:`velocity <woo.dem.DemData.vel>` and so on.

To avoid complexity, there are utility functions that take a few input data and return a finished :obj:`~woo.dem.Particle`.

.. todo:: Write about :obj:`woo.utils.sphere`, :obj:`woo.utils.capsule` and so on, or move to ``woo.dem.Sphere.make``, ``woo.dem.Capsule.make`` as static methods...?!

We can define an infinite plane (:obj:`~woo.dem.Wall`) in the :math:`xy`-plane with :math:`z=0` and add it to the scene::

   wall=woo.utils.wall(0,axis=2)
   S.dem.par.append(wall)

Walls are *fixed* by default, and the :obj:`woo.utils.defaultMaterial` was used as :obj:`~woo.dem.Particle.material` -- the default material is not good for real simulations, but it is handy for quick demos. We also define a :obj:`sphere <woo.dem.Sphere>` and put it in the space above the wall::

   sphere=woo.utils.sphere((0,0,2),radius=.2)
   S.dem.par.append(sphere)

The :obj:`~woo.dem.ParticleContainer.append` method can also take several particles as list, so we can add both particles in a compact way::

   S.dem.par.append([
      woo.utils.wall(0,axis=2),
      woo.utils.sphere((0,0,2),radius=.2)
   ])

or in one line::

   S.dem.par.append([woo.utils.wall(0,axis=2),woo.utils.sphere((0,0,2),radius=.2)])

Engines
========

After adding particles to the scene, we need to tell Woo what to do with those particles. :obj:`Scene.engines <woo.core.Scene.engines>` describe things to do. Engines are run one after another; once the sequence finishes, :obj:`~woo.core.Scene.time` is incremented by :obj:`~woo.core.Scene.dt`, :obj:`~woo.core.Scene.step` is incremented by one, and the sequence starts over.

Although the engine sequence can get complex, there is a pre-cooked engine sequence :obj:`woo.utils.defaultEngines()` suitable for most scenarios; it consists of the minimum of what virtually every DEM simulation needs:

* motion integration (compute accelerations from contact forces and gravity on nodes, update velocities and positions);
* collision detection (find particle overlaps)
* contact resolution (compute forces resulting from overlaps; without any other parameters, the linear contact model is used);

Assigning the engines is simple::

   S.engines=woo.utils.defaultEngines()

-----------

All in all, the minimal simulation of a sphere falling onto plane looks like this:

   import woo, woo.core, woo.dem, woo.utils
   S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField(gravity=(0,0,-9.81))])
   S.dem.par.append([woo.utils.wall(0,axis=2),woo.utils.sphere((0,0,2),radius=.2)])
   S.engines=woo.utils.defaultEngines()
   S.saveTmp()
