.. _linear_contact_model:

==============================
Linear (Cundall) contact model
==============================

Linear contact model is widely used throughout the DEM field and was first published in :cite:`Cundall1979`. Normal force is linear function of normal displacement (=overlap); shear force increases linearly with relative shear displacement, but is limited by Coulomb linear friciton. This model is implemented in :obj:`woo.dem.Law2_L6Geom_FrictPhys_IdealElPl`.

Stiffness
----------

.. _fig-spheres-contact-stiffness:

.. figure:: fig/spheres-contact-stiffness.*
   
	Series of 2 springs representing normal stiffness of contact between 2 spheres.

Normal stiffness is related to :obj:`Young modulus <woo.dem.ElastMat.young>` of both particles' materials. For clarity, we define :obj:`contact area <woo.dem.L6Geom.contA>` of a fictious "connector" between spheres of total length :math:`l=l_1+l_2`. These :obj:`effective lengths <woo.dem.L6Geom.lens>` are roughly equal to radii for spheres (but have other values for e.g. :obj:`walls <woo.dem.Wall>` or :obj:`facets <woo.dem.Facet>`). The contact area is :math:`A=\pi\min(r_1,r_2)^2` (where :math:`r_i` is (equivalent -- for non-spheres) radius the respective particle). The connector is therefore an imaginary cylinder with radius of the lesser sphere, spanning between their centers. Its :obj:`stiffness <woo.dem.FrictPhys.kn>` is then

.. math:: k_n=\left(\frac{l_1}{E_1 A}+\frac{l_2}{E_2 A}\right)^{-1}.

Tangent (shear) stiffness :math:`k_t` is a :obj:`fraction <woo.dem.FrictMat.ktDivKn>` of :math:`k_n`,

.. math:: k_t=\left(\frac{k_t}{k_n}\right)k_t,

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

Trial tangential force is computed from :obj:`relative velocity <woo.dem.L6Geom.vel>` :math:`\dot u` incrementally and finally (:obj:`optionally <woo.dem.Law2_L6Geom_FrictPhys_IdealElPl.noSlip>`) reduced by the coulomb Criterion:

.. math::
	:nowrap:

	\begin{align*}
		\Delta F_t&=(\pprev{\dot u})_t\Dt k_t, \\
		F_t^T=\curr{F_t}+\Delta F_t,
	\end{align*}

and total tangential force is  reduced by the Coulomb criterion:

.. math:: \next{F_t}=\begin{cases} \curr{F_t}+\Delta F_t & \text{if } |\curr{F_t}+\Delta F_t|<F_n\tan\phi, \\  F_n\tan\phi\frac{\curr{F_t}+\Delta F_t}{|\cdot|} & \text{otherwise}. \end{cases}


