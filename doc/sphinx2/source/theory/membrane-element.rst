=================
Membrane element
=================

Membrane elements are special 3-node particles in DEM. They have their mass lumped into nodes and respond to plane-stress stretching (CST element) and bending (DKT element; optional, enabled by setting the :obj:`bending flag <woo.dem.In2_FlexFacet_ElastMat.bending>`). These two elements are superposed. The element has constant thickness. Each node has 6 DoFs:

#. two in-plane translations (gathered in :obj:`woo.dem.FlexFacet.uXy` for all nodes as a 6-vector), used by the CST element;
#. one out-of-plane translation -- defines rigid motion of the whole element and causes no internal response;
#. two in-plane rotations (gathered in :obj:`woo.dem.FlexFacet.phiXy` for all nodes as a 6-vector), used by the DKT element;
#. one out-of-plane rotation (drilling DoF), which is ignored.

Lumping mass into nodes is not automatic. The user is responsible for assigning proper values of :obj:`woo.dem.DemData.mass` and :obj:`woo.dem.DemData.inertia` to nodes.

Element coordinate system
-------------------------

When the element is first used, its configuration defines the reference configuration which is stored in :obj:`woo.dem.FlexFacet.refPos` and :obj:`woo.dem.FlexFacet.refOri`.

The element coordinate system (held in :obj:`woo.dem.FlexFacet.node`) is always such that

#. local origin coindices with centroid of the (deformed) triangle;
#. the local :math:`z`-axis is normal to the plane defined by element nodes in their current positions;
#. local :math:`x` and :math:`y` axes are oriented in a way which minimizes displacements of nodes in the local plane relative to the :obj:`reference positions <woo.dem.FlexFacet.refPos>`, and tries to represent their mutual displacements as rigid body motion as much as possible.

   This point is trivially satisfied in the reference configuration (first time the element is used) when displacements are zero with arbitrary orientation of the local :math:`x` and :math:`y` axes. (The implementation currently chooses to rotate the :math:`x`-axis so that it passes through the first node.)

   Otherwise, we are looking for a best-fit rotation; the algorithm is described in :cite:`FelippaNFEM` in `Appendix C <http://www.colorado.edu/engineering/cas/courses.d/NFEM.d/NFEM.AppC.d/NFEM.AppC.pdf>`__. Nodes positions are projected onto (not yet properly rotated) local :math:`xy` plane, and best-fit angle is computed (from reference positions :math:`X_i`, :math:`Y_i` and current local positions :math:`x_i`, :math:`y_1`) as 

   .. math:: \tan\theta_3=\frac{x_1 Y_1 + x_2 Y_2 + x_3 Y_3 - y_1 X1 - y_2 X_2 - y_3 X_3}{x_1 X_1 + x_2 X_2 + y_3 Y_3 + y_1 Y_1 + y_2 Y_2 + y_3 Y_3}

   where the numerator and denominator are computed separately in the implementation and :math:`\tan\phi_3` is obtained vi via the `atan2 <http://en.wikipedia.org/wiki/Atan2>`__ function as to preserve quadrant information and handle zero denominator correctly. The :math:`\theta_3` rotation is then added to the previous rotation of the :math:`xy` plane, obtaining the best-fit coordinate system.

Once the local coordinate system is established, displacements are evaluated. Evaluating :obj:`woo.dem.FlexFacet.uXy` is done as trivial subtraction of reference nodal positions from the current ones in the local system. Rotations :obj:`woo.dem.FlexFacet.phiXy` are computed by rotation "subtraction" (conjugate product) using quaternions (:obj:`woo.dem.FlexFacet.refRot` holds the rotation necessary to obtain each node orientation from global element orientation). See the source code for details.

Constant Strain Triangle (CST) Element
--------------------------------------

The CST element is implemented following :cite:`FelippaIFEM`, `chapter 15 <http://www.colorado.edu/engineering/cas/courses.d/IFEM.d/IFEM.Ch15.d/IFEM.Ch15.pdf>`__. The strain-displacement matrix is computed as (15.17):

.. math:: \mat{B}=\frac{1}{2A}\begin{pmatrix}y_{23} & 0 & y_{31} & 0 y_{12} & 0 \\ 0 & x_{23} & 0 & x_{12} & 0 & x_{21} \\ x_{32} & y_{23} & x_{12} & y_{31} & x_{21} & y_{12}\end{pmatrix}\begin{pmatrix} u_{x1} \\ u_{y1} \\ y_{x2} \\ u_{y2} \\ u_{x3} \\ u_{y3}\end{pmatrix}.

