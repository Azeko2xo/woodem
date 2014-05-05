.. _theory-motion-integration:

Motion integration
===================

Motion integration consists in computing accelerations from forces/torques acting on nodes carrying mass and updating their positions/oritentations. Velocities are update based on accelerations, and positions are advanced.

.. note:: Let us repeat: It must be carefully observed for which time-point is any value valid; this is show by using time-indices :math:`\prev{x}\equiv x^{(t-\Dt)}`, :math:`\pprev{x}\equiv x^{(t-\Dt/2)}`, :math:`\curr{x}\equiv x^{(t)}`, :math:`\nnext{x} \equiv x^{(t+\Dt/2)}`, :math:`\next{x} \equiv x^{(t+\Dt)}`. Because of the leap-frog integration scheme, even derivatives of position (position, acceleration, forces, torques) are known at full-steps while odd derivatives (velocity) are known at mid-steps.

Motion integration is implemented in :obj:`woo.dem.Leapfrog`.

The summary force :math:`\vec{F}_{\Sigma}` and torque :math:`\vec{T}_{\Sigma}` include contributions from all relevant contacts, fields (such as gravity) and internal forces (e.g. from :ref:`membrane_element`).

.. todo:: Mention how are forces from contacts summed up.


Position
---------
Integrating position consists in using current acceleration :math:`\curr{\vec{a}}\equiv\curr{\ddot{\vec{u}}}` on a node to update its position from the current value of :obj:`position <woo.core.Node.pos>` :math:`\curr{\vec{u}}` to its value at the next timestep :math:`\next{\vec{u}}`; we know the node's :obj:`velocity <woo.dem.DemData.vel>` :math:`\pprev{\vec{v}}\equiv\pprev{\dot{\vec{u}}}`. Computation of acceleration, knowing the :obj:`current force <woo.dem.DemData.force>` :math:`\vec{F}_{\Sigma}` acting on the node in question and its :obj:`mass <woo.dem.DemData.mass>` :math:`m`, is simply

.. math:: \curr{\vec{a}}=\frac{\curr{\vec{F}_{\Sigma}}}{m}

Using the 2-nd order finite difference approximation with step :math:`\Dt`, we obtain

.. math:: \curr{\vec{a}}&\cong\frac{\prev{\vec{u}}-2\curr{\vec{u}}+\next{\vec{u}}}{\Dt^2}

from which we express

.. math::

	\next{\vec{u}}&=2\curr{\vec{u}}-\prev{\vec{u}}+\curr{\vec{a}}\Dt^2 =\\
		&=\curr{\vec{u}}+\Dt\underbrace{\left(\frac{\curr{\vec{u}}-\prev{\vec{u}}}{\Dt}+\curr{\vec{a}}\Dt\right)}_{(\dagger)}.

Typically, :math:`\prev{\vec{u}}` is already not known (only :math:`\curr{\vec{u}}` is); we notice, however, that

.. math:: \pprev{\vec{v}}&\simeq\frac{\curr{\vec{u}}-\prev{\vec{u}}}{\Dt},

i.e. the mean velocity during the previous step, which is known. Plugging this approximate into the :math:`(\dagger)` term, we also notice that mean velocity during the current step can be approximated as

.. math:: \nnext{\vec{v}}&\simeq\pprev{\vec{v}}+\curr{\vec{a}}\Dt,

which is :math:`(\dagger)`; we arrive finally at

.. math:: \next{\vec{u}}&=\curr{\vec{u}}+\Dt\left(\pprev{\vec{v}}+\curr{\vec{a}}\Dt\right).

The algorithm can then be written down by first computing current mean velocity :math:`\nnext{\vec{v}}` which we need to store for the next step (just as we use its old value :math:`\pprev{\vec{v}}` now), then computing the position for the next time step :math:`\next{\vec{u}}`:

.. math:: 
   :label: eq-leapfrog-nnextvel

   \nnext{\vec{v}}=\pprev{\vec{v}}+\curr{\vec{a}}\Dt \\
   \next{\vec{u}}=\curr{\vec{u}}+\nnext{\vec{v}}\Dt.


