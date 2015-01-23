.. _linear_contact_model:

==============================
Linear (Cundall) contact model
==============================

Linear contact model is widely used throughout the DEM field and was first published in :cite:`Cundall1979`. Normal force is linear function of normal displacement (=overlap); shear force increases linearly with relative shear displacement, but is limited by Coulomb linear friciton. This model is implemented in :obj:`woo.dem.Law2_L6Geom_FrictPhys_IdealElPl`.

Stiffness
----------

.. _fig-spheres-contact-stiffness:

.. figure:: fig/spheres-contact-stiffness.*
   :align: center
   
   Series of 2 springs representing normal stiffness of contact between 2 spheres.

Normal stiffness is related to :obj:`Young modulus <woo.dem.ElastMat.young>` of both particles' materials. For clarity, we define :obj:`contact area <woo.dem.L6Geom.contA>` of a fictious "connector" between spheres of total length :math:`l=l_1+l_2`. These :obj:`effective lengths <woo.dem.L6Geom.lens>` are roughly equal to radii for spheres (but have other values for e.g. :obj:`walls <woo.dem.Wall>` or :obj:`facets <woo.dem.Facet>`). The contact area is :math:`A=\pi\min(r_1,r_2)^2` (where :math:`r_i` is (equivalent -- for non-spheres) radius the respective particle). The connector is therefore an imaginary cylinder with radius of the lesser sphere, spanning between their centers. Its :obj:`stiffness <woo.dem.FrictPhys.kn>` is then

.. math:: k_n=\left(\frac{l_1}{E_1 A}+\frac{l_2}{E_2 A}\right)^{-1}.
   :label: linear-kn
   

Tangent (shear) stiffness :math:`k_t` is a :obj:`fraction <woo.dem.FrictMat.ktDivKn>` of :math:`k_n`,

.. math:: k_t=\left(\frac{k_t}{k_n}\right)k_n,
   :label: linear-kt

where the ratio is averaged between both materials in contact.

:obj:`Friction angle <woo.dem.FrictPhys.tanPhi>` on the contact is computed from contacting materials as

.. math:: \tan\phi=\min\left[(\tan\phi)_1,(\tan\phi)_2\right],

(unless :obj:`specified otherwise <woo.dem.Cp2_FrictMat_FrictPhys.tanPhi>`) of which consequence is that material without friction will not have frictional contacts, regardless of friction of the other material.

.. note:: It is a gross simplification to derive friction parameters from material properties -- the interface of each couple of materials might have different parameters, though this simplification is mostly sufficient in the practice. If you need to define different parameters for every combination of material instances, there is the :obj:`woo.core.MatchMaker` infrastructure.

Contact respose
----------------
Normal response is computed from the :obj:`normal displacement <woo.dem.L6Geom.uN>` (or overlap) as

.. math:: F_n=u_n k_n

and the contact is (:obj:`optionally <woo.dem.Law2_L6Geom_FrictPhys_IdealElPl.noBreak>`) broken when :math:`u_n>0`.

Trial tangential force is computed from tangential :obj:`relative velocity <woo.dem.L6Geom.vel>` :math:`\dot u` incrementally and finally (:obj:`optionally <woo.dem.Law2_L6Geom_FrictPhys_IdealElPl.noSlip>`) reduced by the coulomb Criterion. Tangential force is a 2-vector along contact-local :math:`y` and :math:`z` axes.

.. math::
   :nowrap:

   \begin{align*}
      \Delta \vec{F}_t&=(\pprev{\vec{\dot u}})_t\Dt k_t, \\
      \vec{F}_t^T&=\curr{\vec{F}_t}+\Delta \vec{F}_t,
   \end{align*}

and total tangential force is reduced by the Coulomb criterion:

.. math:: \next{\vec{F}_t}=\begin{cases} \curr{\vec{F}_t}+\Delta \vec{F}_t & \text{if } |\curr{\vec{F}_t}+\Delta \vec{F}_t|<F_n\tan\phi, \\  F_n\tan\phi\frac{\curr{\vec{F}_t}+\Delta \vec{F}_t}{|\cdot|} & \text{otherwise}. \end{cases}


Energy balance
--------------

