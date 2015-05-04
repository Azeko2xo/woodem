.. _l6geom:

Local contact coordinates
--------------------------

Contact geometry is given by local coordinates (situated at the contact point and having the :math:`x`-axis along the contact normal) and also is responsible for computing relative velocity of contacting particles in local coordinates. This functionality is implemented by functors producing :obj:`woo.dem.L6Geom` (:obj:`woo.dem.Cg2_Sphere_Sphere_L6Geom` and others).


Local coordinates are defined by position (local origin) :math:`c` (contact point) and rotation (local orientation) given as rotation (=orthonormal) matrix :math:`\mat{T}`. The first row of :math:`\mat{T}` i.e. the local :math:`x`-axis, is noted :math:`\vec{n}` for brevity.

There is 6 generalized displacements (degrees of freedom) at the contact point:

relative displacements :math:`\vec{u}`
   the first (:math:`x`) component is the normal displacement :math:`u_n`, the other two are tangential displacement vector :math:`\vec{u_t}`;

relative rotations :math:`\vec{\phi}`
   the :math:`x`-component is the twist rotation :math:`\phi_x`, :math:`\phi_y` and :math:`\phi_z` are bending (in-plane) rotation along respective local axes.

All displacements define their respective rates noted :math:`\dot{\vec{u}}` and :math:`\dot{\vec{\phi}}`.

:obj:`Cg2_* functors <woo.dem.CGeomFunctor>` are responsible for updating these values on every contact. Most of the computation is done incrementally.

.. note:: It must be carefully observed for which time-point is any value valid; this is show by using time-indices :math:`\prev{x}\equiv x^{(t-\Dt)}`, :math:`\pprev{x}\equiv x^{(t-\Dt/2)}`, :math:`\curr{x}\equiv x^{(t)}`, :math:`\nnext{x} \equiv x^{(t+\Dt/2)}`, :math:`\next{x} \equiv x^{(t+\Dt)}`. Because of the leap-frog integration scheme, even derivatives of position (position, acceleration, forces, torques) are known at full-steps while odd derivatives (velocity) are known at mid-steps.


.. _contact-geometry-l6gom-generic:

Generic contact routine 
^^^^^^^^^^^^^^^^^^^^^^^^

Contact of any two :obj:`shapes <woo.dem.Shape>` is handled by the same routine which supposes two spheres with radii :math:`r_i`, positions :math:`\vec{x}_i`, velocities :math:`\vec{v}_i`, angular velocities :math:`\vec{\omega}_i`. If the shapes in contact are not really spheres, we use "equivalent" sphere geometry which captures the configuration in question.

At every step, every contact directly computes (incomplete) current local coordinates: the contact point :math:`\curr{\vec{c}}` and contact normal :math:`\curr{\vec{n}}`. The rest is computed indirectly. This computation is different for different combinations of :obj:`shapes <woo.dem.Shape>` (:obj:`sphere <woo.dem.Sphere>` with :obj:`sphere <woo.dem.Sphere>`, :obj:`wall <woo.dem.Wall>`, :obj:`facet <woo.dem.Facet>` and so on).

