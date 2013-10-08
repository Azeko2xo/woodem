Ellipsoid
-----------

:obj:`Elllipsoids <woo.dem.Ellipsoid>` are defined by their local coordinates and three :obj:`semi-axes <woo.dem.Ellipsoid.semiAxes>`.

Ellipsoid-Ellipsoid contact
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Thsi algorithm is implemented in by :obj:`woo.dem.Cg2_Ellipsoid_Ellipsoid_L6Geom`.

Following :cite:`Perram1996`, we note :obj:`semi-axes <woo.dem.Ellipsoid.semiAxes>` :math:`a_i` and :math:`b_i` as :math:`i\in\{1,2,3\}`; local axes are given as sets of orthonormal vectors :math:`\vec{u}_i`, :math:`\vec{v}_i`. The intercenter vector reads :math:`\vec{R}=\vec{r}_b-\vec{r}_a`, with :math:`\vec{r}_a` and :math:`\vec{r}_b` being positions of ellipsoid centroids. We define matrices

.. math::
   :nowrap:
   
   \begin{align*}
      \mat{A}&=\sum_{i=1}^{3} a_i^{-1}\vec{u}_i\vec{u}_i^T \\
      \mat{B}&=\sum_{i=1}^{3} b_i^{-1}\vec{v}_i\vec{v}_i^T
   \end{align*}

which are both invertible for non-vanishing semi-axes; then we define the function

.. math::
   :label: perram-wertheim-S-lambda

   S(\lambda)=\lambda(1-\lambda)\vec{R}^T\big[\underbrace{(1-\lambda)\mat{A}^{-1}+\lambda\mat{B}^{-1}}_{\mat{G}}\big]^{-1}\vec{R}

where :math:`\lambda\in[0,1]` is an unknown parameter. The function :math:`S(\lambda)` is concave and non-zero for :math:`\lambda\in[0,1]` and zero at both end-points. It therefore has a sigle maximum. Using the `Brent's method <http://en.wikipedia.org/wiki/Brent_method>`__, we obtain the maximum value :math:`\Lambda` for which :math:`S(\Lambda)` is maximal.

.. todo:: This computation can be (perhaps substantially) sped up by using other iterative methods, such as Newton-Raphson, finding root of :math:`S'(\lambda)`, and re-using the value from the previous step as the initial guess. There are also papers suggesting better algorithms such as :cite:`Zheng2009`.

We define the Perram-Wertham potential (first introduced in :cite:`Perram1985`) as

.. math:: F_{AB}=\{\max S(\lambda)|\lambda\in[0,1]\}=S(\Lambda)

for which (:cite:`Perram1985`)

* :math:`F_{AB}<1` if both ellipsoids overlap,
* :math:`F_{AB}=1` if they are externaly tangent,
* :math:`F_{AB}>1` if the ellipsoids do not overlap.

:cite:`Perram1985` gives a geometrical interpretation of :math:`F_{AB}`, where

.. math:: F_{AB}=\mu^2

where :math:`\mu` is scaling factor which must be applied to both ellipses to be tangent.

Following :cite:`Donev2005` eq (18), we can compute the contact normal and the contact point of two ellipsoids as

.. math::
   :nowrap:

   \begin{align*}
      \vec{n}_c&=\mat{G}^{-1}\mat{R} \\ 
      \vec{r}_c&=\vec{r}_a+(1-\Lambda)\mat{A}^{-1}\cdot
   \end{align*}

with :math:`\mat{G}` defined in :eq:`perram-wertheim-S-lambda`; note that :math:`\vec{n}_c` is not normalized.

The penetration depth (overlap distance) can be reasoned out as follows. :math:`\mu` scaled ellipsoid sizes while keeping their distance, so that they become externally tangent. Therefore :math:`1/\mu` scales ellipsoid distance while keeping their sizes. With :math:`d=|\mat{R}|` being the current inter-center distance, we obtain

.. math:: u_n'=d-d_0=d-\frac{1}{\mu}d=d\left(1-\frac{1}{\mu}\right).

This is the displacement that msut be performed along :math:`\vec{R}` while the contact normal may be oriented differently; we therefore project :math:`u_n'` along :math:`\vec{R}` onto (normalized) :math:`\vec{n}_c` obtaining

.. math:: u_n=d\left(1-\frac{1}{\mu}\right)\normalized{\vec{R}}\cdot\normalized{\vec{n}_c}.


The :math:`u_n`, :math:`\normalized{\vec{n}}`, :math:`\vec{r}_c` can be fed to :ref:`contact-geometry-l6gom-generic` for further computation.

Ellipsoid-Wall intersection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. todo:: Write.