Elastic potential stored in a contact is the contact is the triangular area below the displacement-force diagram, in both normal and tangent sense,

.. math:: \frac{1}{2}\left(\frac{F_n^2}{k_n}+\frac{|\vec{F}_t|^2}{k_t}\right).

Plastically dissipated energy is the elastic energy removed by the tangent slip. Noting :math:`f_y=F_n\tan\phi` (yield force magnitude), and norm of the trial tangent force :math:`f_t=|\curr{\vec{F}_t}+\Delta\vec{F}_t|`, this energy can be seen as the red area (parallelogram) in the displacement-force diagram

.. tikz::

   \fill[fill=red!40, fill opacity=50] (0,0)--(4,3)--(4,4)--(0,1) -- cycle;
   \draw[->] (-.5,0)--(4.5,0) node[anchor=north]{$|u_t|$};
   \draw[->] (0,-.5)--(0,4) node[anchor=east]{$|F_t|$};
   \draw (0,0)--(4.5,3.375);
   \draw[dashed] (0,1)--(4,4);
   \draw[->] (1,0) arc(0:36.87:1) node[anchor=north west]{$\;k_t$};
   \draw[<-,very thick] (4,3)--(4,4) node[anchor=north west]{slip};

leading to

.. math:: \Delta E_{pt}=\frac{f_t-f_y}{k_t}f_y.

.. todo:: The code has a different formulation: one contribution of :math:`\frac{1}{2}\frac{f_t-f_y}{k_t}(f_t-f_y)` and another of :math:`f_y\frac{f_t-f_y}{k_t}`, giving together

   .. math:: \Delta E_{pt}=\left(\frac{f_t-f_y}{2}+f_y\right)\frac{f_t-f_y}{k_t}.

   The difference of elastic potentials leads to yet another formulation:

   .. math:: \Delta E_{pt}=\frac{1}{2}\frac{f_t^2}{k_t}-\frac{1}{2}\frac{f_y^2}{k_t}=\frac{f_t^2-f_y^2}{2k_t}.

   Decide analytically on which of those is the best approximation and use it both in the code and in the docs.


================================
Purely elastic model 6-DoF model
================================

This model is useful for testing purposes. It has elastic response along all 6 DoFs: normal displacement, 2 shear displacements, twisting and 2 bending rotations. There is no non-linearity (like plastic beahvior). The model is implemented in :obj:`woo.dem.Law2_L6Geom_FrictPhys_LinEl6`. For simplicity, it uses :obj:`woo.dem.FrictPhys` (ignoring :obj:`~woo.dem.FrictPhys.tanPhi`) to compute elastic parameters, plus an additional attribute :obj:`~woo.dem.Law2_L6Geom_FrictPhys_LinEl6.charLen` to compute rotational stiffnesses. Thus the stiffnesses are as follows:

* normal and tangential stiffnesses :obj:`~woo.dem.FrictPhys.kn`, :obj:`~woo.dem.FrictPhys.kt` are computed as above in eqs. :eq:`linear-kn`, :eq:`linear-kt`. Their dimension is N/m² as usual.
* bending stiffnesses :math:`k_w` (twist) and :math:`k_b` (bending) are computed using the factor :math:`l` (:obj:`~woo.dem.Law2_L6Geom_FrictPhys_LinEl6.charLen`), which ensures dimensional consistency, as

.. math::
   :nowrap:
   
   \begin{align*}
      k_w&=l k_n, & k_b&=l k_s,
   \end{align*}

which means that :math:`k_t/k_n=k_b/k_w`; this should be acceptable since the model is used for testing only.

Contact forces are computed strictly elastically as

.. math::
   :nowrap:

   \begin{align*}
      F_n&=k_n u_n, & \vec{F}_t&=k_t u_t, \\
      T_n&=k_w \phi_n, & \vec{T}_b&=k_b \vec{\phi}_t.
   \end{align*}

Stored elastic energy is computed as

.. math::

   E_e=\frac{1}{2}\left(\frac{F_n^2}{k_n}+\frac{|\vec{F}_t|^2}{k_t}+\frac{T_n}{k_w}+\frac{|\vec{T}_b|^2}{k_b}\right)

with careful handling of zero stiffnesses so that the expression is numerically sound.