Contact formation
"""""""""""""""""

When a contact is initially formed, current values of :math:`\curr{\vec{c}}` and :math:`\curr{\vec{n}}` are computed directly. Local axes (columns of :math:`\mat{T}`) are initially defined as follows:

* local :math:`x`-axis is contact normal :math:`\vec{n}`;
* local :math:`y`-axis is positioned arbitrarily, but in a deterministic manner: aligned with the :math:`xz` plane (if :math:`\vec{n}_y<\vec{n}_z`) or :math:`xy` plane (otherwise);
* local :math:`z`-axis is perpendicular to both other axis :math:`\vec{z}_l=\vec{x}_l\times\vec{y}_l`.

Relative velocities are zero :math:`\dot{\curr{\vec{u}}}=\dot{\curr{\vec{\phi}}}=\vec{0}`, displacements :math:`\vec{u}` and :math:`\vec{\phi}` are also intially zero (normal displacement is often compute in a non-incremental way and may have non-zero initial value).

.. todo:: Is it ok that relative velocities are initially zero, and not updated until in the next step? Those could be computed directly, although with less precision, when the contact is formed.

.. note:: Quasi-constant values of :math:`\vec{u}_0` (and :math:`\vec{\phi}_0`) are stored as shifted origins of :math:`\vec{u}` (and :math:`\vec{\phi}`); therefore, the current value of displacement is always :math:`\curr{\vec{u}}-\vec{u}_0` This is useful for easy implementation of plastic slipping in the normal/twist sense where the value is often computed non-incrementally.

Contact update
""""""""""""""

For a contact which already exists, the value of :math:`\curr{\vec{c}}` and :math:`\curr{\vec{n}}` is computed directly again. The contact is then updated to keep track of rigid motion of the contact (one that does not change mutual configuration of spheres) and mutual configuration changes. Some computations can be more or less approximate (trading performance for accuracy), which is controlled by :obj:`approximation flags <woo.dem.Cg2_Any_Any_L6Geom__Base.approxMask>`.

Rigid motion transforms local coordinate system and can be decomposed in rigid translation (affecting :math:`\vec{c}`), and rigid rotation (affecting :math:`\mat{T}`), which can be split in bending rotation :math:`\vec{o}_r` perpendicular to the normal and twisting rotation :math:`\vec{o}_t` parallel with the normal:

.. math:: \pprev{\vec{o}_r}=\prev{\vec{n}}\times\curr{\vec{n}}.

Since velocities are known at previous midstep (:math:`t-\Dt/2`), we consider mid-step normal

.. math:: \pprev{\vec{n}}\begin{cases}=\frac{\prev{\vec{n}}+\curr{\vec{n}}}{2} & \text{(accurate solution)} \\ \approx\prev{\vec{n}} & \text{(with approximation flag set)}\end{cases}.

For the sake of numerical stability, :math:`\pprev{\vec{n}}` is re-normalized after being computed (unless prohibited by approximation flags).

Rigid rotation parallel with the normal is

.. math:: \pprev{\vec{o}_t}=\pprev{\vec{n}}\left(\pprev{\vec{n}}\cdot\frac{\pprev{\vec{\omega}}_1+\pprev{\vec{\omega}}_2}{2}\right)\Dt.

*Branch vectors* :math:`\vec{b}_1`, :math:`\vec{b}_2` (connecting :math:`\curr{\vec{x}}_1`, :math:`\curr{\vec{x}}_2` with :math:`\curr{\vec{c}}` are computed depending on :obj:`noRatch<Cg2_Any_Any_L6Geom__Base.noRatch>` (see `details in Yade docs <https://www.yade-dem.org/doc/current/yade.wrapper.html#yade.wrapper.Ig2_Sphere_Sphere_ScGeom.avoidGranularRatcheting>`__):

.. math::
   :nowrap:

   \begin{align*}
      \vec{b}_1&=\begin{cases} r_1 \curr{\vec{n}} & \mbox{with noRatch} \\ \curr{\vec{c}}-\curr{\vec{x}}_1 & \mbox{otherwise} \end{cases} \\
      \vec{b}_2&=\begin{cases} -r_2\curr{\vec{n}} & \mbox{with noRatch} \\ \curr{\vec{c}}-\curr{\vec{x}}_2 & \mbox{otherwise} \end{cases} \\
   \end{align*}

Relative velocity at :math:`\curr{\vec{c}}` can be computed as 

.. math:: \pprev{\vec{v}_r}=(\pprev{\vec{\tilde{v}}_2}+\vec{\omega}_2\times\vec{b}_2)-(\vec{v}_1+\vec{\omega}_1\times\vec{b}_1)

where :math:`\vec{\tilde{v}}_2` is :math:`\vec{v}_2` without mean-field velocity gradient in periodic boundary conditions (see :obj:`woo.core.Cell.homoDeform`). In the numerial implementation, the normal part of incident velocity is removed (since it is computed directly) and replaced with with :math:`\pprev{\vec{v}_{r2}}=\pprev{\vec{v}_r}-(\pprev{\vec{n}}\cdot\pprev{\vec{v}_r})\pprev{\vec{n}}`.

Any vector :math:`\vec{a}` expressed in global coordinates transforms during one timestep as

.. math:: \curr{\vec{a}}=\prev{\vec{a}}+\pprev{\vec{v}_r}\Dt-\prev{\vec{a}}\times\pprev{\vec{o}_r}-\prev{\vec{a}}\times{\pprev{\vec{t}_r}}

where the increments have the meaning of relative shear, rigid rotation normal to :math:`\vec{n}` and rigid rotation parallel with :math:`\vec{n}`. Local coordinate system orientation, rotation matrix :math:`\mat{T}`, is updated by columns (written as transpose for space reasons), i.e.

.. math:: \curr{\mat{T}}=\begin{pmatrix} \curr{\vec{n}_x}, \curr{\vec{n}_y}, \curr{\vec{n}_z} \\ \prev{\mat{T}_{1,\bullet}}-\prev{\mat{T}_{1,\bullet}}\times\pprev{\vec{o}_r}-\prev{\mat{T}_{1,\bullet}}\times\pprev{\vec{o}_t} \\ \prev{\mat{T}_{2,\bullet}}-\prev{\mat{T}_{2,\bullet}}\times\pprev{\vec{o}_r}-\prev{\mat{T}_{,\bullet}}\times\pprev{\vec{o}_t} \end{pmatrix}^T

This matrix is re-normalized (unless prevented approximation flags) and mid-step transformation is computed using quaternion spherical interpolation as

.. math:: \pprev{\mat{T}}\begin{cases}=\mathrm{Slerp}\,\left(\prev{\mat{T}};\curr{\mat{T}};t=1/2\right) & \text{(accurate solution)} \\ \approx\prev{\mat{T}} & \text{(with approximation flag set)}\end{cases}.

Finally, current generalized displacements in local coordinates are evaluated as 

.. math::
   :nowrap:

   \begin{align*}
      \curr{\vec{u}}&=\prev{u}+{\pprev{\mat{T}}}^T\pprev{\vec{v}_r}\Dt, \\
      \curr{\vec{\phi}}&=\prev{\vec{\phi}}+{\pprev{\mat{T}}}^T\Dt(\vec{\omega}_2-\vec{\omega}_1)
   \end{align*}

For the normal component, non-incremental evaluation is preferred if possible; for two spheres, this reads

.. math:: \curr{\vec{u}_x}=|\curr{\vec{x}_2}-\curr{\vec{x}_1}|-(r_1+r_2).