Positions are known at times :math:`i\Delta t` (if :math:`\Delta t` is constant) while velocities are known at :math:`i\Delta t+\frac{\Delta t}{2}`. The fact that they interleave (jump over each other) in such way gave rise to the colloquial name "leap-frog".


Orientation
------------

Spherical nodes
^^^^^^^^^^^^^^^^^

The basic integration procedure applies to nodes which have inertia tensor such that :math:`\tens{I}_{11}=\tens{I}_{22}=\tens{I}_{33}` (this tensor is always diagonal, since local node coordinates coincide with principal axes, and is stored as a 3-vector in :obj:`woo.dem.DemData.inertia`; the sphericity can be queried via :obj:`isAspherical <woo.dem.DemData.isAspherical>`).

.. math::
   :nowrap:
   :label: eq-leapfrog-nnextangvel

	\begin{align*}
      \curr{\dot{\vec{\omega}}}&=\frac{\curr{\vec{T}_{\Sigma}}}{\tens{I}_{11}} \\
      \nnext{\vec{\omega}}&=\pprev{\vec{\omega}}+\Dt\curr{\dot{\vec{\omega}}} \\
      \next{\vec{q}}&=\mathrm{Quaternion}(\nnext{\vec{\omega}}\Dt)\curr{\vec{q}}
	\end{align*}
	

Aspherical nodes
^^^^^^^^^^^^^^^^^

Aspherical nodes have different moment of inertia along each principal axis. Their positions are integrated in the same ways as with spherical nodes.

Integrating rotation is considerably more complicated as the local reference frame is not inertial. Rotation of rigid body in the local frame, where inertia tensor :math:`\mat{I}` is diagonal, is described in the continuous form by Euler's equations (:math:`i\in\{1,2,3\}`, and :math:`i`, :math:`j`, :math:`k` being sequential):

.. math:: \vec{T}_i=\mat{I}_{ii}\dot{\vec{\omega}}_i+(\mat{I}_{kk}-\mat{I}_{jj})\vec{\omega}_j\vec{\omega}_k.

Due to the presence of the current values of both :math:`\vec{\omega}` and :math:`\dot{\vec{\omega}}`, they cannot be solved using the standard leapfrog algorithm.
			
The algorithm presented here is described by :cite:`Allen1989` (pg. 84--89) and was designed by Fincham for molecular dynamics problems; it is based on extending the leapfrog algorithm by mid-step/on-step estimators of quantities known at on-step/mid-step points in the basic formulation. Although it has received criticism and more precise algorithms are known (:cite:`Omelyan1999`, :cite:`Neto2006`, :cite:`Johnson2008`), this one is currently implemented in Woo for its relative simplicity.

.. Finchman: Leapfrog Rotational Algorithms: http://www.informaworld.com/smpp/content~content=a756872469&db=all
	Schvanberg: Leapfrog Rotational Algorithms: http://www.informaworld.com/smpp/content~content=a914299295&db=all

			
Each node has its local coordinate system aligned with the principal axes of inertia; we use :math:`\tilde{\bullet}` to denote vectors in local coordinates. The orientation of the local system is given by the current :obj:`node orientation <woo.core.Node.ori>` :math:`\curr{q}` as a quaternion; this quaternion can be expressed as the (current) rotation matrix :math:`\mat{A}`. Therefore, every vector :math:`\vec{a}` is transformed as :math:`\tilde{\vec{a}}=q\vec{a}q^{*}=\mat{A}\vec{a}`. Since :math:`\mat{A}` is a rotation (orthogonal) matrix, the inverse rotation :math:`\mat{A}^{-1}=\mat{A}^{T}`.

For the node in question, we know

