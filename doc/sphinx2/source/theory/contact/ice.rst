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
* twisting stiffness :math:`k_w=\alpha_w k_n A`, where :math:`\alpha_w` is material parameter and :math:`A` is geometry-dependent :obj:`contact area <woo.dem.L6Geom.contA>`.
* rolling stiffness :math:`k_r=\alpha_r k_t A`, where :math:`\alpha_r` is material parameter.

:math:`\alpha_w` and :math:`\alpha_r` are stored together (in this order) as 2-vector in :obj:`IceMat.alpha <woo.dem.IceMat.alpha>`.

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

Limit force values depend on cohesion parameters; the normal cohesion is computed as

.. math:: c_n=\underbrace{l\left(\frac{l_1}{E_1}+\frac{l_2}{E_2}\right)^{-1}}_{E'}\eps_{bn}
   :label: ice-cn
   
:math:`E'` being equivalent Young's modulus and :math:`\eps_{bn}` being strain at breakage in the normal sense (material parameter). Other cohesions are computed from :math:`c_n` by multiplying that value by dimensionless scaling parameters :math:`\beta_t`, :math:`\beta_w`, :math:`\beta_b` (stored as 3-vector in :obj:`IceMat.beta <woo.dem.IceMat.beta>`).

Cohesion values are only useful for senses which are both bonded and breakable, and the breakage condition is slightly different for different senses:

.. math::
   :nowrap:

   \begin{align*}
      F_{nb}&=c_n A, & F_{n}& > F_{nb}\quad \mbox{(no breakage for $F_n<0$ (compression))}, \\
      F_{tb}&=\beta_t c_n A, & |\vec{F}_{t}|&> F_{tb}, \\
      T_{wb}&=\beta_w c_n A^{\frac{3}{2}}, & |T_{w}|&>T_{wb}, \\
      T_{rb}&=\beta_b c_n A^{\frac{3}{2}}, & |\vec{T}_{b}|&>T_{rb}.
   \end{align*}

Plasticity
-----------

Plastic force limiters (yield values) apply only for senses which are currently not bonded (be they broken, or simply never bonded at all). If force/torque exceeds respective yield force/torque, it is limited to that yield value (retaining its direction).

   There are two plastic parameters, friction angle :math:`\phi` (:obj:`~woo.dem.FrictMat.tanPhi`) and kinetic friction :math:`\mu` (:obj:`~woo.dem.IceMat.mu`), used to compute yield values. Note that the use of :math:`\min(0,F_n \dots)` implies that the *yield values are always zero in tension*, therefore the behavior is ideally plastic in that case. 

.. math::
   :nowrap:

   \begin{align*}
      F_{ty}&=-\min(0,F_n\tan\phi)\\
      T_{wy}&=-\sqrt{A/\pi} \min(0,F_n\tan\phi) \\
      T_{ry}&=-\sqrt{A/\pi} \min(0,F_n\mu)
   \end{align*}

Note that all these values are non-negative, since :math:`\min(0,F_n)`\leq0` and :math:`\mu\geq0`, :math:`\tan\phi\geq0`.


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
     - factors for computing :math:`k_w`, :math:`k_r` from :math:`k_n`, :math:`k_t`
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
