.. _ice-contact-model:

=============================
Ice contact model
=============================

.. note:: Developement of this model is commercially sponsored (identity undisclosed).

The ``Ice`` model was conceived for ice simulations originally, but may be suitable for a wide range of materials. It considers elasticity, cohesion and friction in 4 senses (normal, shear, twisting, rolling); each of the senses can be independently fragile ("unbonding" upon reaching critical force/torque value -- see below) or non-fragile (plastic slip taking place when critical force/torque is reached).

The model consists of a set of classes for material parameters (:obj:`~woo.dem.IceMat`, :obj:`~woo.dem.IceMatState`, :obj:`~woo.dem.IcePhys`, :obj:`~woo.dem.Cp2_IceMat_IcePhys`, :obj:`~woo.dem.Law2_L6Geom_IcePhys`) but consumes :obj:`~woo.dem.L6Geom`, generic contact geometry object. Therefore, it works for any combination of :obj:`shapes <woo.dem.Shape>` handled by :obj:`~woo.dem.L6Geom`.

Values related to respective degrees of freedom are denoted with the following indices:

#. :math:`\bullet_n` for normal sense (translations along the local :math:`x`-axis);
#. :math:`\bullet_t` for tangential sense (translations in the local :math:`yz`-plane; may be 2d vectors if indicated);
#. :math:`\bullet_w` for twisting sense (rotations around the local :math:`x`-axis);
#. :math:`\bullet_r` for rolling sense (rotations around local :math:`y` and :math:`z` axes; may be 2d vectors if indicated).

Stiffnesses
-----------

There are 4 stiffness values:

* normal and tangential stiffnesses :math:`k_n` (:obj:`~woo.dem.FrictPhys.kn`) and :math:`k_t` (:obj:`~woo.dem.FrictPhys.kt`) are computed the same as in :ref:`linear_contact_model`.
* twisting stiffness :math:`k_w=\alpha_w k_t A`, where :math:`\alpha_w` is material parameter and :math:`A` is geometry-dependent :obj:`contact area <woo.dem.L6Geom.contA>`. 

* rolling stiffness :math:`k_r=\alpha_r k_n A`, where :math:`\alpha_r` is material parameter.

:math:`\alpha_w` and :math:`\alpha_r` are stored together (in this order) as 2-vector in :obj:`IceMat.alpha <woo.dem.IceMat.alpha>`.


