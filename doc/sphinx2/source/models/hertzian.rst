.. _hertzian_contact_models:


************************
Hertzian contact models
************************

Hertz contact model is non-linear model which takes in account elastic deformation of spherical solids and changing contact radius. Pure Hertzian model only describes normal deformation of spheres. Later extensions of the model add viscosity, shear response including friction and adhesion. The nomenclature usually used in literature on these models is the following (see :cite:`Modenese2013`):

==================  ===============
:math:`R`           effective radius
:math:`E`           equivalent elastic modulus
:math:`K`           effective elastic modulus (:math:`K=\frac{3}{4}E`)
:math:`m^{*}`       effective mass   
:math:`\delta`      normal geometrical overlap of spheres; :math:`\delta=r_1+r_2-d` for two spheres (positive when there is overlap), numerically corresponds to :obj:`woo.dem.L6Geom.uN` with the sign inversed.
:math:`a`           contact radius, i.e. radius of the contacting area
:math:`\epsilon`    coefficient of restitution
:math:`\eta`        viscous coefficient (as in :math:`F=\eta\dot u`)
:math:`\dot\delta`  rate of overlap (:obj:`normal displacement rate <woo.dem.L6Geom.vel>`)
:math:`p`           function fo contact pressure
:math:`P_{ne}`      normal elastic force; integral of :math:`p` over the contact area
:math:`P_{nv}`      normal viscous force
:math:`P_n`         total normal external force acting on the particle
==================  ===============

Equivalent radius is computed as harmonic means of particle's radii

.. math:: \frac{1}{R}=\frac{1}{R_1}+\frac{1}{R_2}

which gives correct result also for a sphere-plane contacts (plane having :math:`R_2=\infty`).

Equivalent mass 

.. math:: \frac{1}{m^{*}}=\frac{1}{m_1}+\frac{1}{m_2}

is used for in models of viscosity; :math:`m_i` is taken to be zero if the particles is not subject to contact forces (such as when :obj:`woo.dem.DemData.blocked` is set for massless objects like :obj:`facets <woo.dem.Facet>` or :obj:`walls <woo.dem.Wall>`).

Moduli (different authors use either :math:`E` or :math:`K=\frac{3}{4}E`) are computed as 

.. math::

   \frac{1}{E}=\frac{1-\nu_1^2}{E_1}+\frac{1-\nu_2^2}{E_2},

   \frac{1}{K}=\frac{4}{3}\left(\frac{1-\nu_1^2}{E_1}+\frac{1-\nu_2^2}{E_2}\right)


where :math:`E_i` are Young's moduli of contacting particles (:obj:`woo.dem.ElastMat.young`) and :math:`\nu_i` their Poisson's ratios (:obj:`woo.dem.Cp2_FrictMat_HertzPhys.poisson`).

Overlap :math:`\delta` is geomerically related to the contact radius :math:`a` by the relationship

.. math:: a^2=R\delta.

A good summary of various contact models is given at `Wikipedia <http://en.wikipedia.org/wiki/Contact_mechanics>`_.

.. warning:: Literature on contact mechanics mostly gives positive sign to compressive stress. This is opposite than the convension used throughout Woo. Therefore the overlap :math:`\delta` is positive when spheres are overlapping (whereas :obj:`woo.dem.L6Geom.uN` is negative) and contact force :math:`P_{n}` is positive for compression (repulsion) whereas the normal (:math:`x`) component of :obj:`woo.dem.CPhys.force`  would be negative. Keep this in mind when reading sources.

Hertz contact
=============

Hertz gives contact pressure as 

.. math:: p(r)=p_0\sqrt{1-\left(\frac{r}{a}\right)^2}

with :math:`r\in(0,r)` and :math:`p_0` being the maximum pressure in the middle of the contact area; it follows by integration

.. math:: P_{ne}=\int_0^a p(r) 2\pi r \mathrm{d}\,r=\frac{2}{3}p_0\pi a^2.

Hertz also relates the overlap :math:`\delta` to the contact force :math:`P_{ne}` by

.. _eq_hertz_elastic:

.. math:: P_{ne}=\underbrace{\frac{4}{3}E\sqrt{R}}_{k_{n0}}\delta^{\frac{3}{2}}=k_{n0}\delta^{\frac{3}{2}}
	:label: hertz-elastic

where the :math:`k_{n0}` term is often separated (:obj:`woo.dem.HertzPhys.kn0`) as it is constant throughout the contact duration. :obj:`Secant stiffness <woo.dem.FrictPhys.kn>` of the contact is expressed as 

