#################
Inlets & outlets
#################

:obj:`Inlets <woo.dem.Inlet>` are engines which generate new particles during the simulation; their counterpart are outlets (:obj:`~woo.dem.BoxOutlet`) deleting particles during the simulation.

Inlets are split into several orthogonal components:

1. :obj:`~woo.dem.ParticleGenerator` creating a new particle, given some input criteria, such as the PSD;
2. :obj:`~woo.dem.Inlet`, which positions the new particle, given some spatial requirements (random, given sequence and such);
3. :obj:`~woo.dem.ParticleShooter` is used with random inlets and assigns an intial velocity to the particle; this component is optional, and actually only rarely used.

Generators
===========

PSD
----
`Particle size distribution (PSD) <http://en.wikipedia.org/wiki/Particle-size_distribution>`__ is an important characteristics of granular materials -- `cumulative distribution function (CDF) <http://en.wikipedia.org/wiki/Cumulative_distribution_function>`__ statistically describing distribution of grain sizes. In Woo, PSD is given as points :obj:`~woo.dem.PsdSphereGenerator.psdPts` of a piecewise-linear function. The :math:`x`-values should be increasing and :math:`y`-values should be non-decreasing (increasing or equal). The :math:`y`-value used for the last point is always taken as 100%, and all other :math:`y`-values are re-scaled accordingly.

For example, let's have the following PSD:

.. tikz:: \begin{axis}[area style, enlarge x limits=false, xlabel=d,ylabel={$P(D\leq d)$},grid=major]\addplot[mark=o,very thick,red] coordinates { (.2, 0) (.3, .2) (.4, .9) (.5, 1.) }; \end{axis}

Let's play with the generator now:

.. ipython::

   Woo [1]: gen=PsdSphereGenerator(psdPts=[(.2,0),(.3,.2),(.4,.9),(.5,1.)])

   # generators can be called using ``(material)``
   # let's do that in the loop, discaring the result (new particles)
   # the generator remembers the diameter internally
   Woo [1]: m=FrictMat()  # create any material, does not matter now

   # run the generator 10000 times
   Woo [1]: for i in range(0,10000): gen(m)

   @suppress
   Woo [1]: import pylab

   # plot normalized PSD
   @savefig tutorial-psd-generator-normalized.png width=6in
   Woo [1]: pylab.figure(3); \
      ...:pylab.plot(*gen.inputPsd(),label= 'in'); \
      ...:pylab.plot(*gen.     psd(),label='out'); \
      ...:pylab.legend(loc='best');

   # let's define this function for the following
   Woo [1]: def genPsdPlot(g,**kw):
      ...:   pylab.figure(max(pylab.get_fignums())+1)  # new figure
      ...:   pylab.plot(*g.inputPsd(**kw),label= 'in')
      ...:   pylab.plot(*g.     psd(**kw),label='out')
      ...:   pylab.legend(loc='best')

   # plot PSD scaled by mass which was generated
   @savefig tutorial-psd-generator-scaled.png width=6in
   Woo [1]: genPsdPlot(gen,normalize=False)

The size distribution can be shown with ``cumulative=False``:

.. ipython::

   @savefig tutorial-psd-generator-noncumulative.svg width=6in
   Woo [1]: genPsdPlot(gen,cumulative=False)

Discrete PSD
^^^^^^^^^^^^^

The PSD is by default piecewise-linear; piecewise-constant PSD is supported by setting the :obj:`~woo.dem.PsdSphereGenerator.discrete`; discrete values of diameters will be generated. For example:

.. ipython::
   
   # create the discrete generator
   Woo [1]: genDis=PsdSphereGenerator(psdPts=[(.3,.2),(.4,.9),(.5,1.)],discrete=True)

   # generate 10000 particles randomly
   Woo [1]: for i in range(0,10000): genDis(m)

   # plot the PSD
   @savefig tutorial-psd-generator-discrete.png width=6in
   Woo [1]: genPsdPlot(genDis)

   @savefig tutorial-psd-generator-discrete-noncumulative.png width=6in
   Woo [1]: genPsdPlot(genDis,cumulative=False)

Count-based PSD
^^^^^^^^^^^^^^^

In some domains, the :math:`y`-axis has the meaning of cumulative count of particles rather than their mass; this is supported when :obj:`~woo.dem.PsdSphereGenerator.mass` is ``False``; the input PSD is always plotted using the value of :obj:`~woo.dem.PsdSphereGenerator.mass` (mass-based or count-based). The output PSD can be plotted either way using the ``mass`` keyword; note that the count-based PSD is different (even if normalized) as small particles have smaller mass (proportionally to the cube of the size) than bigger ones:

