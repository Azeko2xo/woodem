.. _tet4_element:

=============
Tet4 element
=============

:obj:`~woo.fem.Tet4` is the simplest linear-interpolation tetrahedron-shaped element with 4 nodes, each having only 3 translational degrees of freedom (rotations of nodes are ignored).

Element coordinate system
--------------------------

Initial element configuration is the reference one, and element-local nodal positions (orientations are ignored) are stored in :obj:`~woo.fem.Tet4.refPos`. This reference configuration is created automatically when the element is first used (usually when :obj:`woo.dem.DynDt` asks for stiffnesses before the simulation starts, or by an explicit call to :obj:`~woo.fem.Tet4.setRefConf`).

Current element configuration is stored in :obj:`~woo.fem.Tet4.node`; its origin is always at element's centorid (:obj:`~woo.fem.Tetra.getCentroid`), which is computed as average of nodal positions. The orientation is determined using the algorithm described in :cite:`Mostafa2014` (pg. 114, Procedure 1):

1. compute

   * :math:`C`, element-local reference coordinates of nodes (transpose of :obj:`~woo.fem.Tet4.refPos`);
   * :math:`D`, current globally-oriented coordinates of nodes, with the origin moved to the current element centroid (:obj:`~woo.fem.Tetra.getCentroid`)

2. we introduce the following notation:

   * non-unit quaternion :math:`\mathbf{q}` is a 4-tuple :math:`\mathbf{q}=(q_0,q_x,q_y,q_z)=(q_0,q)`, where :math:`q_0` is the real component and :math:`(q_x,q_y,q_z)=q` is 3D vector of imaginary components;
   * :math:`\mathbf{q}^\circ=(0,\hat q)` forms pure-imaginary quaternion from :math:`q`;
   * :math:`\bullet_L` and :math:`\bullet_R` are matrices representing respectively left- and right-multiplication by quaternion :math:`\bullet` (:cite:`Mostafa2014`, A.4-A.5):

     .. math::

        [\mathbf{q}]_L=\pmatrix{ q_0 & -q^T \\ q & q_0 I+\hat q},

        [\mathbf{q}]_R=\pmatrix{ q_0 & -q^T \\ q & q_0 I-\hat q},

     which simplifies, for pure-imaginary arguments, to

     .. math::

        [\mathbf{q}^\circ]_L=\pmatrix{ 0 & -q^T \\ q & +\hat q},

        [\mathbf{q}^\circ]_R=\pmatrix{ 0 & -q^T \\ q & -\hat q};

   * the hat operator :math:`\hat\bullet` is the skew-symmetric matrix representing corss-product by :math:`\bullet` (:cite:`Mostafa2014`, A.1):

     .. math::

        \hat a=\pmatrix{ 0 & -a_3 & a_2 \\ a_3 & 0 & -a_1 \\ -a_2 & a_1 & 0 }.

3. compute :cite:`Mostafa2014`, (8b):

   .. math:: \mathcal{B}(C,D)=\sum_{n=1}^{4}\beta(c_n,d_n)^T\beta(c_n,d_n)

   where :math:`c_n`, :math:`d_n` are respectively :math:`n`-th rows of :math:`C` and :math:`D`; those are 4-tuples, or non-unit quaternions :math:`\mathbf{q}=(q_0,q_x,q_y,q_z)` and the :math:`\beta` function is defined as

   .. math:: \beta(c,d)&=\left[d^\circ\right]_L-\left[c^\circ\right]_R

4. Compute the smallest eigenvalue of the 4x4 :math:`\mathcal{B}(C,D)` matrix; since the matrix is symmetric, the eigenvalue is real; the corresponding eigenvector is normalized and assigned to :obj:`~woo.core.Node.ori` of the element frame.

Stiffness matrix
----------------

.. todo:: Mostly from :cite:`FelippaAFEM`, chapter 9.