The elastic stiffness matrix for plane-stress conditions reads

.. math:: \mat{E}=\frac{E}{1-\nu^2}\begin{pmatrix}1 & \nu & 0 \\ \nu & 1 & 0 \\ 0 & 0 & \frac{1-\nu}{2}\end{pmatrix}

where :math:`E` is :obj:`Young modulus <woo.dem.ElastMat.young>` and :math:`\nu` is the :obj:`Poisson ratio <woo.dem.In2_FlexFacet_ElastMat>`.

Since :obj:`membrane thickness <woo.dem.In2_FlexFacet_ElastMat.thickness>` :math:`h` is constant over the element, we may write the stiffness matrix as (:cite:`FelippaIFEM`, (15.21)):

.. math:: \mat{K^{\mathrm{CST}}}=A h \mat{B}^T \mat E \mat B.

This matrix is stored as :obj:`woo.dem.FlexFacet.KKcst`.

:obj:`Displacements <woo.dem.FlexFacet.uXy>` :math:`\vec{u}` (6-vector) are computed by subtracting :obj:`reference positions <woo.dem.FlexFacet.refPos>` from the current nodal positions in best-fit local coordinates. Nodal forces (6-vector) are 

.. math:: \vec{F}^{\mathrm{CST}}=\begin{pmatrix}F_{x1}\\F_{y1}\\F_{x2}\\F_{y2}\\F_{x3}\\F_{y3}\end{pmatrix}=\mat{K}^{\mathrm{CST}}\vec{u}.

This is an example of a CST-only mesh (no bending):

.. youtube:: jimWu0_8oLc

Discrete Krichhoff Triangle (DKT) Element
-----------------------------------------

The DKT (bending) element is implemented following :cite:`Batoz1980`. The bending elasticity matrix for constant thickness is computed as 

.. math:: \mat{D}_b=\frac{E h^3}{12(1-\nu^2)}\begin{pmatrix}1 & \nu & 0 \\ \nu & 1 & 0 \\ 0 & 0 & \frac{1-\nu}{2}\end{pmatrix}

where the thickness :math:`h` may be :obj:`different thickness <woo.dem.In2_FlexFacet_ElastMat.bendThickness>` than the one used for the CST element. The strain-displacement transformation matrix for bending reads (:cite:`Batoz1980`, (30))

.. math:: \mat{B}_{b}(\xi,\eta)=\frac{1}{2A}\begin{pmatrix}y_{31}\vec{H}_{x,\xi}^T+y_{12}\vec{H}_{x,\eta}^T \\ -x_{31}\vec{H}_{y,\xi}^T-x_{12}\vec{H}_{y,\eta}^T \\ -x_{31}\vec{H}_{x,\xi}^T-x_{12}\vec{H}_{x,\eta}^T+y_{31}\vec{H}_{y,\xi}^T+y_{12}\vec{H}_{y,\eta}^T \end{pmatrix}.

where :math:`x_{ij}\equiv x_i-x_j`, :math:`y_{ij}\equiv y_i-y_j` and :math:`(\xi,\eta)` are natural (≡barycentric) coordinates on the triangle. Refer to Appendix A of :cite:`Batoz1980` (or the source code of :obj:`woo.dem.FlexFacet`) for formulation of :math:`\vec{H}_{x,\xi}`, :math:`\vec{H}_{x,\eta}`, :math:`\vec{H}_{y,\xi}`, :math:`\vec{H}_{y,\eta}`, which are rather convolved.

The stiffness matrix is integrated over the triangle as (:cite:`Batoz1980`, (31))

.. math:: \mat{K}^{\mathrm{DKT}}=2A\int_0^1\int_0^{1-\eta}\mat{B}^T\mat{D}_b\mat{B}\d\xi\,\d\eta

and can be integrated numerically (see :cite:`Kansara2004`, pg. 48) using the `Gauss quadrature <http://en.wikipedia.org/wiki/Gaussian_quadrature>`__ (Gauss points are mid-points of triangle sides, :math`w` is vector of point weights) over the unit triangle as

