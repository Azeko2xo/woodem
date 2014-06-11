##########
Factories
##########

:obj:`Factories <woo.dem.Factory>` are engines which generate new particles during the simulation; their counterpart are deleters (:obj:`~woo.dem.BoxDeleter`) deleting particles during the simulation.

Factories are split into several orthogonal components:

1. :obj:`~woo.dem.ParticleGenerator` creating a new particle, given some input criteria, such as the PSD;
2. :obj:`~woo.dem.Factory`, which positions the new particle, given some spatial requirements (random, given sequence and such);
3. :obj:`~woo.dem.ParticleShooter` assigning intial velocity to the particle; this component is optional, and actually only rarely used.

Generators
===========

PSD-based
----------
`Particle size distribution (PSD) <http://en.wikipedia.org/wiki/Particle-size_distribution>`__ is an important characteristics of granular materials -- `cumulative distribution function (CDF) <http://en.wikipedia.org/wiki/Cumulative_distribution_function>`__ statistically describing distribution of grain sizes. In Woo, PSD is given as points :obj:`~woo.dem.PsdSphereGenerator.psdPts` of a piecewise-linear function. The :math:`x`-values should be increasing and :math:`y`-values should be non-decreasing (increasing or equal). The :math:`y`-value used for the last point is always taken as 100%, and all other :math:`y`-values are re-scaled accordingly.

For example, let's have the following PSD:

.. tikz:: \begin{axis}[area style, enlarge x limits=false, xlabel=d,ylabel={$P(D\leq d)$},grid=major]\addplot[mark=o,very thick,red] coordinates { (.3, 0) (.3, .2) (.4, .9) (.5, 1.) }; \end{axis}

Let's play with the generator now:

.. ipython::

   Woo [1]: gen=PsdSphereGenerator(psdPts=[(.3,.2),(.4,.9),(.5,1.)])

   # generators can be called using ``(material)``
   # let's do that in the loop, discaring the result (new particles)
   Woo [1]: m=FrictMat()  # create any material, does not matter now

   Woo [1]: for i in range(0,100000): gen(m)

   Woo [1]: import pylab

   # scale the PSD to the mass generator
   Woo [1]: pylab.plot(*gen.inputPsd(scale=True),label='input',linewidth=3,alpha=.4)
   
   # we could, alternatively, normalize the output PSD by gen.psd(normalize=True)
   Woo [1]: pylab.plot(*gen.psd(),label='output',linewidth=3,alpha=.4)

   Woo [1]: pylab.legend()
   
   @savefig tutorial-psd-sphere-generator.png width=8in
   Woo [1]: pylab.grid(True)

