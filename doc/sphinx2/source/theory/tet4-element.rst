.. _tet4_element:

=============
Tet4 element
=============

:obj:`~woo.fem.Tet4` is the simplest linear-interpolation tetrahedron-shaped element with 4 nodes, each having only 3 translational degrees of freedom (rotations of nodes are ignored).

Element coordinate system
--------------------------

Initial element configuration is the reference one, and element-local nodal positions (orientations are ignored) are stored in :obj:`~woo.fem.Tet4.refPos`. This reference configuration is created automatically when the element is first used (usually when :obj:`woo.dem.DynDt` asks for stiffnesses before the simulation starts, or by an explicit call to :obj:`~woo.fem.Tet4.setRefConf`).

Current element configuration is stored in :obj:`woo.fem.Tet4.node`; its origin is always at element's centorid (:obj:`~woo.fem.Tetra.getCentroid`), which is computed as average of nodal positions. The orientation is determined using the algorithm described in :cite:`Mostafa2014` (pg. 114, Procedure 1):

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

   * the hat operator :math:`\hat\bullet` is the skew-symmetric matrix representing cross-product by :math:`\bullet` (:cite:`Mostafa2014`, A.1):

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

Lumped mass and inertia
------------------------

Suppose the tetrahedron is defined by vertices :math:`\vec{v}_1, \vec{v}_2, \vec{v}_3, \vec{v}_4`.

Centroid of the tetrahedron is the average :math:`\vec{c}_g=\frac{1}{4}\vec{v}_i`.

Volume of tetrahedron can be computed in various ways, `wikipedia <http://en.wikipedia.org/wiki/Tetrahedron#Volume>`__ gives the two following:

.. math:: V=\frac{1}{6}\begin{vmatrix}(\vec{v}_1-\vec{v}_4)^T \\ (\vec{v}_2-\vec{v}_4)^T \\ (\vec{v}_3-\vec{v}_4)^T\end{vmatrix}=\frac{(\vec{v}_1-\vec{v}_4)\cdot\left((\vec{v}_2-\vec{v}_4)\times(\vec{v}_3-\vec{v}_4)\right)}{6}

where non-canonical vertex ordering yields negative volume value.

Inertia tensor of tetrahedron in term of its vertex coordinates (with respect to origin and global axes) is derived at `this excellent page <http://www.mjoldfield.com/atelier/2011/03/tetra-moi.html>`__. We only summarize the most important parts here.

In general, inertia tensor :math:`\mat{J}` of any body can be computed from covariance

.. math:: \mat{J}=\operatorname{tr}(\mat{C})\mat{I}_{3}-\mat{C}

where covariance is outer product of coordinates over the volume

.. math:: \mat{C}\equiv\int_V \vec{x}\vec{x}^T \d V .

The page referenced shows that covariance of generic tetrahedron can be derived by transforming covariance of unit tetrahedron, giving

.. math:: \mat{C}=\frac{V}{20}\left(\sum_i\vec{v}_i\vec{v}_i^T+\sum_i\vec{v}_i\sum\vec{v}_i^T\right) .

When lumping mass and inertia, only the part adjacent to each node is considered; tetrahedron is partitioned into 8 sub-tetrahedra. When we define :math:`\vec{v}_{ij}=\frac{\vec{v}_i+\vec{v}_j}{2}`, those sub-tetrahedra for node :math:`i` are defined by vertices :math:`\{\vec{v}_i,\vec{v}_{ij},\vec{v}_{ik},\vec{v}_{il}\}` and :math:`\{\vec{v}_{ij},\vec{v}_{ik},\vec{v}_{il},\vec{c}_g\}` (partitioning planes are voronoi tesselation of vertices). Mass and inertia of sub-tetrahedra are computed in node coordinate system, summed over all attached particles, and lumped into the node.