.. math::
   :nowrap:

   \begin{align*}
      \vec{\xi}&=\left(\frac{1}{2},\frac{1}{2},0\right),\\
      \vec{\eta}&=\left(0,\frac{1}{2},\frac{1}{2}\right),\\
      \vec{w}&=\left(\frac{1}{3},\frac{1}{2},\frac{1}{2}\right),\\
      \mat{K}^{\mathrm{DKT}}&=2A\sum_{j=1}^{3}\sum_{i=1}^{3} w_j w_i \mat{B}_b^T(\xi_i,\eta_j) \mat{D}_b \mat{B}_b(\xi_i,\eta_j).
   \end{align*}

Generalized nodal forces are computed as

.. math::

   \vec{F}^{\mathrm{DKT}}=\begin{pmatrix} F_{z1} \\ T_{x1} \\ T_{y1} \\ F_{z2} \\ T_{x1} \\ T_{x2} \\ F_{z3} \\ T_{x3}  \\ T_{y3}\end{pmatrix}=\mat{K}^{\mathrm{DKT}}\begin{pmatrix}u_{z1}\equiv0\\ \phi_{x1} \\ \phi_{y1} \\ u_{z2}\equiv0 \\ \phi_{x2} \\ \phi_{y2} \\ u_{z3}\equiv 0 \\ \phi_{x3} \\ \phi_{y3}\end{pmatrix},

Since out-of-plane translations :math:`u_{zi}` are always zero (they determine rigid body rotation of the element), we may condense those rows from :math:`\mat{K}_{\mathrm{DKT}}` away, making it 9×6 rather than 9×9, and removing zero elements from the generalized displacement vector as well. Note that corresponding DoFs may nevertheless have non-zero forces :math:`F_{zi}`.

The following video shows the combined response of CST and DKT elements:

.. youtube:: KmQWD_MfR8M

Total nodal forces
------------------

Total nodal forces are  superimposed from both elements and also from contact forces on the particle and from a special load type on membranes, hydrostatic pressure. We note force and torque contributions as :math:`\Delta \vec{F}_i`, :math:`\Delta \vec{T}_i` for :math:`i`-th node, :math:`i\in\{1,2,3\}`.

Hydrostatic pressure
"""""""""""""""""""""

Elements may be under :obj:`surface load <woo.dem.FlexFacet.surfLoad>` which is always perpendicular to the element plane, thus representing hydrostatic pressure. Such pressure :math:`p` is distributed into nodes as

.. math:: \Delta\vec{F}_i = \begin{pmatrix} 0 \\ 0 \\ p\frac{A^*}{3} \end{pmatrix}

where :math:`A^*` is the current (not reference) element surface area.

Contact forces
"""""""""""""""

Contact forces defined by contact point :math:`\vec{c}`, force :math:`\vec{F}_c` and torque :math:`\vec{T}_c` are simply distributed onto all nodes, so that each node :math:`i` (positioned at :math:`\vec{x}_i`) receives for each contact the contribution

.. math::
   :nowrap:

   \begin{lign*}
      \Delta\vec{F}_i&=\frac{\vec{F}_c}{3}, \\
      \Delta\vec{T}_i&=(\vec{x}_i-\vec{c})\times\vec{F}_c+\vec{T}_c.
   \end{align*}

CST + DKT
"""""""""

Generalized forces from elements are distributed to nodes as follows:

.. math::
   :nowrap:

   \begin{align*}
      \Delta\vec{F}_i&=\begin{pmatrix} \vec{F}^{\mathrm{CST}}_{xi} \\ \vec{F}^{\mathrm{CST}}_{yi} \\ -\vec{F}^{\mathrm{DKT}}_{zi} \end{pmatrix}, \\
      \Delta\vec{T}_i&=\begin{pmatrix} \vec{F}^{\mathrm{DKT}}_{\phi_{xi}} \\ \vec{F}^{\mathrm{DKT}}_{\phi_{yi}} \\ 0 \end{pmatrix}.
   \end{align*}

.. note:: The value of :math:`\vec{T}_{zi}` (drilling torque) is always zero; therefore, drilling motion of nodes is unconstrained (though still governed by dynamics). This can lead to some problems in special cases, manifesting as wobbly rotation of nodes which does not go away.

This video demonstrates the :obj:`cylindrical triaxial test <woo.pre.cylTriax.CylTriaxialTest>` which includes CST+DKT elements with hydrostatic pressure and interaction with particles inside the membrane (displacements are scaled 10×):

.. youtube:: Li13NrIyMYU
