.. _ice-contact-model:

=============================
Ice contact model
=============================

.. note:: Developement of this model was sponsored by `Centre for Arctic Resource Development (CARD) <http://www.card-arctic.ca>`__.

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

Limit force value depends on cohesion parameters (:math:`c_n`, :math:`c_t`, :math:`c_w`, :math:`c_r`) and is only useful for senses which are both bonded and breakable, and the breakage condition is slightly different for different senses:

.. math::
   :nowrap:

   \begin{align*}
      F_{nb}&=c_n A, & F_{n}& > F_{nb}\quad \mbox{(no breakage for $F_n<0$ (compression))}, \\
      F_{tb}&=c_t A, & |\vec{F}_{t}|&> F_{tb}, \\
      T_{wb}&=c_w A^{\frac{3}{2}}, & |T_{w}|&>T_{wb}, \\
      T_{rb}&=c_b A^{\frac{3}{2}}, & |\vec{T}_{b}|&>T_{rb}.
   \end{align*}

Plasticity
-----------

Plastic force limiters apply only for senses which are currently not bonded (they could be broken, or were never bonded at all). If force/torque exceeds respective yield force/torque, it is limited to that yield value (in the original direction).

There are two plastic parameters, friction angle :math:`\phi` (:obj:`~woo.dem.FrictMat.tanPhi`) and kinetic friction :math:`\mu`.

.. math::
	:nowrap:

	\begin{align*}
		F_{ty}&=\min(0,F_n\tan\phi) \quad\mbox{(zero shear in tension)} \\
		T_{wy}&=\sqrt{\frac{A}{pi}} \min(0,F_n\tan\phi) \quad\mbox{(zero twist in tension)}
	\end{align*}

.. todo:: Rolling friction? Where is it applied, what does it depend on? Robert wrote :math:`F_r=F_n\mu`, but... what is the orientation of the force? It must depend on whether the contact is deforming or not (rolling?), and must result in force/torque acting at the contact point.