* :math:`\curr{\tilde{\mat{I}}}` (constant) :obj:`inertia tensor diagonal <woo.dem.DemData.inertia>` (non-diagonal items are zero, since local coordinates are principal),
* :math:`\curr{\vec{T}}` external :obj:`torque <woo.dem.DemData.torque>`,
* :math:`\curr{q}` current :obj:`orientation (<woo.core.Node.ori>` and its equivalent rotation matrix :math:`\mat{A}`),
* :math:`\pprev{\vec{\omega}}`  mid-step :obj:`angular velocity <woo.dem.DemData.angVel>`,
* :math:`\pprev{\vec{L}}` mid-step :obj:`angular momentum <woo.dem.DemData.angMom>`; this is an auxiliary variable that must be tracked additionally for use in this algorithm. It will be zero in the initial step.

Our goal is to compute new values of the latter three, i.e. :math:`\nnext{\vec{L}}`, :math:`\next{q}`, :math:`\nnext{\vec{\omega}}`. We first estimate current angular momentum and compute current local angular velocity:

.. math::
	:nowrap:

	\begin{align*}
		\curr{\vec{L}}&=\pprev{\vec{L}}+\curr{\vec{T}}\frac{\Dt}{2}, &\curr{\tilde{\vec{L}}}&=\mat{A}\curr{\vec{L}}, \\
		\nnext{\vec{L}}&=\pprev{\vec{L}}+\curr{\vec{T}}\Dt, &\nnext{\tilde{\vec{L}}}&=\mat{A}\nnext{\vec{L}}, \\
		\tilde{\curr{\vec{\omega}}}&=\curr{\tilde{\mat{I}}}{}^{-1}\curr{\tilde{\vec{L}}}, \\
		\nnext{\tilde{\vec{\omega}}}&=\curr{\tilde{\mat{I}}}{}^{-1}\nnext{\tilde{\vec{L}}}. \\
	\end{align*}

Then we compute :math:`\curr{\dot{q}}` (see `Quaternion differentiation <http://www.euclideanspace.com/physics/kinematics/angularvelocity/QuaternionDifferentiation2.pdf>`__), using :math:`\curr{q}` and :math:`\curr{\tilde{\vec{\omega}}}`:

.. math::
	:label: eq-quaternion-derivative
	:nowrap:

		\begin{align*}
			\begin{pmatrix}\curr{\dot{q}}_w \\ \curr{\dot{q}}_x \\ \curr{\dot{q}}_y \\ \curr{\dot{q}}_z\end{pmatrix}&=
				\def\cq{\curr{q}}
				\frac{1}{2}\begin{pmatrix}
					\cq_w & -\cq_x & -\cq_y & -\cq_z \\
					\cq_x & \cq_w & -\cq_z & \cq_y \\
					\cq_y & \cq_z & \cq_w & -\cq_x \\
					\cq_z & -\cq_y & \cq_x & \cq_w
				\end{pmatrix}
				\begin{pmatrix} 0 \\ \curr{\tilde{\vec{\omega}}}_x \\ \curr{\tilde{\vec{\omega}}}_y \\ \curr{\tilde{\vec{\omega}}}_z	\end{pmatrix},  \\
				\nnext{q}&=\curr{q}+\curr{\dot{q}}\frac{\Dt}{2}.\\
		\end{align*}

We evaluate :math:`\nnext{\dot{q}}` from :math:`\nnext{q}` and :math:`\nnext{\tilde{\vec{\omega}}}` in the same way as in :eq:`eq-quaternion-derivative` but shifted by :math:`\Dt/2` ahead. Then we can finally compute the desired values

.. math::
	:nowrap:

	\begin{align*}
		\next{q}&=\curr{q}+\nnext{\dot{q}}\Dt, \\
		\nnext{\vec{\omega}}&=\mat{A}^{-1}\nnext{\tilde{\vec{\omega}}}.
	\end{align*}


Motion in uniformly deforming space
------------------------------------

In some simulations, nodes can be considered as moving within uniformly deforming medium with velocity gradient tensor :math:`\tens{L}`, being stationary at origin. This functionality is provided by the :obj:`woo.core.Cell` class (:obj:`Scene.cell <woo.core.Scene.cell>`), along with periodic boundaries. The equations written here only valid when :obj:`Cell.homoDeform <woo.core.Cell.homoDeform>` has the value ``Cell.homoGradV2``, which is the default.