.. note:: *Twisting* stiffness is proportional to *tangential* stiffness and *rolling* stiffness is proportional to *normal* stiffness. (This is unlike for limit breakage forces, where normal--twist and tangential--rolling are proportional.) The rationale is that analogy with isotropic circular beam, where the rescpective stiffnesses are related to the same moduli (Young's modulus :math:`E` and shear modulus :math:`G`)

  .. math::
    :nowrap:

    \begin{align*}
       k_n&=\frac{EA}{L}, & k_t&=\frac{GA}{L}, \\
       k_w&=\frac{GJ}{L}, & k_r&=\frac{EI}{L}
    \end{align*}

  with :math:`A=\pi R^2`, :math:`J=\frac{\pi R^4}{2}`, :math:`I=\frac{\pi R^4}{4}`, :math:`G=\frac{E}{2(1+\nu)}`. Thus, to emulate isotropic beam on the contact, the coefficients would be

  .. math::
     \alpha_w=\frac{1}{2\pi},

     \alpha_r=\frac{1}{4\pi}.

Bonds
------

The contact state can be bonded (cohesive) or unbonded in any of the 4 senses, independently. The initial bonding state (stored per-sense as bitmask in :obj:`~woo.dem.IcePhys.bonds`) is determined by the :obj:`~woo.dem.Cp2_IceMat_IcePhys` functor, and the possibility for breakage in 4 different senses is also initially set there.

Breakage
"""""""""

#. Bond breaks (i.e. the contact becomes non-cohesive) when two conditions are met simultaneously:

   * force/torque in any sense exceeds limit value (see below);
   * the bond is breakable (fragile) in that sense, as indicated by flag for given sense (stored again in :obj:`~woo.dem.IcePhys.bonds`).

#. When breakage in any sense occurs, the bond is broken in all senses at once and becomes fully unboded.

#. The transition from unbonded to bonded state never occurs naturally (though it can be forced by hand).

Limit force values depend on cohesion parameters; the normal cohesion (size-independent stress) is computed as

.. math:: c_n=\underbrace{l\left(\frac{l_1}{E_1}+\frac{l_2}{E_2}\right)^{-1}}_{E'}\eps_{bn}
   :label: ice-cn
   
:math:`E'` being equivalent Young's modulus and :math:`\eps_{bn}` being "strain" (as in :math:`\eps_{bn}=\Delta l/l`) at breakage in the normal sense (material parameter :obj:`IceMat.breakN <woo.dem.IceMat.breakN>`), :math:`l=l_1+l_2` is total center distance and radii, as used  in :ref:`linear_contact_model` (or equivalent measures used to distribute stiffness, when contacting particles are not spheres).

Cohesion values are only useful for senses which are both bonded and breakable, and the breakage condition is slightly different for different senses. The values are all computed from :math:`c_n` using :math:`A` for correct dimension and :math:`\beta_t`, :math:`\beta_w`, :math:`\beta_b` (stored as 3-vector in :obj:`IceMat.beta <woo.dem.IceMat.beta>`) as dimensionless scaling parameters:

.. math::
   :nowrap:

   \begin{align*}
      F_{n}& > F_{nb},         & F_{nb}&=c_n A, \\
      |\vec{F}_{t}|& > F_{tb}, & F_{tb}&=\beta_t F_{nb}=\beta_t c_n A, \\
      |T_{w}|& > T_{wb},       & T_{wb}&=\beta_w F_{nb}A^{\frac{1}{2}}=\beta_w c_n A^{\frac{3}{2}}, \\
      |\vec{T}_{b}|& > T_{rb}, & T_{rb}&=\beta_r F_{tb}A^{\frac{1}{2}}=\beta_r \beta_t c_n A^{\frac{3}{2}}.
   \end{align*}

Note that there is no absolute value in the first equation, as there is no breakage in compression (:math:`F_n<0`).

Plasticity
-----------

Plastic force limiters (yield values, noted :math:`\bullet_y`) apply only for senses which are currently not bonded (be they broken, or simply never bonded at all). If force/torque exceeds respective yield force/torque, it is limited to that yield value (retaining its direction).

There are two plastic parameters, friction angle :math:`\phi` (:obj:`~woo.dem.FrictMat.tanPhi`) and kinetic friction :math:`\mu` (:obj:`~woo.dem.IceMat.mu`), used to compute yield values. Note that the use of :math:`\min(0,F_n \dots)` implies that the *yield values are always zero in tension*, therefore the behavior is ideally plastic in that case. 

.. math::
   :nowrap:

   \begin{align*}
      F_{ty}&=-\min(0,F_n\tan\phi)\\
      T_{wy}&=-\sqrt{A/\pi} \min(0,F_n\tan\phi) \\
      T_{ry}&=-\sqrt{A/\pi} \min(0,F_n\mu)
   \end{align*}

Note that all these values are non-negative, since :math:`\min(0,F_n)\leq0` and :math:`\mu\geq0`, :math:`\tan\phi\geq0`.

For each unbonded sense, when the yield condition is satisfied, the corresponding force is reduced to return to the yield value:

.. math::
   :nowrap:

   \begin{align*}
      |\vec{F}_t|&>F_{ty} &  \Longrightarrow && \vec{F}_t&\leftarrow \normalized{\vec{F}_t} F_{ty}, \\
      |T_w|&>T_{wy} &  \Longrightarrow && T_w&\leftarrow \normalized{T_w} T_{wy}, \\
      |\vec{T}_r|&>T_{ry} &  \Longrightarrow && \vec{T}_r&\leftarrow \normalized{\vec{T}_r} T_{ry}.
   \end{align*}



Nomenclature
-------------

.. list-table::
   :widths: 15 10 30 10 50
   :header-rows: 1

   * - Symbol
     - Unit
     - Variable
     - Algorithm
     - Meaning
   * - :math:`E`
     - Pa
     - :obj:`ElastMat.young <woo.dem.ElastMat.young>`
     - geometry-weighted
     - Young's modulus
   * - :math:`A`
     - :math:`\mathrm{m^2}`
     - :obj:`L6Geom.contA <woo.dem.L6Geom.contA>`
     - computed
     - contact area
   * - :math:`k_t/k_n`
     - --
     - :obj:`FrictMat.ktDivKn <woo.dem.FrictMat.ktDivKn>`
     - averaged
     - factor to compute :math:`k_t` from :math:`k_n`
   * - :math:`\tan\phi`
     - --
     - :obj:`FrictMat.tanPhi <woo.dem.FrictMat.tanPhi>`, :obj:`FrictPhys.tanPhi <woo.dem.FrictPhys.tanPhi>`
     - minimum
     - friction angle
   * - :math:`(\alpha_w,\alpha_r)`
     - --
     - :obj:`IceMat.alpha <woo.dem.IceMat.alpha>`
     - averaged
     - factors for computing :math:`k_w`, :math:`k_r` from :math:`k_t`, :math:`k_n`
   * - :math:`\eps_{bn}`
     - --
     - :obj:`IceMat.breakN <woo.dem.IceMat.breakN>`
     - averaged
     - normal strain where cohesion stress is reached
   * - :math:`c_n`
     - Pa
     - temporary
     - computed from :eq:`ice-cn`
     - normal cohesion value
   * - :math:`(\beta_t,\beta_w,\beta_r)`
     - --
     - :obj:`IceMat.beta <woo.dem.IceMat.beta>`
     - averaged
     - factors for computing cohesions from :math:`c_n`
   * - :math:`(F_{nb},F_{tb}), (T_{wb},T_{rb})`
     - (N ,N, Nm, Nm)
     - :obj:`IcePhys.brkNT <woo.dem.IcePhys.brkNT>`, :obj:`IcePhys.brkWR <woo.dem.IcePhys.brkWR>`
     - computed
     - limit forces for bond breakage
   * - :math:`\mu`
     - --
     - :obj:`IceMat.mu <woo.dem.IceMat.mu>`, :obj:`IcePhys.mu <woo.dem.IcePhys.mu>`
     - averaged
     - kinetic (rolling) friction coefficient
   * - :math:`k_n, k_t, k_w, k_r`
     - N/m, N/m, N, N
     - :obj:`FrictPhys.kn <woo.dem.FrictPhys.kn>`, :obj:`FrictPhys.kt <woo.dem.FrictPhys.kt>`, :obj:`IcePhys.kWR <woo.dem.IcePhys.kWR>`
     - computed
     - stifffnesses in all 4 senses
