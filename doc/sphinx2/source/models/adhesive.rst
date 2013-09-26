.. _adhesive_contact_models:

****************
Adhesive models
****************
In addition to hertzian models, adhesive models capture the effect of distance-dependent attractive force between particles. Adhesive models are usually mathematically rather complicated, extending the Hertz theory in some way. Usually the equations for :math:`\delta(a)` and :math:`a(P)` are provided, reflecting the typical use of adhesive models in microscopy fields. For DEM models, the relationship :math:`P(\delta)` is needed. In some cases, the :math:`\delta(a(P))` is not analytically invertible and solution must be sought by iteration which can have some performance impacts.

Symbols
========

Symbols used for adhesive models, in addition to those introduced for Hertzian models) are:

==================  ===============
:math:`\gamma`      surface energy (in models with adhesion)
:math:`P_{na}`      normal adhesive (also called "pull-off" or "sticking") force
:math:`P_c`	        critical force (leads to surface separation); maximum tensile force an adhesive contact can sustain
:math:`\alpha`      :cite:`Carpick1999` transition parameter between DMT and JKR models
:math:`\hat\delta`  reduced penetration (dimensionless)
:math:`\hat P`      reduced total normal force (dimensionless)
:math:`\hat a`      reduced contact radius (dimensionless)
==================  ===============

:cite:`Maugis1991` introduced normalized (dimensionless) numbers of some quantities on adhesive contacts; they are useful for validating models against published plots regardless of the value of input parameters. These units are defined as follows:

.. math::
   :nowrap:

   \begin{align}
      \hat a&=a\left(\frac{K}{pi\gamma R^2}\right)^{\frac{1}{3}}, \\
      \hat\delta&=\delta\left(\frac{K^2}{\pi^2\gamma^2 R}\right)^{\frac{1}{3}}, \\
      \hat P&=P\frac{1}{\pi\gamma R}.
   \end{align}

Derjaugin-Muller-Toporov (DMT)
==============================

The DMT model superposes adhesion outside of the area of contact as predicted by the Hertz model. The magnitude of adhesion is derived from surface energy :math:`\gamma` using thermodynamic approach (:cite:`Modenese2013` pg 39; :cite:`Derjaguin1975` derives adhesion force for one surface with nonzero surface energy, here we suppose they have together the surface energy of :math:`2\gamma`):

.. math:: P_{na}=-4\pi R\gamma

which acts against (negative sign) the elastic repulsion of particles :ref:`(eq) <eq_hertz_elastic>`; the total normal force then reads :math:`P_n=P_{ne}+P_{nv}+P_{na}`. Contact radius :math:`a` can be expressed from :ref:`(eq) <eq_contact_radius_general>` by replacing :math:`P_{ne}` with total normal force :math:`P_n`.

.. todo:: Mention JKR model, then MD as the one which accounts for the transition between both, and finally COS as a good approximation of MD.

Schwarz model
=============

Schwarz :cite:`Schwarz2003` proposes model which accounts for the DMT-JKR transition and, unlike the Maugis "Dugdale" model (:cite:`Maugis1991`), gives the :math:`a(F)` and :math:`\delta(a)` analytically. The surface energy is decomposed as short-range adhesion :math:`w_1` and long-range adhesion :math:`w_2`, i.e.

.. math:: \gamma=w_1+w_2
   :label: schwarz-gamma-w1-w2

and their proportion determines whether the model is DMT-like or JKR-like. The critical load is superposition of critical forces found in the JKR and DMT models respectively (:cite:`Schwarz2003` (23)):

.. math:: P_c=-\frac{3}{2}\pi R w_1-2\pi R w_2.
   :label: schwarz-pc-w

DMT-JKR transition
------------------

For convenience, we will use the :math:`\alpha\in\langle 0\dots 1\rangle` dimensionless parameter introduced by Carpick, Ogletree and Salmeron in :cite:`Carpick1999` defined as (:cite:`Schwarz2003` (34))

.. math:: \alpha^2=-\frac{3 P_c+6\pi R \gamma}{P_c}

and by rearranging

.. math:: P_c=-\frac{6\pi R \gamma}{\alpha^2+3}.
   :label: schwarz-pc-alpha

Combining :eq:`schwarz-gamma-w1-w2`, :eq:`schwarz-pc-w` and :eq:`schwarz-pc-alpha` leads to

.. math:: \gamma\left(\frac{-12}{\alpha^2+3}+4\right)=w_1

and by further rearrangements we can relate the ratio of short-range and long-range adhesion forces to :math:`\alpha` both ways:

.. math::

   \frac{w_1}{w_2}&=\frac{3(1-\alpha^2)}{4\alpha^2}

   \alpha^2=\frac{3}{4\frac{w_1}{w_2}+3}