.. note:: It is (currently) not possible to simulate deforming space without periodicity. As workaround, set the periodic space big enough to encompass the whole simulation.

.. note:: The node's :obj:`linear <woo.dem.DemData.vel>` and :obj:`angular <woo.dem.DemData.angVel>` velocities comprise both space and fluctuation velocity. It is (almost) only in the integrator that the two must be distinguished.

Spin is skew-symmetric (rotational) part of velocity gradient :math:`\tens{W}=\frac{1}{2}(\tens{L}-\tens{L}^T)`; its `Horge dual <http://en.wikipedia.org/wiki/Hodge_dual>` vector (noted :math:`*\bullet`) is position-independent medium angular velocity. Local medium velocities (in cartesian coordinates) read

.. math::

   \vec{v}_L&=\tens{L}\vec{x} \\
   \vec{\omega}_L&=*\tens{W}\!/2

where :math:`\vec{\omega}_L` is written in cartesian coordinates component-wise as

.. math:: \omega_{Lk}=\frac{1}{2}\epsilon_{ijk}W_{ij}

where :math:`\epsilon_{ijk}` is the `Levi-Civita symbol <http://en.wikipedia.org/wiki/Levi-Civita_symbol>`__ (also known as "permutation tensor").

Linear velocity
^^^^^^^^^^^^^^^
Velocity :math:`\vec{v}` of each particle is sum of fluctuation velocity :math:`\vec{v}_f` and local medium velocity :math:`\vec{v}_L`. :math:`\vec{v}` evolves not only by virtue of acceleration, but also of :math:`\tens{L}`, defined at mid-step time-points. We add this term to :eq:`eq-leapfrog-nnextvel`

.. math:: \nnext{\vec{v}}=\pprev{\vec{v}}+\Dt\curr{\vec{a}}+\Dt\curr{\vec{\dot v}_L}
   :label: eq-nnext-v-simple

with 

.. math:: 
   :nowrap:
   :label: eq-hvl-currv

   \begin{align*}
      \Dt\curr{\vec{\dot v}_L}&=\Dt\partial_t(\curr{\tens{L}}\curr{\vec{x}})=\Dt(\curr{\tens{\dot L}}\curr{\vec{x}}+\curr{L}\curr{v})\approx\\
      &\approx \Dt \left[\frac{\nnext{\tens{L}}-\pprev{\tens{L}}}{\Dt}\curr{\vec{x}}+\frac{\pprev{\tens{L}}+\nnext{\tens{L}}}{2}\curr{\vec{v}}\right].
   \end{align*}

This equation can be rearranged as

.. math:: \Dt\curr{\vec{\dot v}_L}=-\pprev{\tens{L}}(\underbrace{\curr{\vec{x}}-\curr{\vec{v}}\frac{\Dt}{2}}_{\approx\pprev{\vec{x}}})+\nnext{\tens{L}}(\underbrace{\curr{\vec{x}}+\curr{\vec{v}}\frac{\Dt}{2}}_{\approx\nnext{\vec{x}}})\approx-\pprev{\tens{L}}\pprev{\vec{x}}+\nnext{\tens{L}}\nnext{\vec{x}}

showing that the correction :math:`\Dt\curr{\vec{\dot v}_L}` corresponds to subtracting the previous field velocity and adding the current field velocity.

Going back to :eq:`eq-hvl-currv`, we write the unknown on-step velocity as :math:`\curr{\vec{v}}\approx (\pprev{\vec{v}}+\nnext{\vec{v}})/2` and substitute :math:`\Dt\curr{\vec{\dot v}_L}` into :eq:`eq-nnext-v-simple` obtaining

.. math::
   \nnext{\vec{v}}&=\left(\mat{1}-\frac{\nnext{\tens{L}}+\pprev{\tens{L}}}{4}\Dt\right)^{-1}\left[(\nnext{\tens{L}}-\pprev{\tens{L}})\curr{\vec{x}}+\left(\mat{1}+\frac{\nnext{\tens{L}}+\pprev{\tens{L}}}{4}\Dt\right)\pprev{\vec{v}}+\curr{\vec{a}}\Dt\right].