.. ipython::

   # create the discrete generator
   Woo [1]: genNum=PsdSphereGenerator(psdPts=[(.3,0),(.3,.2),(.4,.9),(.5,1.)],mass=False)

   Woo [1]: for i in range(0,10000): genNum(m)

   @savefig tutorial-psd-generator-countbased.png width=6in
   Woo [1]: pylab.figure(max(pylab.get_fignums())+1); \
      ...:pylab.plot(*genNum.inputPsd(),label= 'in (count)'); \
      ...:pylab.plot(*genNum.psd(),label='out (mass)'); \
      ...:pylab.plot(*genNum.psd(mass=False),label='out (count)'); \
      ...:pylab.legend(loc='best');

Non-spherical particles
^^^^^^^^^^^^^^^^^^^^^^^^
Non-spherical particles can also be generated using PSD, by using their `equivalent spherical diameter <http://en.wikipedia.org/wiki/Equivalent_spherical_diameter>`__ (:obj:`woo.dem.Shape.equivRadius` gives equivalent spherical radius), i.e. diameter of sphere with the same volume. Examples of those generators are :obj:`~woo.dem.PsdEllipsoidGenerator`, :obj:`~woo.dem.PsdCapsuleGenerator` and :obj:`~woo.dem.PsdClumpGenerator`. The clump generator can use different probabilties for different clump shapes and scale them to satisfy required PSD -- see :obj:`its documentation <woo.dem.PsdClumpGenerator>` for details.

Non-PSD
--------

There are some generators which are not based on PSD; one of them is :obj:`woo.dem.PharmaCapsuleGenerator` which generates instances of pharmaceutical capsules of identical geometry; this generator was used in the bottle simulation shown in the previous section:

.. youtube:: jXL8qXi780M


Inlets
==========

Newly created particles must be placed in the simulation space, without colliding with existing particles; this is the task of :obj:`~woo.dem.Inlet`. The mass rate of particles generated is can be always obtained by querying :obj:`~woo.dem.Inlet.currRate`.

Random
-------
Random inlets position particles in a given part of space (box with :obj:`~woo.dem.BoxInlet`, 2d box with :obj:`~woo.dem.BoxInlet2d`, cylinder with :obj:`~woo.dem.CylinderInlet`); various parameters can govern the generation process:

1. :obj:`~woo.dem.Inlet.massRate` which limits how many mass per second may be generated; if set to zero, generate as many particles as "possible" in each step. "Possible" means that there will be in total :obj:`~woo.dem.Inlet.maxAttempts` attempts to place a particle in one step; one particle will be tried :obj:`~woo.dem.Inlet.attemptPar` times.

2. :obj:`~woo.dem.Inlet.maxMass` or :obj:`~woo.dem.Inlet.maxNum` limit total mass or the number of particles generated altogether. Once this limit is reached, the :obj:`~woo.dem.Inlet.doneHook` is run and the engine is made :obj:`~woo.core.Engine.dead`.

The bottle in the example above has :obj:`~woo.dem.CylinderInlet` above the neck:

.. literalinclude:: bottle.py

Dense
------
There is a special inlet :obj:`~woo.dem.ConveyorInlet` which can continuously produce stream of particles from pre-generated (compact, dense) arrangement which is periodic in one direction; as the name suggests, this is especially useful for simulating conveyors. The compaction process is itself dynamic simulation, which generates particles randomly in space at first, and lets them settle down in gravity; all this happens behind the scenes in the :obj:`woo.pack.makeBandFeedPack` function.

.. note:: Since the initial compaction is a relatively expensive (time-demanding) process, use the ``memoizeDir`` argument so that when the function is called with the same argument, it returns the result immediately.

.. literalinclude:: inlet-conveyor.py

.. youtube:: P862JVvWmpo 


This is another movie (created in Paraview) showing the :obj:`~woo.dem.ConveyorInlet` with spherical particles:

.. youtube:: -Q81oGxoz_s


Shooters
========
When random inlets generate particles, they have zero intial velocity; this can be changed by assigning a :obj:`~woo.dem.ParticleShooter` to :obj:`~woo.dem.RandomInlet.shooter`, as in this example:

.. literalinclude:: inlet-shooter.py

.. youtube:: zCPUll9MrLM


Outlets
=========

The inverse task is to delete particles from simulation while it is running; this is done using the :obj:`woo.dem.BoxOutlet` engine. This engine is able to report PSD just like inlets. Comparing PSDs from inlet and from the outlet can be useful for segregation analysis.

.. literalinclude:: inlet-outlet.py

.. youtube:: JHVD9au7LTY

.. figure:: fig/inlet-outlet_psds.*
   :align: center

   Comparison of PSDs of the inlet and the outlet.