We can see that extreme values of :math:`\alpha` recover DMT or JKR models (in the table below), and intermediate values represent transition between them:

============== =============== =============== ========
:math:`\alpha` :math:`w_1`     :math:`w_2`     model
-------------- --------------- --------------- --------
0                        0     :math:`\gamma`  DMT
1               :math:`\gamma`              0  JKR
============== =============== =============== ========

Contact radius
---------------

Contact radius and force are related by the function (:cite:`Schwarz2003` (36))

.. math:: a=\left(\frac{R}{K}\right)^{\frac{1}{3}}\left(\alpha\sqrt{-P_c}\pm\sqrt{P_n-P_c}\right)^{\frac{2}{3}}

which is analytically invertible to

.. math:: P_n=\left(\sqrt{a^3\frac{K}{R}}-\alpha\sqrt{-P_c}\right)^2+P_c.

The following plot shows both functions (dots are the inverse relationship; this plot appears in :cite:`Maugis1991`, Fig. 5.):

.. plot::

   import woo.models
   alphaGammaName=[(1.,.1,'JKR'),(.5,.1,''),(.01,.1,'$\\to$DMT')]
   woo.models.SchwarzModel.normalized_plot('a(F)',alphaGammaName)

Peneration
-----------

Penetration is given as (:cite:`Schwarz2003` (27))

.. math:: \delta=\frac{a^2}{R}-4\sqrt{\frac{\pi a}{3 K}\left(\frac{P_c}{\pi R}+2\gamma\right)}
   :label: schwarz-delta-pc

where :math:`K` is the "effective elastic modulus" computed as 

.. math:: K=\frac{3}{4}\left(\frac{1-\nu_1^2}{E_1}+\frac{1-\nu_2^2}{E_2}\right)^{-1}=\frac{3}{4}E.

Plugging :eq:`schwarz-pc-alpha` into :eq:`schwarz-delta-pc`, we obtain

.. math:: \delta=\frac{a^2}{R}-4\underbrace{\sqrt{\frac{2\pi\gamma}{3K}\left(1-\frac{3}{\alpha^2+3}\right)}}_{\xi}\sqrt{a}=\frac{a^2}{R}-4\xi\sqrt{a}

where the :math:`\xi` term was introduced for readability. This equation is not analytically invertible and has to be solved numerically. We can find the global minimum as

.. math::
   :nowrap:

   \begin{align}
      \delta'(a)&=\frac{2a}{R}-4\xi a^{\frac{1}{2}} \\
      \delta'(a_\min)&=0 \\
      a_\min&=(R\xi)^{\frac{2}{3}} \\
      \delta_\min&=\delta(a_\min)=3R^{\frac{1}{3}}\xi^{\frac{4}{3}}.
   \end{align}

The second derivative

.. math:: \delta''(a)=\underbrace{\frac{2}{R}}_{>0}+\underbrace{\xi a^{-\frac{3}{2}}}_{\geq 0}>0

is strictly positive as :math:`\xi`, :math:`R` and positive and :math:`a` non-negative.

Given known penetration :math:`\delta`, we can find the corresponding value of :math:`a` with `Newton-Raphson <http://en.wikipedia.org/wiki/Newton-Raphson>`__ or `Halley's <http://en.wikipedia.org/wiki/Halley%27s_method>`__ methods. There are two solutions for all :math:`\delta\in(\delta_\min\dots 0\rangle`. The solution for the ascending branch (:math:`\delta'(a<a_\min)>0`) is energetically unstable and we can ignore it in numerical simulations. As initial solution for iteration, the value of e.g. :math:`2a_\min` can be used when the contact is :obj:`fresh <woo.dem.Contact.isFresh>`, the previous value of :math:`a` is a good starting point otherwise.

This plot shows both loading and unloading (unstable) branches, obtained via Newton iteration (bisection for the unstable branch for simplicity); this plot reproduces :cite:`Maugis1991`, Fig. 6.:

.. plot::

   import woo.models
   alphaGammaName=[(1.,.1,'JKR'),(.5,.1,''),(.01,.1,'$\\to$DMT')]
   woo.models.SchwarzModel.normalized_plot('a(delta)',alphaGammaName)

By composing :math:`P_n(a)` and (numerically evaluated) :math:`a(\delta)`, we obtain the displacement-force relationship (:cite:`Maugis1992`, Fig. 7.)

.. plot::

   import woo.models
   alphaGammaName=[(1.,.1,'JKR'),(.5,.1,''),(.01,.1,'$\\to$DMT')]
   woo.models.SchwarzModel.normalized_plot('F(delta)',alphaGammaName)