The position-independent terms are stored in :obj:`ImLL4hInv <woo.dem.Leapfrog.ImLL4hInv>`, :obj:`LmL <woo.dem.Leapfrog.LmL>`, :obj:`IpLL4h <woo.dem.Leapfrog.IpLL4h>` and are updated at each timestep. Both :math:`\pprev{\tens{L}}` and :math:`\nnext{\tens{L}}` must be know in this equations; they are stored in :obj:`woo.core.Cell.gradV` and :obj:`woo.core.Cell.nextGradV` respectively.

Angular velocity
^^^^^^^^^^^^^^^^

As :math:`\vec{\omega}_L` is only a function of time (not of space, unlike :math:`\vec{v}_L`), :eq:`eq-leapfrog-nnextangvel` simply becomes

.. math:: \nnext{\vec{\omega}}&=\pprev{\vec{\omega}}+\Dt\curr{\dot{\vec{\omega}}} \underbrace{- \pprev{\vec{\omega}_L}+\nnext{\vec{\omega}_L}}_{(\dagger)}

where the :math:`(\dagger)` term is stored as :obj:`deltaSpinVec <woo.dem.Leapfrog.deltaSpinVec>`.

.. warning:: Uniformly deforming medium integration is not implemented for aspherical particles, their rotation will be integrated pretending :math:`\tens{W}` is zero.

Periodic space
^^^^^^^^^^^^^^^

Periodic space is defined as parallelepiped located at origin, expressed as :obj:`matrix <woo.core.Cell.hSize>` :math:`\mat{H}` of which columns are its side vectors. Periodic space volume is given by :math:`|\det\mat{H}|`.

:math:`\mat{H}` follows motion of the space which can be written down as motion of a point :math:`p` with zero fluctuation velocity and acceleration; linearizing its motion around mid-step, we obtain its next position as

.. math::
		\next{\vec{p}}=\curr{\vec{p}}+\Dt\nnext{\tens{L}}\nnext{\vec{p}}\approx\curr{\vec{p}}+\Dt\nnext{\tens{L}}\frac{\curr{\vec{p}}+\next{\vec{p}}}{2}

from which :math:`\next{\vec{p}}` is found as

.. math::
   :label: eq-fixed-point

   \next{\vec{p}}=\left(\mat{1}-\nnext{\tens{L}}\frac{\Dt}{2}\right)^{-1}\left(\mat{1}+\nnext{\tens{L}}\frac{\Dt}{2}\right)\curr{\vec{p}}.

The :math:`\mat{H}` matrix is updated in the same way, i.e.

.. math:: \next{\mat{H}}=\left(\mat{1}-\nnext{\tens{L}}\frac{\Dt}{2}\right)^{-1}\left(\mat{1}+\nnext{\tens{L}}\frac{\Dt}{2}\right)\curr{\mat{H}}.


The :obj:`transformation matrix <woo.core.Cell.trsf>` accumulates deformation from :math:`\tens{L}` and is updated as

.. math:: \next{\mat{T}}=\curr{\mat{T}}+\pprev{\tens{L}}\curr{\mat{T}}\Dt.

If :math:`\tens{L}` is diagonal, then the (initially zero) :math:`\mat{T}` is also diagonal and is a strain matrix. Diagonal contains the `Hencky strain <http://en.wikipedia.org/wiki/Deformation_%28mechanics%29#True_strain>`__ along respective axes.

Clumps
-------

.. todo:: Write about clump motion integration; write about :obj:`woo.dem.ClumpSpherePack` and how its properties are computed. Perhaps move clumps to a separate file.

Timestep
---------

.. todo:: Write on :math:`\Dt`, how :obj:`Scene <woo.core.Scene>` queries :obj:`fields <woo.core.Field.critDt>` and :obj:`engines <woo.core.Engine.critDt>` on their timestep, about :obj:`woo.core.Scene.dtSafety`, about :obj:`woo.dem.DynDt`.