.. math:: k_n=\frac{\partial P_{ne}}{\partial \delta}=\frac{3}{2}k_{n0}\delta^{\frac{1}{2}}.

By combining the above, we also obtain:

.. _eq_contact_radius_general:

.. math:: a=\sqrt[3]{\frac{3R}{4E}P_{ne}}.
	:label: contact-radius-general

:cite:`Antypov2011` also references Landau & Lifschitz's analytical solution for collision time of purely elastic collision as 

.. math:: \tau_{\rm Hertz}=2.214 \left(\frac{\rho}{E}\right)^{\frac{2}{5}}\frac{r_1+r_2}{v_0^{1/5}}

where :math:`\rho` is density of particles. There is a :obj:`regression test <woo.tests.hertz.TestHertz.testElasticCollisionTime>` verifying that simulated collision gives the same result.

Viscosity
---------

Coefficient of restitution
^^^^^^^^^^^^^^^^^^^^^^^^^^

Observable collisions of particles usually result in some energy dissipation of which measure is `coefficient of restitution <http://en.wikipedia.org/wiki/Coefficient_of_restitution>`_ which can be expressed as the ratio of relative velocity before and after collision, :math:`\epsilon=v_0/v_1` where :math:`v_0`, :math:`v_1` are relative velocities before and after collision respectively.

The coefficient must be squared when reasoning in terms of energy with kinetic energy :math:`E_k` and :math:`E_d` the dissipated energy, since kinertic energy :math:`E_k=\frac{1}{2}mv^2` contains the velocity squared:

.. math:: \epsilon=\sqrt{\frac{E_{k0}-E_{k1}}{E_{k0}}}=\sqrt{\frac{E_d}{E_{k_0}}}.

Similarly, potential field (gravity) can be used to determine coefficient of restitution based on potential energy (:math:`E_p=mgh`) for particle falling from intial height :math:`h_0` with zero initial velocity, rebounding from horizontal plane and again reaching zero velocity at :math:`h_1`:

.. math:: \epsilon=\sqrt{\frac{h_1}{h_0}}.

Viscous damping
^^^^^^^^^^^^^^^

Viscosity adds force :math:`P_c` linearly related to the current rate of overlap :math:`\dot\delta` with linear term :math:`\eta`, the viscous coefficient

.. math:: P_{nv}=\eta\dot\delta .

The viscous coefficient :math:`\eta` is not straight-forwardly realted to the coefficient of restition :math:`C_R`, which is an integral measure over the whole collision time.

This problem is treated by :cite:`Antypov2011` in detail, which derives analytical relationships for viscous coefficient and coefficient of restitution in Hertzian contact. In order to obtain dissipative force resulting in velocity-independent coefficient of restitution, the force must take the form

.. math:: P_{nv}(\delta)=\alpha(\epsilon)\sqrt{m^* k_{n0}}\delta^{\frac{1}{4}}\dot\delta.
	:label: hertz-viscous

(this result was previously known, but :math:`\alpha(\epsilon)` was only numerically evaluated) and :math:`\alpha(\epsilon)` is shown to be

.. math:: \alpha(\epsilon)=\frac{-\sqrt{5}\ln\epsilon}{\sqrt{\ln^2\epsilon+\pi^2}}.

The part which is constant throughout the contact life, :math:`\alpha(\epsilon)\sqrt{m^*k_{n0}}` is stored in :obj:`woo.dem.HertzPhys.alpha_sqrtMK`.

The total contact force :math:`P_n` is superposition of the elastic response :math:`P` :eq:`hertz-elastic` and normal viscous force :math:`P_{nv}` :eq:`hertz-viscous`, i.e.

.. math:: P_n=P_{ne}+P_{nv}.
	:label: hertz-viscoleastic

Nonphysical attraction
^^^^^^^^^^^^^^^^^^^^^^
The presence of viscous force can lead to attraction between spheres towards the end of collision (:math:`\dot\delta<0`) when :math:`P_n=P_{ne}+P_{nv}<0`. This effect is non-physical in the absence of adhesion. For this reason, such attractive force is ignored, though it can be changed by setting :obj:`woo.dem.Law2_L6Geom_HertzPhys_DMT.noAttraction`. 

This effect is discussed in :cite:`Antypov2011` and should be taken in account when measuring coefficients of restitution in simulations (see `this discussion <https://answers.launchpad.net/yade/+question/235934>`__).

